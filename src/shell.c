#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

#include "builtins.h"
#include "pipe_utils.h"   // provides MAX_ARGS, MAX_CMDS, parse_command, run_pipeline

#define PROMPT "$ "
#define MAX_LINE 2048

// ------------------------------
// Helpers: trimming
// ------------------------------
static char *ltrim(char *s) {
    if (!s) return s;
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static void rtrim(char *s) {
    if (!s) return;
    char *end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
}

// ------------------------------
// Builtin dispatcher helpers
// ------------------------------
static int call_builtin_if_exists(char **args) {
    if (!args || !args[0]) return -1;

    size_t count = 0;
    struct builtin *bt = get_builtins(&count);
    if (!bt) return -1;

    for (size_t i = 0; i < count; ++i) {
        if (strcmp(args[0], bt[i].name) == 0) {
            return bt[i].handler(args); // builtin executed in parent
        }
    }
    return -1; // not a builtin
}

static int is_parent_builtin(const char *cmd) {
    return (cmd && (strcmp(cmd, "cd") == 0 || strcmp(cmd, "exit") == 0));
}

// ------------------------------
// SIGINT handler
// ------------------------------
static void sigint_handler(int signo) {
    (void)signo;
    write(STDOUT_FILENO, "\n", 1);
    write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
    fflush(stdout);
}

// ------------------------------
// Execute single command (with redirection)
// ------------------------------
static int execute_single(char **args) {
    if (!args || !args[0]) return 1;

    // Parent-only builtins (cd, exit)
    if (is_parent_builtin(args[0])) {
        if (strcmp(args[0], "cd") == 0) {
            if (!args[1]) {
                fprintf(stderr, "cd: missing argument\n");
            } else if (chdir(args[1]) != 0) {
                perror("cd");
            }
        } else if (strcmp(args[0], "exit") == 0) {
            exit(0);
        }
        return 1;
    }

    // Other builtins (echo, pwd, env, help) — run in parent for simplicity
    int bres = call_builtin_if_exists(args);
    if (bres != -1) return bres;

    // Parse simple redirections from args (single-command style)
    char *input_file = NULL;
    char *output_file = NULL;
    int append = 0;

    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "<") == 0) {
            if (!args[i+1]) {
                fprintf(stderr, "redirection: missing input filename\n");
                return 1;
            }
            input_file = args[i+1];
            args[i] = NULL;
            i++;
        } else if (strcmp(args[i], ">>") == 0) {
            if (!args[i+1]) {
                fprintf(stderr, "redirection: missing output filename\n");
                return 1;
            }
            output_file = args[i+1];
            append = 1;
            args[i] = NULL;
            i++;
        } else if (strcmp(args[i], ">") == 0) {
            if (!args[i+1]) {
                fprintf(stderr, "redirection: missing output filename\n");
                return 1;
            }
            output_file = args[i+1];
            append = 0;
            args[i] = NULL;
            i++;
        }
    }

    if (input_file) {
        input_file = ltrim(input_file);
        rtrim(input_file);
    }
    if (output_file) {
        output_file = ltrim(output_file);
        rtrim(output_file);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        // child
        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) { perror("open input"); exit(1); }
            if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2"); close(fd); exit(1); }
            close(fd);
        }
        if (output_file) {
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            int fd = open(output_file, flags, 0644);
            if (fd < 0) { perror("open output"); exit(1); }
            if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); close(fd); exit(1); }
            close(fd);
        }

        execvp(args[0], args);
        perror(args[0]);
        exit(127);
    } else {
        // parent
        int status = 0;
        waitpid(pid, &status, 0);
        return 1;
    }
}

