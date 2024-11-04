#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE_LENGTH 1024
#define MAX_ARGS 128
char msg_error[25] = "An error has occurred\n";


void print_error() {
    write(STDERR_FILENO, msg_error, strlen(msg_error));
}

void process_command(char *cmd, char **path){
    preprocess_command(cmd);
    char *args[MAX_ARGS];
    int argc = 0;
    char *token = strtok(cmd, " \t\n");
    int redirection = 0;

    while (token != NULL) {
        if (strcmp(token, "&") == 0) {
            if (argc > 0) {
                args[argc] = NULL;
                exec_process(args, path);
                argc = 0;
            }
        } else if (strcmp(token, ">") == 0) {
            if (argc == 0) {
                print_error();
                return;
            }
            output_redirection(token, path, args);
            redirection = 1;
            break;
        } else {
            args[argc++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;

    if (args[0] == NULL) return;

    if (redirection == 0){
        if (!check_builtin(args)) {
            exec_process(args, path);
        } else {
            exec_builtin(args, path);
        }
    }
}

void preprocess_command(char *cmd) {
    char temp_cmd[1024] = {0};
    int j = 0;
    for (int i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == '&' && (i == 0 || cmd[i-1] != ' ') && (cmd[i+1] != ' ' && cmd[i+1] != '\0')) {
            temp_cmd[j++] = ' ';
            temp_cmd[j++] = '&';
            temp_cmd[j++] = ' ';
        } else {
            temp_cmd[j++] = cmd[i];
        }
    }
    temp_cmd[j] = '\0';
    strcpy(cmd, temp_cmd);
}

void output_redirection(char *token, char **path, char **args){
    token = strtok(NULL, " \t\n");
    char *next_token = strtok(NULL, " \t\n");
    int saved_stdout = dup(STDOUT_FILENO);

    if (next_token != NULL &&(strcmp(next_token, "&") != 0) ){
        if (!token || next_token){
            print_error();
            exit(0);
        }
    }

    int fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        print_error();
        close(saved_stdout);
        exit(0);

    }
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    exec_process(args, path);

    close(fd);

    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}

void exec_process(char **args, char **path){
    int status;
    pid_t pid = fork();
    if (pid == 0) {
        char execpath[MAX_LINE_LENGTH];
        int i = 0;

        while (path[i] != NULL) {
            snprintf(execpath, sizeof(execpath), "%s/%s", path[i], args[0]);
            if (execv(execpath, args) == 0){
                exit(0);
            };
            i++;
        }
        print_error();
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, &status, 0);
    } else {
        print_error();
    }
}

int check_builtin(char **args){
    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "path") == 0 || strcmp(args[0], "exit") == 0) {
        return 1;
    }
    return 0;
}

void exec_builtin(char **args, char **path){
    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
            print_error();
            return;
        } 
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        change_cd(args);
    } else if (strcmp(args[0], "path") == 0) {
        append_path(args, path);
    }
}

void append_path(char **args, char **path){
    int i = 1;
    int j = 0;

    while (path[j] != NULL) {
        path[j++] = NULL; 
    }

    j = 0;
    while (args[i] != NULL) {
        path[j++] = strdup(args[i++]);
    }
}

void change_cd(char **args){
    if (args[1] == NULL || args[2] != NULL) {
        print_error();
    } else if (chdir(args[1]) != 0) {
        print_error();
    }
}

int main(int argc, char *argv[]) {
    char *cmd = NULL;
    size_t len = 0;
    FILE *input_stream = stdin;

    if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (!input_stream) {
            print_error();
            exit(1);
        }
    } else if (argc > 2) {
        print_error();
        exit(1);
    }

    char *path[MAX_ARGS] = {"/bin"};

    while (1) {
        if (argc == 1) {
            printf("wish> ");
        }
        if (getline(&cmd, &len, input_stream) == -1) {
            if (argc == 2) fclose(input_stream);
            exit(0);
        }

        process_command(cmd, path);
        free(cmd);
        cmd = NULL;
    }
}

