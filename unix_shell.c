#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024

void parse_echo(char *input) {
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == ' ') {
            printf("SPACE\n");
        } else if (input[i] == '|') {
            printf("PIPE\n");
        } else {
            // Print the word character by character until a space or pipe is found
            int start = i;
            while (input[i] != ' ' && input[i] != '|' && input[i] != '\0') {
                i++;
            }
            // Print the word found
            char word[MAX_INPUT_SIZE];
            strncpy(word, &input[start], i - start);
            word[i - start] = '\0'; // Null terminate the string
            printf("%s\n", word);
            i--; // Adjust for loop increment
        }
    }
}

void print_spaces(char *input) {
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == ' ') {
            printf("SPACE\n");
        } else {
            putchar(input[i]);
        }
    }
    printf("\n");
}

void print_pipes(char *input) {
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '|') {
            printf("PIPE\n");
        } else {
            putchar(input[i]);
        }
    }
    printf("\n");
}

void built_in_commands(char *input, char *last_command) {
    if (strcmp(input, "help") == 0) {
        printf("Available commands: help, cd, mkdir, exit, !!\n");
    } else if (strncmp(input, "cd", 2) == 0) {
        char *path = input + 3;
        if (chdir(path) != 0) {
            perror("cd failed");
        }
    } else if (strncmp(input, "mkdir", 5) == 0) {
        char *dir_name = input + 6;
        if (mkdir(dir_name, 0777) != 0) {
            perror("mkdir failed");
        }
    } else if (strcmp(input, "exit") == 0) {
        exit(0);
    } else if (strcmp(input, "!!") == 0 && last_command != NULL) {
        printf("Running last command: %s\n", last_command);
        strcpy(input, last_command);  // Re-run the last command
    }
}

void execute_command(char *input) {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        char *args[] = {input, NULL};
        execvp(args[0], args);
        perror("exec failed");
        exit(1);
    } else if (pid > 0) {
        // Parent process waits for child to finish
        wait(NULL);
    } else {
        perror("fork failed");
    }
}

void execute_with_redirection(char *input) {
    char *cmd = strtok(input, ">");
    char *filename = strtok(NULL, " ");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        int fd = open(filename, O_WRONLY | O_CREAT, 0644);
        dup2(fd, STDOUT_FILENO); // Redirect stdout to file
        close(fd);
        
        char *args[] = {cmd, NULL};
        execvp(args[0], args);
        perror("exec failed");
        exit(1);
    } else if (pid > 0) {
        // Parent process waits for child to finish
        wait(NULL);
    } else {
        perror("fork failed");
    }
}

void execute_with_pipe(char *cmd1, char *cmd2) {
    int pipefd[2];
    pipe(pipefd);

    pid_t pid1 = fork();

    if (pid1 == 0) {
        // First child - write to pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        char *args1[] = {cmd1, NULL};
        execvp(args1[0], args1);
        perror("exec1 failed");
        exit(1);
    }

    pid_t pid2 = fork();

    if (pid2 == 0) {
        // Second child - read from pipe
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        char *args2[] = {cmd2, NULL};
        execvp(args2[0], args2);
        perror("exec2 failed");
        exit(1);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    wait(NULL);
}

int main() {
    char input[MAX_INPUT_SIZE];
    char last_command[MAX_INPUT_SIZE] = {0};

    while (1) {
        printf("shell> ");
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = 0; // Remove trailing newline

        // Save the input as last command for !!
        strcpy(last_command, input);

        // Check for special commands
        if (strstr(input, "ECHO") != NULL) {
            parse_echo(input);
        } else if (strchr(input, ' ') != NULL) {
            print_spaces(input);
        } else if (strchr(input, '|') != NULL) {
            print_pipes(input);
        } else if (strstr(input, "help") || strstr(input, "cd") || strstr(input, "mkdir") || strstr(input, "exit") || strstr(input, "!!")) {
            built_in_commands(input, last_command);
        } else if (strchr(input, '>') != NULL) {
            execute_with_redirection(input);
        } else if (strchr(input, '|') != NULL) {
            char *cmd1 = strtok(input, "|");
            char *cmd2 = strtok(NULL, "|");
            execute_with_pipe(cmd1, cmd2);
        } else {
            execute_command(input);
        }
    }

    return 0;
}
