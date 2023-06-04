/**
 * Konstantinos Delis // email: csd4623@csd.uoc.gr
 * HY345 // Assignment1
 * Implementation of a Unix shell
 * supported:
 *  -All default unix commands(eg ls, pwd, mkdir, echo ....)
 *  -Extra commands: cd, exit
 *  -Declaration of global variables(eg myvar="Hello World!")
 *  -Pipelines of any size(eg cat file.txt|sort)
 *  -Signals: ctrl+S(freeze), ctrl+Q(unfreeze) and ctrl+Z/fg
 * 
 * note: the two extra commands, the variables and ctrl+Z/fg signals don't work in a pipeline
 * note2: fg doesn't have a second parameter, it continues all child processes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0
#define BUFFERSZ 1000
#define PRAMS 1000
#define READ 0
#define WRITE 1

int firstTokenFlag = TRUE, command_number, var_assign_flg, ds_parameter_num;
char **parsed_command;
pid_t child_process;

void sigtstp_handler(int signum)
{ /*Overriding the default ctrl+Z handler*/
    fprintf(stdout, "\nControl+Z detected, child with pid = %d put in background\n", child_process);
}

/*Reads the entire input regardless of size and returns it as a string*/
char *read_input()
{
    int index = 0, current_bufsz = BUFFERSZ;
    char current;
    char *buffer = malloc(sizeof(char) * BUFFERSZ);
    if (buffer == NULL)
    {
        fprintf(stderr, "malloc: unable to allocate memory");
        exit(EXIT_FAILURE);
    }

    while (TRUE)
    {
        current = getchar();

        /*Check if the size of the buffer's size needs to increase*/
        if (index >= current_bufsz)
        {
            current_bufsz += BUFFERSZ;
            buffer = (char *)realloc(buffer, sizeof(char *) * current_bufsz);
            if (buffer == NULL)
            {
                fprintf(stderr, "realloc: unable to allocate memory for buffer\n");
                exit(EXIT_FAILURE);
            }
        }

        /*Check if we read the entire input, if yes return*/
        if (current == '\n' || current == EOF)
        {
            buffer[index] = '\0'; /*Previous if guarantees buffer is big enough to add '\0'*/
            return buffer;
        }

        buffer[index] = current;
        index++;
    }
}

/*Tokenizes the input based on the "|" character and returns an array of strings, which contain the commands*/
char **tokenize_input(char *input)
{
    char **tokenizedInput = malloc(sizeof(char **));
    char *command;
    int command_no = 0;
    if (tokenizedInput == NULL)
    {
        fprintf(stderr, "malloc: unable to allocate memory");
        exit(EXIT_FAILURE);
    }
    /*Get the first command*/
    command = strtok(input, "|");
    if (command == NULL)
    {
        printf("input was null\n");
        return tokenizedInput;
    }
    tokenizedInput[0] = command;
    command_no++;
    /*using null with strtok to get the rest of the commands*/
    while ((command = strtok(NULL, "|")) != NULL)
    {
        command_no++;
        tokenizedInput = (char **)realloc(tokenizedInput, sizeof(char **) * command_no);
        if (tokenizedInput == NULL)
        {
            fprintf(stderr, "realloc: unable to allocate memory for tokenizedInput char* array\n");
            exit(EXIT_FAILURE);
        }
        if (tokenizedInput == NULL)
        {
            fprintf(stderr, "realloc: unable to allocate memory");
            exit(EXIT_FAILURE);
        }
        tokenizedInput[command_no - 1] = command;
    }
    command_number = command_no;
    return tokenizedInput; /*An array of strings containing all the commands*/
}

