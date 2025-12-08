#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "builtins.h"
#include <fcntl.h>
#include "pipe_utils.h"


#define PROMPT "$ "
#define MAX_LINE 2048

// shell.c

// Read Input
// Tokenization included; strtok

// ----------------------
// Helper Functions
// ----------------------

// Trim leading and trailing whitespace
void trim(char **str) {
  while (**str == ' ' || **str == '\t') (*str)++;
  char *end = *str + strlen(*str) - 1;
  while (end > *str && (*end == ' ' || *end == '\t' || *end == '\n')) {
    *end = '\0';
    end--;
  }
}

// Trim trailing whitespace only (for filenames in redirection)
void trim_trailing(char *str) {
  if (!str) return;
  char *end = str + strlen(str) - 1;
  while (end > str && (*end == ' ' || *end == '\t')) {
    *end = '\0';
    end--;
  }
}

// Check if a command is a parent-handled builtin (cd or exit)
int is_parent_builtin(const char *cmd) {
  return (strcmp(cmd, "cd") == 0 || strcmp(cmd, "exit") == 0);
}

// Execute a single command (with optional redirection)
int execute_single(char **args) {
  if (!args[0]) return 1;

  // Parent-handled builtins
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

 // Redirection
 char *input_file = NULL;
 char *output_file = NULL;
 int append = 0;

 for (int i = 0; args[i]; i++) {
    if (strcmp(args[i], "<") == 0) {
      input_file = args[i + 1];
      args[i] = NULL;
      i++;
    } else if (strcmp(args[i], ">") == 0) {
      output_file = args[i + 1];
      append = 0;
      args[i] = NULL;
      i++;
    } else if (strcmp(args[i], ">>") == 0) {
      output_file = args[i + 1];
      append = 1;
      args[i] = NULL;
      i++;
    }
 }

 trim_trailing(input_file);
 trim_trailing(output_file);

 pid_t pid = fork();
 if (pid == 0) {
   // Child process
   if (input_file) {
     int fd = open(input_file, O_RDONLY);
     if (fd < 0) { perror("open input"); exit(1); }
     dup2(fd, STDIN_FILENO);
     close(fd);
   }
   if (output_file) {
     int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
     int fd = open(output_file, flags, 0644);
     if (fd < 0) { perror("open output"); exit(1); }
     dup2(fd, STDOUT_FILENO);
     close(fd);
   }
   execvp(args[0], args);
   perror(args[0]);
   exit(1);
 } else if (pid > 0) {
   int status;
   waitpid(pid, &status, 0);
   return 1;
 } else {
   perror("fork");
   return 1;
 }
}

// Execute a pipeline line
int execute_pipeline_line(char *line) {
    char *input_file = NULL;
    char *output_file = NULL;
    int append = 0;

    char *redir;
    if ((redir = strstr(line, ">>"))) {
      *redir = '\0';
      output_file = redir + 2;
      append = 1;
    } else if ((redir = strstr(line, ">"))) {
      *redir = '\0';
      output_file = redir + 1;
    }
    if ((redir = strstr(line, "<"))) {
      *redir = '\0';
      input_file = redir + 1;
    }

    if (input_file) trim_trailing(input_file);
    if (output_file) trim_trailing(output_file);

    char *cmd_strings[MAX_CMDS];
    int num_cmds = 0;
    char *token = strtok(line, "|");
    while (token != NULL && num_cmds < MAX_CMDS) {
      trim(&token);
      if (strlen(token) > 0) cmd_strings[num_cmds++] = token;
      token = strtok(NULL, "|");
    }

    char *cmds[MAX_CMDS][MAX_ARGS];
    for (int i = 0; i < num_cmds; i++)
      parse_command(cmd_strings[i], cmds[i]);

    return run_pipeline(cmds, num_cmds, input_file, output_file, append);
}

// Check if the line contains a pipeline
int contains_pipe(char *line) {
  return strchr(line, '|') != NULL;
}

// ----------------------
// Main Loop
// ----------------------
int main(void) {
  char *line = NULL;
  size_t len = 0;
  char *args[MAX_ARGS];

  while (1) {
    printf("%s", PROMPT);
    fflush(stdout);

    ssize_t read = getline(&line, &len, stdin);
    if (read == -1) {
      printf("\n");
      break;
    }

    line[strcspn(line, "\n")] = '\0';
    if (strlen(line) == 0) continue;

    if (contains_pipe(line)) {
      execute_pipeline_line(line);
    } else {
      int argc = 0;
      char *token = strtok(line, " \t");
      while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t");
      }
      args[argc] = NULL;

      execute_single(args);
    }
}

free(line);
return 0;
}
