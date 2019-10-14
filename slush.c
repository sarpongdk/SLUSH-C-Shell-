#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

void display_error(char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int min(int x, int y) {
    if (x < y) {
        return x;
    }

    return y;
}

int validate(char* input_str) {
    // returns -1 if invalid; 0 otherwise
    if (input_str[0] == '(' || input_str[strlen(input_str) - 1] == '(') {
        return -1;
    }

    for (int i = 0; i < strlen(input_str) - 1; i++) {
        for (int j = i + 1; j < strlen(input_str); j++) {
            if (input_str[i] == input_str[j] && input_str[i] == '(') {
                return -1;
            }
        }
    }

    return 0;
}

// parse command from stdin
void parse(char* input_str, char* delim, char* buffer[]) {
    
    char* token = strtok(input_str, delim);
    int index=0;
    while ( token != NULL ) {
        int n = strlen(token);
        token[n] = '\0';
        buffer[index] = token;
        index++;

        token = strtok(NULL, delim);
    }
}

// return size of array
int find_size(char* buffer[]) {
    int result = 0;
    
    while (buffer[result] != NULL) {
        result += 1;
    }

    return result;
}

// executes command
void execute_prog(char* my_argv[]) {
    char* cmd = my_argv[0];
    int arg_size = find_size(my_argv);
    my_argv[arg_size] = NULL;

    if (execvp(cmd, my_argv) < 0) {
        display_error("Program: not found");
    }
}


void change_dir(char* path) {
    int err = chdir(path);

    if (err < 0) {
        display_error("Unable to change directories");
    }
}

// read input
char* read_input(char buffer[], int buffer_size) {
    char* pwd = getcwd(pwd, 80);
    char* home = getenv("HOME");
    char prompt[80] = "[slush|";
    strcat(prompt, pwd);
    strcat(prompt, "] ");
    printf("%s ", prompt);

    char* input_str = fgets(buffer, buffer_size, stdin);
    if (input_str == NULL) {
        return NULL;
    }

    int size = strlen(input_str);
    input_str[size - 1] = '\0';

    return input_str;
}

void handler(int sig) {
    exit(EXIT_SUCCESS);
}


void pipeline(char* commands[], int num_operations, char* my_argv[]);
int pipeline_helper(char* commands[], int num_operations, int i, char* args_delim, char* my_argv[]); // int old_read_fd); // int new_write_fd, int old_read_fd, int old_write_fd);

int main( int argc, char** argv ) {
    int buffer_size = 1024;
    char buffer[buffer_size];
    
    char* commands[buffer_size];
    memset(commands, 0, buffer_size*sizeof(char*));

    char* input_str;
    char* cmd_delim = "(";

    signal(SIGINT, handler);

    while (1) {
        char cwd[100];
        char* pwd = getcwd(cwd, 80);
        char* home = getenv("HOME");
        int len = strlen(home);
        char* dir = (pwd + len);
        char prompt[80] = "[slush|";

        strcat(prompt, dir);
        strcat(prompt, "] ");
        printf("%s ", prompt);
        input_str = fgets(buffer, 1024, stdin);

        if (input_str == NULL) {
            break;
        } else if (input_str[0] == '\n') {
            input_str = read_input(buffer, buffer_size);
        }
        

        input_str[strlen(input_str) - 1] = '\0';

        if (validate(input_str) == -1) {
            display_error("Invalid null command");
        }

        parse(input_str, cmd_delim, commands);
        
        int max_args = 15;
        int max_argv_size = max_args + 2; 
        char* args_delim = " ";
        char* my_argv[max_argv_size];
	    memset(my_argv, 0, max_argv_size*sizeof(char*));
        int num_operations = find_size(commands);

        switch (num_operations) {
                case 1:
                    parse(commands[0], args_delim, my_argv);
                    if (my_argv[0] == NULL) {
                        display_error("Command not found");
                    }

                    if (strcmp(my_argv[0], "cd") == 0) {
                        if (my_argv[1] == NULL) {
                            my_argv[1] = "/";
                        }

                        change_dir(my_argv[1]);
                    } else {
		                int child = fork();

			            if (child < 0) {
		                    display_error("Unable to fork new process");
			            } else if (child == 0) {
			                // in child process
		                    execute_prog(my_argv);
                            exit(EXIT_SUCCESS);
                        }

			            // parent process
			            waitpid(child, NULL, 0);
			 
		            }  

                    break;

                default: 
                    pipeline(commands, num_operations, my_argv);
                    break;
        }

    } 

    return EXIT_SUCCESS;
}

void pipeline(char* commands[], int num_operations, char* my_argv[]) {
    pipeline_helper(commands, num_operations, 1, " ", my_argv); // STDIN_FILENO, STDOUT_FILENO);
}

// pipe[1] - write end, pipe[0] - read end
int pipeline_helper(char* commands[], int num_operations, int i, char* args_delim, char* my_argv[]) { 

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        display_error("Unable to instantiate pipe");
    }

    if (i == 1) {
        close(pipefd[0]);
        close(pipefd[1]);
    }

    // base case
    if (i == num_operations) {
        parse(commands[num_operations - 1], args_delim, my_argv);

        if (my_argv[0] == NULL) {
        	display_error("Invalid argument provided");
        }

        int child = fork();
        if (child < 0) {
            display_error("Unable to fork child process");
        } else if (child == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            execute_prog(my_argv);  
        }

        close(pipefd[1]);
        waitpid(child, NULL, 0);
        return pipefd[0];
    }

    int old_fd = pipeline_helper(commands, num_operations, i + 1, args_delim, my_argv); 

    parse(commands[i - 1], args_delim, my_argv);

    int child = fork();
    if (child < 0) {
        display_error("Unable to fork child process");
    } else if (child == 0) {
        
        if (i == 1) {
            // last command
            dup2(old_fd, STDIN_FILENO);
        } else  {
            // middle command
            dup2(old_fd, STDIN_FILENO);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
        }

        execute_prog(my_argv);  
    }

    // in parent process
    if (i == 1) {
        // last command
        close(old_fd);
    } else {
        // middle command
        close(old_fd);
        close(pipefd[1]);
    }

    waitpid(child, NULL, 0);
    return pipefd[0];    
}
