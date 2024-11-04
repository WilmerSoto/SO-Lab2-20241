#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 1024
#define MAX_PATHS 100
#define ERROR_MESSAGE "An error has occurred\n"

char *path[MAX_PATHS];
int path_count = 1;

void print_error() {
    write(STDERR_FILENO, ERROR_MESSAGE, strlen(ERROR_MESSAGE));
}

void init_path() {
    path[0] = "/bin";
}

void update_path(char **args) {
    for (int i = 0; i < path_count; i++) {
        free(path[i]);
    }
    path_count = 0;
    for (int i = 1; args[i] != NULL; i++) {
        path[path_count] = strdup(args[i]);
        path_count++;
    }
}

void parse_command(char *line, char **args) {
    int i = 0;
    args[i] = strtok(line, " \n");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \n");
    }
}

void execute_command(char **args) {
    char *command = args[0];
    int redirection = 0;
    char *file = NULL;
    int i = 0;
    
    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            redirection = 1;
            args[i] = NULL;
            if (args[i+1] != NULL && args[i+2] == NULL) {
                file = args[i+1];
            } else {
                print_error();
                return;
            }
        }
        i++;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
    } else if (pid == 0) {
        if (redirection) {
            int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
                print_error();
                exit(1);
            }
            close(fd);
        }

        for (int j = 0; j < path_count; j++) {
            char full_path[MAX_LINE_LENGTH];
            snprintf(full_path, sizeof(full_path), "%s/%s", path[j], command);
            if (access(full_path, X_OK) == 0) {
                execv(full_path, args);
            }
        }
        print_error();
        exit(1);
    } else {
        wait(NULL);
    }
}

int handle_builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
            print_error();
        } else {
            exit(0);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL || chdir(args[1]) != 0) {
            print_error();
        }
        return 1;
    } else if (strcmp(args[0], "path") == 0) {
        update_path(args);
        return 1;
    }
    return 0;
}

void execute_parallel(char *line) {
    char *command = strtok(line, "&");
    pid_t pids[MAX_LINE_LENGTH];
    int num_cmds = 0;

    while (command != NULL) {
        char *args[MAX_LINE_LENGTH / 2 + 1];
        parse_command(command, args);

        if (args[0] != NULL && !handle_builtin(args)) {
            pids[num_cmds] = fork();
            if (pids[num_cmds] == 0) {
                execute_command(args);
                exit(0);
            }
            num_cmds++;
        }
        command = strtok(NULL, "&");
    }

    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

int main(int argc, char *argv[]) {
    char line[MAX_LINE_LENGTH];
    char *args[MAX_LINE_LENGTH / 2 + 1];
    FILE *input = stdin;

    if (argc > 2) {
        print_error();
        exit(1);
    }

    init_path();

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            print_error();
            exit(1);
        }
    }

    while (1) {
        if (argc == 1) {
            printf("wish> ");
        }

        if (fgets(line, MAX_LINE_LENGTH, input) == NULL) {
            break;
        }

        if (strchr(line, '&')) {
            execute_parallel(line);
        } else {
            parse_command(line, args);
            if (args[0] != NULL && !handle_builtin(args)) {
                execute_command(args);
            }
        }
    }

    if (argc == 2) {
        fclose(input);
    }

    return 0;
}
