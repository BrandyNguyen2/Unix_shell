#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>

#define MAXCOM 1000 // max number of letters to be supported
#define MAXLIST 100 // max number of commands to be supported

// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")

void parse(char *line, char **argv)
{
    while (*line != '\0')
    {
        while (*line == ' ' || *line == '\t' || *line == '\n')
        {
            *line++ = '\0'; // replace white spaces with null terminator
        }
        *argv++ = line; // store the argument position
        while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
        {
            line++; // move to the next space
        }
    }
    *argv = NULL; // mark the end of argument list
}

void parse_and_echo(char *line, char **argv)
{
    int echo = 0;
    int argc = 0;

    while (*line != '\0')
    { 
        while (*line == ' ' || *line == '\t' || *line == '\n')
        {
            *line++ = '\0'; // replace white spaces with 0
        }

        if (*line != '\0')
        {
            argv[argc++] = line;
        }

        while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
        {
            line++;
        }
    }

    argv[argc] = '\0'; // mark the end of argument list

    // Check if last argument is "ECHO"
    if (argc > 0 && strcmp(argv[argc - 1], "ECHO") == 0)
    {
        echo = 1;
    }

    // Print each word on a new line if "ECHO" is present
    if (echo)
    {
        for (int i = 0; i < argc - 1; i++)
        {
            printf("%s\n", argv[i]);
        }
    }
}

void execute_pipe(char **argv1, char **argv2)
{
    int pipefd[2];
    pid_t pid1, pid2;

    pipe(pipefd);

    pid1 = fork();
    if (pid1 == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(argv1[0], argv1);
        exit(0);
    }

    pid2 = fork();
    if (pid2 == 0)
    {
        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to pipe
        close(pipefd[1]);
        close(pipefd[0]);
        execvp(argv2[0], argv2);
        exit(0);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    wait(NULL);
}

int split_pipe(char *line, char **cmd1, char **cmd2)
{
    char *pipe_position = strchr(line, '|');
    if (pipe_position != NULL)
    {
        *pipe_position = '\0'; // split into two commands
        parse(line, cmd1);
        parse(pipe_position + 1, cmd2);
        return 1;
    }
    return 0;
}

void execute(char **argv)
{
    int i = 0;
    int in_redirect = 0, out_redirect = 0;
    char *input_file = NULL, *output_file = NULL;

    while (argv[i] != NULL)
    {
        if (strcmp(argv[i], "<") == 0)
        {
            in_redirect = 1;
            input_file = argv[i + 1];
            argv[i] = NULL; // Cut the command here
        }
        else if (strcmp(argv[i], ">") == 0)
        {
            out_redirect = 1;
            output_file = argv[i + 1];
            argv[i] = NULL;
        }
        i++;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        if (in_redirect)
        {
            freopen(input_file, "r", stdin);
        }
        if (out_redirect)
        {
            freopen(output_file, "w", stdout);
        }
        execvp(argv[0], argv);
        exit(0);
    }
    else
    {
        wait(NULL);
    }
}
\

void detect_special_chars(char *line)
{
    while (*line != '\0')
    {
        if (*line == ' ')
        {
            printf("SPACE ");
        }
        else if (*line == '|')
        {
            printf("PIPE ");
        }
        else
        {
            putchar(*line);
        }
        line++;
    }
    printf("\n");
}

void execute_builtin(char **argv)
{
    if (strcmp(argv[0], "help") == 0)
    {
        printf("Supported commands:\n");
        printf("- help\n");
        printf("- cd\n");
        printf("- mkdir\n");
        printf("- exit\n");
        printf("- !! (repeats the last command)\n");
    }
    else if (strcmp(argv[0], "cd") == 0)
    {
        if (argv[1] != NULL)
        {
            if (chdir(argv[1]) != 0)
            {
                perror("cd failed");
            }
        }
        else
        {
            printf("Expected argument to \"cd\"\n");
        }
    }
    else if (strcmp(argv[0], "mkdir") == 0)
    {
        if (argv[1] != NULL)
        {
            if (mkdir(argv[1], 0777) == -1)
            {
                perror("mkdir failed");
            }
        }
        else
        {
            printf("Expected argument to \"mkdir\"\n");
        }
    }
}

void main(void)
{
    char line[1024];
    char *argv[64], *argv_pipe[64];
    char last_command[1024] = "";

    while (1)
    {
        printf("Shell -> ");
        fgets(line, 1024, stdin);

        if (strcmp(line, "!!\n") == 0)
        {
            if (strlen(last_command) > 0)
            {
                strcpy(line, last_command);
            }
            else
            {
                printf("No previous command.\n");
                continue;
            }
        }
        else
        {
            strcpy(last_command, line); // Save the last command
        }

        if (split_pipe(line, argv, argv_pipe)) // If pipe is present
        {
            execute_pipe(argv, argv_pipe);
        }
        else
        {
            parse(line, argv); // Regular command parsing

            if (argv[0] == NULL)
                continue; // Skip if no command was entered

            if (strcmp(argv[0], "exit") == 0)
            {
                exit(0);
            }
            else if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "cd") == 0 || strcmp(argv[0], "mkdir") == 0)
            {
                execute_builtin(argv);
            }
            else
            {
                execute(argv);
            }
        }
    }
}

