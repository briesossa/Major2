#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define MAX_LINE 512
#define MAX_ARGS 100
#define HISTORY_SIZE 20

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
