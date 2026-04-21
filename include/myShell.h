#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

/* ———————————————————————————
 * Configuration constants
 * ——————————————————————————— */
#define MAX_INPUT 100
#define MAX_ARGS 30
#define MAX_HISTORY 100

/* ———————————————————————————
 * Function declarations
 * ——————————————————————————— */

// Splits input string into arguments
void parse_input(char *input, char **args);

// Scans args for redirection tokens and redirects file descriptors
void handle_redirection(char **args);

//
void execute_pipe(char **args);

// Executes external commands (fork + exec)
void execute_command(char **args, int background);

// Handles built-in commands (cd, pwd, exit)
int handle_builtin_commands(char **args);

#endif