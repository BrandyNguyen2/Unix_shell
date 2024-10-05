#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024

// 1, 2, & 3 (GOOD): parses commands and echo them back
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

// 4 (!! still needs to be fixed): run built-in commands
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
// 5 (GOOD): execute without command line redirection
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

// 6 (GOOD?): execute with command line redirection
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
        //perror("exec failed!");
        exit(1);
    } else if (pid > 0) {
        // Parent process waits for child to finish
        wait(NULL);
    } else {
        perror("fork failed");
    }
}

// 7 (NOT GOOD YET): execute two commands connected by a pipe
void process_pipe_cmds(char **argv1, char **argv2) {
  int p[2];

  if (pipe(p) < 0) {
    perror("Pipe failed");
    exit(1);
  }

  pid_t pid1 = fork();

  if (pid1 < 0) {
    perror("Fork for first command failed");
    exit(1);
  }

  if (pid1 == 0) {
    dup2(p[1], STDOUT_FILENO);
    close(p[0]);
    close(p[1]);

    if (execvp(argv1[0], argv1) < 0) {
      printf("Execution of first command failed");
      exit(1);
    }
  }

  pid_t pid2 = fork();

  if (pid2 < 0) {
    printf("Fork for second command failed");
    exit(1);
  }

  if (pid2 == 0) {
    dup2(p[0], STDIN_FILENO);
    close(p[1]);
    close(p[0]);

    if (execvp(argv2[0], argv2) < 0) {
      printf("Execution of second command failed");
      exit(1);
    }
  }

  close(p[0]);
  close(p[1]);

  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);
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
        } else if (strstr(input, "help") || strstr(input, "cd") || strstr(input, "mkdir") || strstr(input, "exit") || strstr(input, "!!")) {
            built_in_commands(input, last_command);
        } else if (strchr(input, '>') != NULL) {
            execute_with_redirection(input);
        } else if (strchr(input, '|') != NULL) {
            char *cmd1 = strtok(input, "|");
            char *cmd2 = strtok(NULL, "|");
            if (cmd1 != NULL && cmd2 != NULL) {
                // Remove leading and trailing spaces from cmd1 and cmd2
                while (*cmd1 == ' ') cmd1++;
                while (*cmd2 == ' ') cmd2++;
                cmd1[strcspn(cmd1, " ")] = 0;
                cmd2[strcspn(cmd2, " ")] = 0;
                process_pipe_cmds(cmd1, cmd2);
            }
        } else {
            execute_command(input);
        }
    }

    return 0;
}
