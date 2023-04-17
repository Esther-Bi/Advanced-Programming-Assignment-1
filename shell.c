#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

void sig_handler(int signum){
    printf("You typed Control-C!\n");
}


int main() {
char * command;
char *prompt = "hello";
char *prompt_plus = "hello: ";
char *token;
char *outfile;
int i, fd, amper, retid, status;
char *argv[10];
char history[20][1024];
int last = 0;

char* read_command() {
    char* input = readline(prompt_plus);
    if (input && *input) {
        add_history(input);
        strncpy(history[last], input, 1024);
        last = (last + 1) % 20;
    }
    return input;
}

signal (SIGINT,sig_handler);

    while (1)
    {
        command = read_command();

        if (((strstr(command, " | ") != NULL) && (strstr(command, "if") != NULL) && (strstr(command, "then") != NULL))
            || ((strstr(command, " | ") != NULL) && (strstr(command, "if") == NULL))) {
        
            char **commands = NULL;
            int command_count = 0;
            int i;
            int command_capacity = 0;

            char *token = strtok(command, "|");
            while (token != NULL) {
                // Resize commands array if necessary
                if (command_count >= command_capacity) {
                    command_capacity += 16;
                    commands = realloc(commands, command_capacity * sizeof(char*));
                }
                commands[command_count++] = strdup(token);
                token = strtok(NULL, "|");
            }

            // Add NULL terminator to end of commands array
            if (command_count >= command_capacity) {
                command_capacity++;
                commands = realloc(commands, command_capacity * sizeof(char*));
            }
            commands[command_count] = NULL;

            // Execute all commands
            int fd[2];
            int last_in = STDIN_FILENO;
            for (i = 0; i < command_count; i++) {
                // Split command string into array of arguments
                char *arg = strtok(commands[i], " ");
                char **args = malloc(sizeof(char*) * 2);
                int arg_count = 0;
                while (arg != NULL) {
                    args[arg_count++] = arg;
                    args = realloc(args, sizeof(char*) * (arg_count + 1));
                    arg = strtok(NULL, " ");
                }
                args[arg_count] = NULL;

                // Create pipe
                if (i != command_count - 1) {
                    if (pipe(fd) == -1) {
                        perror("pipe failed");
                        exit(EXIT_FAILURE);
                    }
                }

                // Execute command
                pid_t pid = fork();
                if (pid == 0) {
                    // Redirect input from previous command (if not first command)
                    if (last_in != STDIN_FILENO) {
                        if (dup2(last_in, STDIN_FILENO) == -1) {
                            perror("dup2 failed");
                            exit(EXIT_FAILURE);
                        }
                        close(last_in);
                    }

                    // Redirect output to next command (if not last command)
                    if (i != command_count - 1) {
                        if (dup2(fd[1], STDOUT_FILENO) == -1) {
                            perror("dup2 failed");
                            exit(EXIT_FAILURE);
                        }
                        close(fd[1]);
                    }

                    // Execute command
                    execvp(args[0], args);
                    perror("execvp failed");
                    exit(EXIT_FAILURE);
                } else {
                    // Close input of previous command (if not first command)
                    if (last_in != STDIN_FILENO) {
                        close(last_in);
                    }

                    // Close output of this command (if not last command)
                    if (i != command_count - 1) {
                        close(fd[1]);
                    }

                    // Save input for next command (if not last command)
                    if (i != command_count - 1) {
                        last_in = fd[0];
                    }

                    // Wait for command to finish
                    wait(NULL);
                }

                // Free memory used by argument array
                free(args);
            }

            // Free memory used by input buffer and commands array
            for (i = 0; i < command_count; i++) {
                free(commands[i]);
            }
            free(commands);
            continue;
        }
        
        /* parse command line */
        i = 0;
        token = strtok (command," ");
        while (token != NULL)
        {
            argv[i] = token;
            token = strtok (NULL, " ");
            i++;
        }
        argv[i] = NULL;

        /* Is command empty */
        if (argv[0] == NULL)
            continue;

        if (! strcmp(argv[0], "!!")){
            /* parse command line */
            i = 0;
            token = strtok (history[(last-2)%20]," ");
            while (token != NULL)
            {
                argv[i] = token;
                token = strtok (NULL, " ");
                i++;
            }
            argv[i] = NULL;
        }
        
        if ((! strcmp(argv[0], "prompt")) && (argv[1] != NULL) && (! strcmp(argv[1], "=")) && (argv[2] != NULL) && (argv[3] == NULL)){
            prompt = argv[2];
            prompt_plus = argv[2];
            strcat(prompt_plus , ": ");
            continue;
        }
     
        if (! strcmp(argv[0], "echo")){
            if ((argv[1]!=NULL) && (! strcmp(argv[1], "$?"))){
                printf("%d" , status);
            } else if (argv[1]!=NULL){
                int i=1;
                while (argv[i] != NULL) {                                     
                    if (i == 1){
                        printf("%s", (getenv(argv[i]) != NULL) ? getenv(argv[i]) : argv[i]);
                    } else{
                        printf(" %s", (getenv(argv[i]) != NULL) ? getenv(argv[i]) : argv[i]);
                    }
                    i++;
                }
            }
            printf("\n");
            continue;
        }

        if ((! strcmp(argv[0], "cd")) && (argv[1])!=NULL){
            if (chdir(argv[1]) != 0) {
                printf("Error: Could not change directory to %s\n", argv[1]);
            }
            continue;
        }

        if (! strcmp(argv[0], "quit")){
            break;
        }

        if ((argv[0][0] == '$') && (argv[1]!=NULL) && (! strcmp(argv[1], "=")) && (argv[2]!=NULL)){
            setenv(argv[0],argv[2],1);
            continue;
        }

        if ((! strcmp(argv[0], "read")) && (argv[1]!=NULL)){
            char var[19];
            fgets(var, 19, stdin);
            var[strlen(var) - 1] = '\0';
            char c = '$';
            char result[20];
            result[0] = c;
            result[1] = '\0';
            strcat(result, argv[1]);
            if(setenv(result,var,1) == -1) {
                printf("Error setting environment variable\n");
                return 1;
            }
            continue;
        }

        if (! strcmp(argv[0], "if")) {
            
            char result[1024]= "";
            char *ch=malloc(sizeof("50"));
            ch = malloc (50);
            
            for(int num =0; num< i; num++){
                strcat(result, argv[num]);
                if (num != i-1)
                    strcat(result, " ");
            }            

            for(int j=0; j<5; j++){

                if (j==0 || j==2 || j==4){
                    strcat(result, "; ");
                }
                scanf ("%[^\n]%*c", ch);
                strcat(result, ch);
                strcat(result, " ");

            }
            argv[0] = "sh";
            argv[1] = "-c";
            argv[2] = strdup(result);
            argv[3] = NULL;
            free(ch);
            i = 3;
        }
        /* Does command line end with & */ 
        if ((argv[i - 1]!=NULL) &&(! strcmp(argv[i - 1], "&"))) {
            amper = 1;
            argv[i - 1] = NULL;
        }
        else 
            amper = 0;

        int redirect = 0;
        int redirect_stderr = 0;
        int add2file = 0;

        if ((argv[i - 2]!=NULL) && (! strcmp(argv[i - 2], ">"))) {
            redirect = 1;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
            }
        else if ((argv[i - 2]!=NULL) && (! strcmp(argv[i - 2], "2>"))) {
            redirect_stderr = 1;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
            }
        else if ((argv[i - 2]!=NULL) && (! strcmp(argv[i - 2], ">>"))) {
            add2file = 1;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
            }
        else{
            redirect = 0;
            redirect_stderr = 0;
            add2file = 0;
        }

        /* for commands not part of the shell command language */ 
        if (fork() == 0) { 
            //signal(SIGINT, SIG_DFL);
            /* redirection of IO ? */
            if (redirect) {
                fd = creat(outfile, 0660); 
                close (STDOUT_FILENO) ; 
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            if (redirect_stderr) {
                fd = creat(outfile, 0660); 
                close (STDERR_FILENO) ;
                dup(fd);
                close(fd);
                /* stderr is now redirected */
            }
            if (add2file) {
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
                close (STDOUT_FILENO) ; 
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            execvp(argv[0], argv);
        }
        /* parent continues here */
        if (amper == 0)
            retid = wait(&status);
    }
}