/*Splits the command and its parameters, returns the number of the parameters*/
int parse_command(char *entire_command)
{
    int whtspace_flag, parameter_no = 0, index = 0, commandRead_flg = FALSE, string_flg = FALSE;
    parsed_command = malloc(2 * sizeof(char **));
    parsed_command[0] = NULL;
    parsed_command[1] = NULL;

    whtspace_flag = TRUE;
    var_assign_flg = FALSE;
    ds_parameter_num = -1;
    parameter_no = 0;
    /*Splitting the parameters*/
    while (entire_command[index] != '\0' && entire_command[index] != '\n')
    {
        if (entire_command[index] == '"' && string_flg == FALSE)
        { /*Starting to read a string*/
            index++;
            parameter_no++;
            commandRead_flg = TRUE;
            string_flg = TRUE;
            parsed_command = (char **)realloc(parsed_command, sizeof(char **) * parameter_no); /*make the new pointer of 'parsed_command' point to the start of the string*/
            if (parsed_command == NULL)
            {
                fprintf(stderr, "realloc: unable to allocate memory for parsed_command char* array\n");
                exit(EXIT_FAILURE);
            }
            parsed_command[parameter_no - 1] = &entire_command[index];
        }
        else if (entire_command[index] == '"' && string_flg == TRUE)
        { /*End of the string*/
            string_flg = FALSE;
            entire_command[index] = '\0';
            whtspace_flag = TRUE;
        }
        else if (string_flg == TRUE)
        {
            /*Skip until the end of the string*/
        }
        else if (entire_command[index] == '$' && whtspace_flag == TRUE && entire_command[index + 1] != ' ')
        {/*parameter should be replaced later using getenv*/
            ds_parameter_num = parameter_no + 1; /*later the variable name will be used, so that it is replaced by its value*/
        }
        else if (whtspace_flag == FALSE && entire_command[index] == '=')
        {/*assignment of value to a variable*/
            whtspace_flag = TRUE;         /*Get ready to read value of var*/
            entire_command[index] = '\0'; /*Add terminating char at the end of the variable number*/
            var_assign_flg = TRUE;
        }
        else if (entire_command[index] == ' ' && whtspace_flag == TRUE)
        { /*We already read a space and we read another one*/
            /*Skip all spaces*/
        }
        else if (entire_command[index] == ' ' && whtspace_flag == FALSE)
        {/*First space after another char, means end of a param*/
            entire_command[index] = '\0'; /*Add terminating char at the end of the parameter*/
            whtspace_flag = TRUE;
        }
        else if (whtspace_flag == TRUE)
        { /*Reading a new param*/
            parameter_no++;
            commandRead_flg = TRUE;
            parsed_command = (char **)realloc(parsed_command, sizeof(char **) * parameter_no); /*make the new pointer of 'parsed_command' point to the parameter*/
            if (parsed_command == NULL)
            {
                fprintf(stderr, "realloc: unable to allocate memory for parsed_command char* array\n");
                exit(EXIT_FAILURE);
            }
            parsed_command[parameter_no - 1] = &entire_command[index];
            whtspace_flag = FALSE;
        }
        else if (whtspace_flag == FALSE)
        { /*Reading a parameter*/
            /*Skip until the end of the parameter*/
        }
        else
        { /*Should be impossible to reach this*/
            fprintf(stderr, "Unexpected Error\n");
            exit(EXIT_FAILURE);
        }
        index++;
    }
    parsed_command[parameter_no] = NULL;
    return parameter_no;
}

void exec_simple_com(char *input, char **commands)
{
    int status;
    pid_t pid;
    int parameter_number = parse_command(commands[0]);

    if (parameter_number > 0)/*Make sure command is not empty*/
    {
        if (ds_parameter_num > 0)/*there was a '$' character in the command, check value of variable*/
        {
            parsed_command[ds_parameter_num - 1] = getenv(parsed_command[ds_parameter_num - 1]);
        }

        if (strcmp(parsed_command[0], "exit") == 0 && parameter_number == 1)
        {/*free malloced space and exit*/
            free(input);
            free(parsed_command);
            free(commands);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(parsed_command[0], "cd") == 0 && parameter_number == 2)
        {/*change to the directory specified in the second parameter*/
            chdir(parsed_command[1]);
        }
        else if (var_assign_flg == TRUE && parameter_number == 2)
        {/*'=' char was detected in command, assign value to the variable, if variable exists overwrite value*/
            setenv(parsed_command[0], parsed_command[1], TRUE);
        }
        else if (strcmp(parsed_command[0], "fg") == 0 && parameter_number == 1)
        {/*Send SIGCONT to all the children processes that have been stopped*/
            kill(-1, SIGCONT);
            waitpid(-1, &status, WUNTRACED);/*wait for all children to finish*/
        }
        else
        {
            pid = fork();
            if (pid == 0)
            {/*Child process*/
                execvp(parsed_command[0], parsed_command);
                fprintf(stderr, "Failed execvp, no such command//wrong parameters\n");
                exit(EXIT_FAILURE);
            }
            else if (pid > 0)
            {/*wait for child process to terminate, or to be stopped(specified with WUNTRACED)*/
                child_process = waitpid(-1, &status, WUNTRACED);
            }
            else
            {
                fprintf(stderr, "Fork was unsuccessful");
            }
        }
    }
}

