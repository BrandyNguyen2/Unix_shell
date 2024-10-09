#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024

void parse_command(char *command, char **argv);

// 1, 2, & 3: parses commands and echo them back
void parse_echo(char *input) 
{
    for (int i = 0; input[i] != '\0'; i++) 
    {
        if (input[i] == ' ') 
        {
            printf("SPACE\n");
        } 
        else if (input[i] == '|') 
        {
            printf("PIPE\n");
        } 
        else 
        {
            int start = i;
            while (input[i] != ' ' && input[i] != '|' && input[i] != '\0') 
            {
                i++;
            }
            char word[MAX_INPUT_SIZE];
            strncpy(word, &input[start], i - start);
            word[i - start] = '\0';
            printf("%s\n", word);
            i--; 
        }
    }
}

// 4: run built-in commands
void built_in_commands(char *input, char *last_command) 
{
    if (strcmp(input, "help") == 0) {
        printf("Available commands:\n");
        printf("help: displays available commands\n");
        printf("cd: changes directory\n");
        printf("mkdir: creates a new directory\n");
        printf("exit: exits the shell\n");
        printf("!!: reruns the last command\n");
    } 
    else if (strncmp(input, "cd", 2) == 0) 
    {
        char *path = input + 3;
        if (strcmp(path, "~") == 0) 
        {
            path = getenv("HOME");
        }
        if (chdir(path) != 0) 
        {
            perror("cd failed");
        }
    } 
    else if (strncmp(input, "mkdir", 5) == 0) 
    {
        char *dir_name = input + 6;
        if (mkdir(dir_name, 0777) != 0) 
        {
            perror("mkdir failed");
        }
    } 
    else if (strcmp(input, "exit") == 0) 
    {
        exit(0);
    } 
    else if (strcmp(input, "!!") == 0) 
    {
        if (last_command[0] != '\0') 
        {
            strcpy(input, last_command); 
        } 
        else 
        {
            printf("No previous command.\n");
            input[0] = '\0'; // 
        }
    }
}

// 5 : execute without command line redirection (just like fork() from hw)
void execute_command(char *input) 
{
    char *args[MAX_INPUT_SIZE];
    parse_command(input, args);  
    pid_t pid = fork();

    if (pid == 0) 
    {
        execvp(args[0], args);  
        perror("exec failed");  
        exit(1);
    } 
    else if (pid > 0) 
    {
        // Parent process: waits for child process to finish
        wait(NULL);
    } 
    else 
    {
        // Fork failed
        perror("fork failed");
    }
}

// 6: execute with command line redirection
void execute_with_redirection(char *input) 
{
    char *cmd = strtok(input, ">");
    char *filename = strtok(NULL, " ");
    
    pid_t pid = fork();
    
    if (pid == 0)
    {
        int fd = open(filename, O_WRONLY | O_CREAT, 0644);
        dup2(fd, STDOUT_FILENO); 
        close(fd);
        
        char *args[] = {cmd, NULL};
        execvp(args[0], args);
        //perror("exec failed!");
        exit(1);
    } 
    else if (pid > 0) 
    {
        wait(NULL);
    } 
    else 
    {
        perror("fork failed");
    }
}

// 7 (John Carlo Manuel helped me with this function): execute two commands connected by a pipe
void process_pipe_cmds(char **argv1, char **argv2) 
{
    int p[2];

    if (pipe(p) < 0) 
    {
        perror("Pipe failed");
        exit(1);
    }

    pid_t pid1 = fork();

    if (pid1 < 0) 
    {
        perror("Fork for first command failed");
        exit(1);
    }

    if (pid1 == 0) 
    {
        dup2(p[1], STDOUT_FILENO);
        close(p[0]);
        close(p[1]);

        if (execvp(argv1[0], argv1) < 0) 
        {
        printf("Execution of first command failed");
        exit(1);
        }
    }

    pid_t pid2 = fork();

    if (pid2 < 0) 
    {
        printf("Fork for second command failed");
        exit(1);
    }

    if (pid2 == 0) 
    {
        dup2(p[0], STDIN_FILENO);
        close(p[1]);
        close(p[0]);

        if (execvp(argv2[0], argv2) < 0) 
        {
        printf("Execution of second command failed");
        exit(1);
        }
    }

    close(p[0]);
    close(p[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void parse_command(char *command, char **argv) 
{
    int index = 0;
    char *token = strtok(command, " ");
    while (token != NULL) 
    {
        argv[index++] = token;
        token = strtok(NULL, " ");
    }
    argv[index] = NULL;  // Null-terminate the argument list for execvp
}


int main() 
{
    char input[MAX_INPUT_SIZE];
    char last_command[MAX_INPUT_SIZE] = {0};

    while (1) 
    {
        printf("shell> ");
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "!!") == 0) 
        {
            if (last_command[0] != '\0') 
            {
                strcpy(input, last_command); 
            } 
            else 
            {
                printf("No previous command.\n");
                continue;
            }
        } 
        else 
        {
            strcpy(last_command, input); 
        }
        if (strstr(input, "ECHO") != NULL) 
        {
            parse_echo(input);
        } 
        else if (strstr(input, "help") || strstr(input, "cd") || strstr(input, "mkdir") 
        || strstr(input, "exit") || strcmp(input, "!!") == 0) 
        {
            built_in_commands(input, last_command);
            if (input[0] == '\0') 
            {
                continue; 
            }
        } 
        else if (strchr(input, '>') != NULL) 
        {
            execute_with_redirection(input);
        } 
        else if (strchr(input, '|') != NULL) 
        {
            char *cmd1 = strtok(input, "|");
            char *cmd2 = strtok(NULL, "|");
            if (cmd1 != NULL && cmd2 != NULL) 
            {
                char *argv1[MAX_INPUT_SIZE];
                char *argv2[MAX_INPUT_SIZE];
                parse_command(cmd1, argv1);
                parse_command(cmd2, argv2);
                process_pipe_cmds(argv1, argv2);
            }
        } 
        else if (input[0] != '\0') 
        {
            execute_command(input);
        }
    }
    
    return 0;
}
