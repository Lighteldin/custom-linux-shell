#include "myShell.h"

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

    return 0; // not a built-in command
}


/* ——————————————————————————————————————————————————————————————
 * > main
 * Reads input, parses it, and executes commands.
 * —————————————————————————————————————————————————————————————— */
int main() {
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

        // execute external command
        execute_command(args, background);
    }

    return 0;
}