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
int redirect = 0;
int redirect_stderr = 0;
int add2file = 0;
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
        // printf("%s: ", prompt);
        // fgets(command, 1024, stdin);
        // command[strlen(command) - 1] = '\0';
        // last++;
        //last = last%20;
        command = read_command();
        //strncpy(history[last] , command, 1024);

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

        // if (! strcmp(argv[0], "hist")){
        //     for (int i=19 ; i>=0 ; i--){
        //         printf("%d : %s\n", i, history[i]);
        //     }
        //     continue;
        // }
        
        if ((! strcmp(argv[0], "prompt")) && (argv[1] != NULL) && (! strcmp(argv[1], "=")) && (argv[2] != NULL) && (argv[3] == NULL)){
            prompt = argv[2];
            prompt_plus = argv[2];
            strcat(prompt_plus , ": ");
            continue;
        }
     
        if (! strcmp(argv[0], "echo")){
            if ((argv[1]!=NULL) && (! strcmp(argv[1], "$?"))){
                printf("%d ????" , status);
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


        /* Does command line end with & */ 
        if (! strcmp(argv[i - 1], "&")) {
            amper = 1;
            argv[i - 1] = NULL;
        }
        else 
            amper = 0;

        if (! strcmp(argv[i - 2], ">")) {
            redirect = 1;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
            }
        else if (! strcmp(argv[i - 2], "2>")) {
            redirect_stderr = 1;
            argv[i - 2] = NULL;
            outfile = argv[i - 1];
            }
        else if (! strcmp(argv[i - 2], ">>")) {
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
                fd = open(outfile, O_CREAT | O_APPEND | O_WRONLY, 0644);
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
