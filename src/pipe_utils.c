#include "pipe_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

void error_and_exit(const char *msg) {
  perror(msg);
  exit(1);
}

void parse_command(char *cmd, char **argv) {
  int i = 0;
  char *token = strtok(cmd, " \t\n");
  while (token != NULL && i < MAX_ARGS - 1) {
    argv[i++] = token;
    token = strtok(NULL, " \t\n");
  }
  argv[i] = NULL;
}

int run_pipeline(char *cmds[][MAX_ARGS], int num_cmds) {
  int i;
  int pipefd[MAX_CMDS - 1][2];

  // Create pipes
  for (i = 0; i < num_cmds - 1; i++) {
    if (pipe(pipefd[i]) == -1)
      error_and_exit("pipe");
  }

  // Fork each command
  for (i = 0; i < num_cmds; i++) {
    pid_t pid = fork();
    if (pid < 0)
      error_and_exit("fork");

    if (pid == 0) {
      // CHILD PROCESS

      // If not first command -> read from previous pipe
      if (i > 0) {
        dup2(pipefd[i - 1][0], STDIN_FILENO);
      }

      // If not last command -> write into next pipe
      if (i < num_cmds - 1) {
        dup2(pipefd[i][1], STDOUT_FILENO);
      }

      // Close all pipe fds in child
      for (int j = 0; j < num_cmds - 1; j++) {
        close(pipefd[j][0]);
        close(pipefd[j][1]);
      }

      execvp(cmds[i][0], cmds[i]);
      error_and_exit("execvp");
    }
  }

  // PARENT closes all pipes
  for (i = 0; i < num_cmds - 1; i++) {
    close(pipefd[i][0]);
    close(pipefd[i][1]);
  }

  // Wait for all children
  for (i = 0; i < num_cmds; i++) {
    wait(NULL);
  }

  return 0;
}
