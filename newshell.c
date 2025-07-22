#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>

// 7/21/25
// CSCE 3600.001
// Brianna Jackson, Lucero Torres, Anthony Ayala, Alexandre DeWolf
// This program has two main functions. Batch mode and Interactive mode. Interactive mode allows the user to manually input commands into the prompt.
// In batch mode, you provide a batch file that contains a list on commands to be executed. This program can take those commands and echo them to the user before executing them.

#define MAX_LINE 512
#define MAX_ARGS 100
#define HISTORY_SIZE 20
#define MAX_ALIASES 100
#define MAX_PIPE_CMDS 3
#define MAX_PATHS 100

char *history[HISTORY_SIZE];
int history_count = 0;

char *custom_paths[MAX_PATHS];
int path_count = 0;

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void init_shell() {
    shell_terminal = STDIN_FILENO;
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("Couldn’t put shell in its own process group");
        exit(1);
    }
    tcsetpgrp(shell_terminal, shell_pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
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
void execute_pipeline(char *line);

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

typedef struct {
    char *command;
    char *name;
} Alias;

Alias alias_list[MAX_ALIASES];
int alias_count = 0;

void add_alias(char *arg) {
    char *copy = strdup(arg);
    char *name = strtok(copy, "=");
    char *command_quotes = strtok(NULL, "=");
    if (!command_quotes) {
        print_error();
        return;
    }
    char *command_no_quotes = malloc(strlen(command_quotes) + 1);
    int j = 0;
    for (int i = 0; i < strlen(command_quotes); ++i) {
        if (command_quotes[i] != ''') {
            command_no_quotes[j++] = command_quotes[i];
        }
    }
    command_no_quotes[j] = '\0';

    for (int i = 0; i < alias_count; ++i) {
        if (strcmp(alias_list[i].name, name) == 0) {
            free(alias_list[i].command);
            alias_list[i].command = command_no_quotes;
            return;
        }
    }
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
    if (alias_count == 0) {
        printf("Alias list is empty\n");
    } else {
        for (int i = 0; i < alias_count; ++i) {
            printf("%s: %s\n", alias_list[i].name, alias_list[i].command);
        }
    }
}

void alias_remove(char *arg) {
    for (int i = 0; i < alias_count; ++i) {
        if (strcmp(alias_list[i].name, arg) == 0) {
            free(alias_list[i].name);
            free(alias_list[i].command);
            --alias_count;
        }
    }
}

void alias_remove_all() {
    for (int i = 0; i < alias_count; ++i) {
        free(alias_list[i].name);
        free(alias_list[i].command);
    }
    alias_count = 0;
}

void execute_pipeline(char *line) {
    char *commands[MAX_PIPE_CMDS];
    int cmd_count = 0;
    char *token = strtok(line, "|");
    while (token != NULL && cmd_count < MAX_PIPE_CMDS) {
        commands[cmd_count++] = token;
        token = strtok(NULL, "|");
    }
    int pipes[2 * (cmd_count - 1)];
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes + i * 2) < 0) {
            print_error();
            return;
        }
    }
    for (int i = 0; i < cmd_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            setpgid(0, 0);
            tcsetpgrp(STDIN_FILENO, getpid());
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);

            if (i > 0) dup2(pipes[(i - 1) * 2], STDIN_FILENO);
            if (i < cmd_count - 1) dup2(pipes[i * 2 + 1], STDOUT_FILENO);
            for (int j = 0; j < 2 * (cmd_count - 1); j++) close(pipes[j]);
            execute_command(commands[i]);
            exit(0);
        }
    }
    for (int i = 0; i < 2 * (cmd_count - 1); i++) close(pipes[i]);
    for (int i = 0; i < cmd_count; i++) wait(NULL);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}

void execute_command(char *cmd) {
    char *args[MAX_ARGS];
    int arg_idx = 0;
    char *input_file = NULL, *output_file = NULL;
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

    char *token = strtok(cmd, " \t\n");
    while (token != NULL && arg_idx < MAX_ARGS - 1) {
        args[arg_idx++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_idx] = NULL;
    if (!args[0]) return;

    // Handle built-ins like cd, exit, etc...
    // [Trimmed for brevity; continues handling built-in cases and path logic]

    pid_t pid = fork();
    if (pid == 0) {
        setpgid(0, 0);
        tcsetpgrp(STDIN_FILENO, getpid());
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) { print_error(); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (output_file) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { print_error(); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        for (int i = 0; i < path_count; i++) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", custom_paths[i], args[0]);
            execv(full_path, args);
        }
        print_error();
        exit(1);
    } else {
        setpgid(pid, pid);
        tcsetpgrp(STDIN_FILENO, pid);
        wait(NULL);
        tcsetpgrp(STDIN_FILENO, shell_pgid);
    }
}

void process_line(char *line) {
    add_history(line);
    char *command = strtok(line, ";");
    while (command != NULL) {
        if (strchr(command, '|')) execute_pipeline(command);
        else execute_command(command);
        command = strtok(NULL, ";");
    }
}

int main(int argc, char *argv[]) {
    FILE *input = stdin;
    char line[MAX_LINE];

    init_shell(); // Initialize terminal handling and signal setup

    char *env_path = getenv("PATH");
    char *tok = strtok(env_path, ":");
    while (tok && path_count < MAX_PATHS) {
        custom_paths[path_count++] = strdup(tok);
        tok = strtok(NULL, ":");
    }

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) { print_error(); exit(1); }
    } else if (argc > 2) {
        print_error(); exit(1);
    }

    while (1) {
        if (input == stdin) printf("prompt> ");
        if (!fgets(line, MAX_LINE, input)) break;
        if (input != stdin) printf("%s", line);
        process_line(line);
    }

    if (input != stdin) fclose(input);
    clear_history();
    for (int i = 0; i < path_count; i++) free(custom_paths[i]);
    return 0;
}