// ------------------------------
// Execute pipeline line (N commands, optional <, >, >>)
// ------------------------------
static int execute_pipeline_line(char *line) {
    if (!line) return 1;

    // Make a working copy (we'll modify it)
    char buf[MAX_LINE];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    char *input_file = NULL;
    char *output_file = NULL;
    int append = 0;

    // Parse '>>' first
    char *p;
    if ((p = strstr(buf, ">>"))) {
        *p = '\0';
        output_file = p + 2;
        append = 1;
    } else if ((p = strstr(buf, ">"))) {
        *p = '\0';
        output_file = p + 1;
    }
    // Parse input redirection '<'
    if ((p = strstr(buf, "<"))) {
        *p = '\0';
        input_file = p + 1;
    }

    if (input_file) { input_file = ltrim(input_file); rtrim(input_file); }
    if (output_file) { output_file = ltrim(output_file); rtrim(output_file); }

    if ((input_file && strlen(input_file) == 0) ||
        (output_file && strlen(output_file) == 0)) {
        fprintf(stderr, "redirection: missing filename\n");
        return 1;
    }

    // Split pipeline by '|'
    char *saveptr = NULL;
    char *tokens[MAX_CMDS];
    int n = 0;

    char *tok = strtok_r(buf, "|", &saveptr);
    while (tok && n < MAX_CMDS) {
        tok = ltrim(tok);
        rtrim(tok);
        if (strlen(tok) > 0) tokens[n++] = tok; // skip empty segments
        tok = strtok_r(NULL, "|", &saveptr);
    }

    if (n == 0) {
        fprintf(stderr, "empty pipeline\n");
        return 1;
    }

    // If pipeline is one command, prefer running builtin in parent
    if (n == 1) {
        // parse into argv using existing helper
        char *argv[MAX_ARGS];
        parse_command(tokens[0], argv);
        if (argv[0] == NULL) return 1;

        // parent-only builtins should run in parent
        if (is_parent_builtin(argv[0])) {
            if (strcmp(argv[0], "cd") == 0) {
                if (!argv[1]) fprintf(stderr, "cd: missing argument\n");
                else if (chdir(argv[1]) != 0) perror("cd");
            } else if (strcmp(argv[0], "exit") == 0) {
                exit(0);
            }
            return 1;
        }

        // other builtins in parent
        int bres = call_builtin_if_exists(argv);
        if (bres != -1) return bres;

        // if not builtin, run it with redirection support using execute_single
        // but need to place redirection tokens into argv appropriately:
        // if redirections were in the original line they were removed above; however
        // user might have provided them as part of tokens[0] — parse_command will include them
        return execute_single(argv);
    }

    // For N>1: build cmds[][] and call run_pipeline
    char *cmds[MAX_CMDS][MAX_ARGS];
    for (int i = 0; i < n; ++i) {
        parse_command(tokens[i], cmds[i]); // parse_command uses strtok on the token
        if (cmds[i][0] == NULL) {
            fprintf(stderr, "empty command in pipeline\n");
            return 1;
        }
        // disallow parent-only builtins inside pipelines
        if (is_parent_builtin(cmds[i][0])) {
            fprintf(stderr, "error: builtin '%s' cannot be used in a pipeline\n", cmds[i][0]);
            return 1;
        }
    }

    return run_pipeline(cmds, n, input_file, output_file, append);
}

// ------------------------------
// Utility
// ------------------------------
static int contains_pipe(const char *line) {
    return line && strchr(line, '|') != NULL;
}

// ------------------------------
// Main loop
// ------------------------------
int main(void) {
    // Install SIGINT handler so Ctrl+C doesn't kill shell
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;
    char *argv[MAX_ARGS];

    while (1) {
        printf("%s", PROMPT);
        fflush(stdout);

        nread = getline(&line, &cap, stdin);
        if (nread == -1) {
            // EOF (Ctrl+D)
            printf("\n");
            break;
        }

        // remove trailing newline
        line[strcspn(line, "\n")] = '\0';

        // skip empty
        char *ptr = ltrim(line);
        if (*ptr == '\0') continue;

        if (contains_pipe(ptr)) {
            execute_pipeline_line(ptr);
        } else {
            // tokenize into argv (simple whitespace split)
            int argc = 0;
            char *save = NULL;
            char *t = strtok_r(ptr, " \t", &save);
            while (t && argc < MAX_ARGS - 1) {
                argv[argc++] = t;
                t = strtok_r(NULL, " \t", &save);
            }
            argv[argc] = NULL;

            execute_single(argv);
        }
    }

    free(line);
    return 0;
}
