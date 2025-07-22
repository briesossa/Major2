#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

// 7/21/25
// CSCE 3600.001
// Brianna Jackson, Lucero Torres, Anthony Ayala, Alexandre DeWolf
// This program has two main functions. Batch mode and Interactive mode. Interactive mode allows the user to manually input commands into the prompt.
// In batch mode, you provide a batch file that contains a list on commands to be executed. This program can take those commands and echo them to the user before executing them.

#define MAX_LINE 512
#define MAX_ARGS 100
#define HISTORY_SIZE 20
#define MAX_ALIASES 100 //mox number of aliases

char *history[HISTORY_SIZE];
int history_count = 0;

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void add_history(char *line) {
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(line);
        history_count++;
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; ++i)
            history[i - 1] = history[i];
        history[HISTORY_SIZE - 1] = strdup(line);
    }
}

void clear_history() {
    for (int i = 0; i < history_count; ++i) {
        free(history[i]);
    }
    history_count = 0;
}

void print_history() {
    for (int i = 0; i < history_count; ++i) {
        printf("%d: %s", i + 1, history[i]);
    }
}

void execute_command(char *cmd);

void handle_history_command(char *arg) {
    if (strcmp(arg, "-c") == 0) {
        clear_history();
    } else if (strncmp(arg, "-e", 2) == 0) {
        int index = atoi(arg + 2) - 1;
        if (index >= 0 && index < history_count) {
            execute_command(history[index]);
        } else {
            print_error();
        }
    } else {
        print_error();
    }
}

typedef struct {        //struct to define an alias
    char *command;  //command part
    char *name;     //name part
}   Alias;

Alias alias_list[MAX_ALIASES];  //alias_list array
int alias_count = 0;    //alias_list size

void add_alias(char *arg) { //adds alias to alias_list

    char *copy = strdup(arg);   //copies arg to not cause possible problems later

    //splits arg into usable strings
    char *name = strtok(copy, "=");
    char *command_quotes = strtok(NULL, "=");
    char *command_no_quotes = malloc(strlen(command_quotes) + 1);

    //removes single quotes from command str
    int j = 0;
    for (int i = 0; i < strlen(command_quotes); ++i) {
        if (!command_quotes) {
            print_error();
            return;
        }
        if (command_quotes[i] != '\'') {
            command_no_quotes[j] = command_quotes[i];
            j++;
        }
    }
    command_no_quotes[j] = '\0';

    //creates alias
    for (int i = 0; i < alias_count; ++i) {
        if (strcmp(alias_list[i].name, name) == 0) {
            free(alias_list[i].command);
            alias_list[i].command = command_no_quotes;
            return;
        }
    }

    //adds alias to alias_list
    if (alias_count < MAX_ALIASES) {
        alias_list[alias_count].name = strdup(name);
        alias_list[alias_count].command = strdup(command_no_quotes);
        alias_count++;
    } else {
        print_error();
        printf("Alias list is full\n");
    }
}

void print_alias_list() {

    //checks if alias_list is empty, if not, prints alias_list
    if (alias_count == 0) {
        printf("Alias list is empty\n");
    }
    else {
        for (int i = 0; i < alias_count; ++i) {
            printf("%s: %s\n", alias_list[i].name, alias_list[i].command);
        }
    }
}

void alias_remove(char *arg) {

    //removes/deletes specified alias from alias_list
    for(int i = 0; i < alias_count; ++i) {
        if (strcmp(alias_list[i].name, arg) == 0)
        {
            free(alias_list[i].name);
            free(alias_list[i].command);
            --alias_count;
        }
    }
}

void alias_remove_all() {

    //removes/deletes all aliases from alias_list
    for (int i = 0; i < alias_count; ++i) {
        free(alias_list[i].name);
        free(alias_list[i].command);
    }
    alias_count = 0;
}