void exec_piped_com(char *input, char **commands)
{
    int status;
    int i = 0, fd_index = 0;
    pid_t pid;

    int fd[command_number - 1][2];

    for (i = 0; i < command_number - 1; i++)
    { /*Create all necessary pipes*/
        if (pipe(fd[i]) < 0)
        {
            fprintf(stderr, "pipe() failed\n");
            exit(EXIT_FAILURE);
        }
    }

    int current_command = 1;
    while (current_command <= command_number)
    {
        parse_command(commands[current_command - 1]);
        pid = fork();
        if (pid == 0)
        {

            if (current_command < command_number)
            { /*If it's not the last command, replace stdout with the*/
                if (dup2(fd[fd_index][WRITE], STDOUT_FILENO) < 0)
                { /*'write' part of the fd of the pipeline*/
                    fprintf(stderr, "dup2() failed\n");
                    return;
                }
            }

            if (current_command != 1)
            { /*If it is not the first command, replace stdin with the*/
                if (dup2(fd[fd_index - 1][READ], STDIN_FILENO) < 0)
                { /*'read' part of the fd of the previous command*/
                    fprintf(stderr, "dup2() failed\n");
                    return;
                }
            }

            for (i = 0; i < command_number - 1; i++)
            { /*Close the child's pipes*/
                close(fd[i][READ]);
                close(fd[i][WRITE]);
            }

            execvp(parsed_command[0], parsed_command); /*Execute the command*/
            fprintf(stderr, "Failed execvp, no such command//wrong parameters\n");
            exit(EXIT_FAILURE);
        }
        else if (pid < 0)
        {
            fprintf(stderr, "Fork unsuccessful\n");
            exit(EXIT_FAILURE);
        }

        current_command++;
        fd_index++;
    }

    for (i = 0; i < command_number - 1; i++)
    { /*Parent closes all pipes*/
        close(fd[i][READ]);
        close(fd[i][WRITE]);
    }

    /*Tried 'waitpid(-1, &status, 0), but causes a visual error(prompt print is out of sync)
    but the shell continues to operate normally (could't find the bug, so I replaced waitpid with this)*/
    for (i = 0; i < command_number; i++)
    { /*Parent waits for children processes to finish*/
        wait(&status);
    }
}

void printPrompt()
{
    char currentDir[200];
    getcwd(currentDir, 200);
    printf("<%s>cs345sh/<%s>$ ", getlogin(), currentDir);
}

int main()
{
    char *input, **commands;
    int parameter_number;
    int current_command = 0, param_flag, status;
    pid_t pid;
    struct termios config;
    input = malloc(sizeof(char));            /*To avoid "double free" undefined behavior*/
    parsed_command = malloc(sizeof(char *)); /*To avoid "double free" undefined behavior*/
    commands = malloc(sizeof(char *));       /*To avoid "double free" undefined behavior*/

    /*Re-enabling ctrl+S and ctrl+Q*/
    tcgetattr(STDIN_FILENO, &config);
    config.c_cc[VSTOP] = '\023';
    config.c_cc[VSTART] = '\021';
    config.c_iflag |= IXON | IXOFF;
    tcsetattr(STDIN_FILENO, TCIFLUSH, &config);

    signal(SIGTSTP, sigtstp_handler);

    while (TRUE)
    {
        free(input);
        free(parsed_command);
        free(commands);
        printPrompt();
        input = read_input();

        if (strcmp(input, "\n") != 0 && strcmp(input, "\0") != 0)
        {
            commands = tokenize_input(input); /*Tokenize the input base on the '|' character*/

            if (command_number == 1)
            { /*A simple command*/
                exec_simple_com(input, commands);
            }
            else if (command_number > 1)
            { /*A series of piped commands*/
                exec_piped_com(input, commands);
            }
            else
            {
                fprintf(stderr, "Error tokenization of input failed\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            commands = malloc(sizeof(char **));       /*To avoid "double free" undefined behavior*/
            parsed_command = malloc(sizeof(char **)); /*To avoid "double free" undefined behavior*/
        }
    }
    return EXIT_FAILURE;
}
