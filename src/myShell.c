#include "myShell.h"

char history[MAX_HISTORY][MAX_INPUT];
int history_count = 0;

/* ——————————————————————————————————————————————————————————————
 * > parse_input
 * Breaks user input into command + arguments.
 * —————————————————————————————————————————————————————————————— */
void parse_input(char *input, char **args) {
    int i = 0;

    args[i] = strtok(input, " ");

    // Keep extracting tokens until no more arguments exist
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " ");
    }
}


/* ——————————————————————————————————————————————————————————————
 * > handle_redirection
 * Scans args for redirection tokens and redirects file (args[i+1]) descriptors
 * token ">": (output) opens file and redirects stdout using dup2()
 * token "<": (input)  opens file and redirects stdin  using dup2()
 * —————————————————————————————————————————————————————————————— */
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {

        // Output redirection: >
        if (strcmp(args[i], ">") == 0) {
            // open file for writing - create file it doesn't exist - truncate file if it already exists 
            // 0644 -> read write permission for file owner
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("output redirection error");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);  // replace stdout with file
            close(fd);

            // remove '>' and filename from args
            args[i] = NULL;
            args[i + 1] = NULL;
            break;
        }

        // Input redirection: 
        if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i + 1], O_RDONLY);
            if (fd == -1) {
                perror("input redirection error");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);   // replace stdin with file
            close(fd);

            // remove '<' and filename from args
            args[i] = NULL;
            args[i + 1] = NULL;
            break;
        }
    }
}


/* ——————————————————————————————————————————————————————————————
 * > execute_pipe
 * Handles piped commands by splitting args at '|'.
 * Left command writes to pipe, right command reads from it.
 * Uses two forks and dup2() to connect them.
 * —————————————————————————————————————————————————————————————— */
void execute_pipe(char **args) {
    // split args into left and right at '|'
    char *left[MAX_ARGS];
    char *right[MAX_ARGS];
    int i = 0, j = 0;
    int pipe_index = -1;

    // find the '|' position
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }

    // fill left command (everything before '|')
    for (i = 0; i < pipe_index; i++) {
        left[i] = args[i];
    }
    left[pipe_index] = NULL;

    // fill right command (everything after '|')
    for (i = pipe_index + 1, j = 0; args[i] != NULL; i++, j++) {
        right[j] = args[i];
    }
    right[j] = NULL;

    // create the pipe
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe failed");
        return;
    }

    // fork child 1: runs left command, writes into pipe
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork failed");
        return;
    }

    if (pid1 == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        dup2(pipe_fd[1], STDOUT_FILENO);  // stdout -> pipe write end
        close(pipe_fd[0]);                // child 1 doesn't need read end
        close(pipe_fd[1]);

        handle_redirection(left);
        if (execvp(left[0], left) == -1) {
            printf("Command not found: %s\n", left[0]);
            exit(1);
        }
    }

    // fork child 2: runs right command, reads from pipe
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork failed");
        return;
    }

    if (pid2 == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        dup2(pipe_fd[0], STDIN_FILENO);   // stdin -> pipe read end
        close(pipe_fd[1]);                // child 2 doesn't need write end
        close(pipe_fd[0]);

        handle_redirection(right);
        if (execvp(right[0], right) == -1) {
            printf("Command not found: %s\n", right[0]);
            exit(1);
        }
    }

    // parent closes both ends and waits for both children
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

/* ——————————————————————————————————————————————————————————————
 * > execute_command
 * Runs external programs using fork + execvp.
 * Handles both foreground and background execution.
 * —————————————————————————————————————————————————————————————— */
void execute_command(char **args, int background) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return;
    }

    // CHILD PROCESS: replaces itself with requested program
    if (pid == 0) {
        // reset signals back to default so the child can be killed
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        handle_redirection(args);

        // execvp searches command in system PATH
        if (execvp(args[0], args) == -1) {
            printf("Command not found\n");
        }

        // Only reached if exec fails
        exit(1);
    }

    // PARENT PROCESS: decides whether to wait or not
    if (!background) {
        wait(NULL);   // foreground execution: wait for child
    } else {
        // background execution: do not block shell
        printf("Background PID: %d\n", pid);
    }
}


/* ——————————————————————————————————————————————————————————————
 * > handle_builtin_commands
 * Processes commands that do not require external programs.
 * —————————————————————————————————————————————————————————————— */
int handle_builtin_commands(char **args) {

    // exit: terminate shell immediately
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // cd: change directory (requires special handling)
    if (strcmp(args[0], "cd") == 0) {

        if (args[1] == NULL) {
            printf("cd: missing argument\n");
        }
        else if (chdir(args[1]) != 0) {
            perror("cd error");
        }

        return 1;
    }

    // pwd: print current working directory
    if (strcmp(args[0], "pwd") == 0) {

        char cwd[1024];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd error");
        }

        return 1;
    }

    // history: prints all previous commands
    if (strcmp(args[0], "history") == 0) {

        for (int i = 0; i < history_count; i++) {
            printf("%d  %s\n", i + 1, history[i]);
        }

        return 1;
    }

    return 0; // not a built-in command
}


/* ——————————————————————————————————————————————————————————————
 * > main
 * Reads input, parses it, and executes commands.
 * —————————————————————————————————————————————————————————————— */
int main() {
    signal(SIGINT, SIG_IGN);   // shell ignores Ctrl+C
    signal(SIGTSTP, SIG_IGN);  // shell ignores Ctrl+Z

    char input[MAX_INPUT];

    while (1) {

        char cwd[1024];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("[%s] myShell> ", cwd);
        } else {
            printf("[unknown] myShell> ");
        }
        fflush(stdout); // Force print to avoid buffer

        if (fgets(input, MAX_INPUT, stdin) == NULL) { // Ctrl+D : Instead of killing input -> print newline
            printf("\n");
            break;
        }

        // remove trailing newline for clean parsing
        input[strcspn(input, "\n")] = 0;

        char *args[MAX_ARGS];

        parse_input(input, args);


        if (args[0] == NULL) continue; // ignore empty input

        // save command to history
        if (history_count < MAX_HISTORY) {
            strcpy(history[history_count], input);
            history_count++;
        }

        int background = 0;

        // detect '&' for background execution
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "&") == 0) {
                background = 1;

                // remove '&' so execvp doesn't receive it
                args[i] = NULL;
                break;
            }
        }

        // check built-in commands first (no fork needed)
        if (handle_builtin_commands(args)) {
            continue;
        }

        // check for pipe before executing
        int is_pipe = 0;
        for (int i = 0; args[i] != NULL; i++) {
         if (strcmp(args[i], "|") == 0) {
              is_pipe = 1;
              break;
          }
        }

        if (is_pipe) {
           execute_pipe(args);
           continue;
        }
        
        // execute external command
        execute_command(args, background);
    }

    return 0;
}