void execute_command(char *cmd) {
    char *args[MAX_ARGS];
    int arg_idx = 0;

    // Redirection flags
    char *input_file = NULL;
    char *output_file = NULL;

    // Check for redirection symbols
    char *in_redirect = strchr(cmd, '<');
    char *out_redirect = strchr(cmd, '>');

    if (in_redirect) {
        *in_redirect = '\0';
        input_file = strtok(in_redirect + 1, " \t\n");
    }
    if (out_redirect) {
        *out_redirect = '\0';
        output_file = strtok(out_redirect + 1, " \t\n");
    }

    // Tokenize command
    char *token = strtok(cmd, " \t\n");
    while (token != NULL && arg_idx < MAX_ARGS - 1) {
        args[arg_idx++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_idx] = NULL;

    if (args[0] == NULL) return; // Empty command

    // Built-ins
    if (strcmp(args[0], "cd") == 0) {
        const char *path = args[1] ? args[1] : getenv("HOME");
        if (chdir(path) != 0) print_error();
        return;
    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "myhistory") == 0) {
        if (args[1]) handle_history_command(args[1]);
        else print_history();
        return;
    } else if (strcmp(args[0], "alias") == 0) {

        int arg_num = 0;
        while (args[arg_num] != NULL) {arg_num++;}

        if (arg_num == 1) {
            print_alias_list();
            return;
        } else if (arg_num == 2) {
            if (strchr(args[1], '=') != NULL) { //checks if second arg has equal sign
                add_alias(args[1]);
                return;
            } else if (strcmp(args[1], "-c") == 0) {
                alias_remove_all();
                return;
            }
        } else if (arg_num == 3) {
            if (strcmp(args[1], "-r") == 0) {
                alias_remove(args[2]);
                return;
            }
        }
        return;
    } else if (strcmp(args[0], "man") == 0) {
        if (args[1] && strcmp(args[1], "alias") == 0) {
            printf("alias is used to create aliases/shortcuts for commands\n");
            printf("alias <name>='<command>'    : creates a new alias\n");
            printf("alias -r <name>             : removes an alias\n");
            printf("alias -c                    : removes all aliases\n");
            printf("alias                       : lists all aliases\n");
            printf("<name>                      : executes alias cmd\n");
        }
    }

    //checks for if the entered command matches an alias in alias_list and then tokenizes alias command
    for (int i = 0; i < alias_count; ++i) {
        if (strcmp(args[0], alias_list[i].name) == 0) {
            char *alias_cmd_copy = strdup(alias_list[i].command);

            int og_count = 0; //original count of args
            char *original_args[MAX_ARGS];
            for (int j = 1; j < MAX_ARGS && args[j] != NULL; ++j) {
                original_args[og_count++] = args[j];
            }

            //tokenizes alias command to be properly executed
            arg_idx = 0;
            char *alias_token = strtok(alias_cmd_copy, " \t\n");
            while (alias_token != NULL && arg_idx < MAX_ARGS - 1) {
                args[arg_idx++] = alias_token;
                alias_token = strtok(NULL, " \t\n");
            }

            for (int j = 0; j < og_count && args[j] != NULL; ++j) {     //appends remaining args to end of aliased cmd
                if (arg_idx < MAX_ARGS - 1) {
                    args[arg_idx++] = original_args[j];
                }
            }

            args[arg_idx] = NULL;
            break;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
    } else if (pid == 0) {
        // Redirection setup
        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (output_file) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Execute
        execvp(args[0], args);
        print_error();
        exit(1);
    } else {
        wait(NULL);
    }
}

void process_line(char *line) {
    add_history(line);
    char *command = strtok(line, ";");
    while (command != NULL) {
        execute_command(command);
        command = strtok(NULL, ";");
    }
}

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    char line[MAX_LINE];

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            print_error();
            exit(1);
        }
    } else if (argc > 2) {
        print_error();
        exit(1);
    }

    while (1) {
        if (input == stdin) printf("prompt> ");
        if (!fgets(line, MAX_LINE, input)) break;
        if (input != stdin) printf("%s", line);
        process_line(line);
    }

    if (input != stdin) fclose(input);
    clear_history();
    return 0;
}

