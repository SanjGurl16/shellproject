#include <stdio.h>
#include <string.h>
#include "pipe_utils.h"

#define MAX_LINE 2048

int main() {
  char line[MAX_LINE];

  printf("Enter a pipeline (cmd1 | cmd2 | ...): ");
  fgets(line, sizeof(line), stdin);

  char *cmd_strings[MAX_CMDS];
  int num_cmds = 0;

  // Split around '|'
  char *token = strtok(line, "|");
  while (token != NULL && num_cmds < MAX_CMDS) {
    cmd_strings[num_cmds++] = token;
    token = strtok(NULL, "|");
  }

  if (num_cmds == 0 || num_cmds == 1) {
    fprintf(stderr, "Error: you must provide at least one pipe, e.g. ls | wc\n");
    return 1;
  }

  // Convert each command string to argv[]
  char *cmds[MAX_CMDS][MAX_ARGS];

  for (int i = 0; i < num_cmds; i++) {
    // Trim whitespace
    while (*cmd_strings[i] == ' ')
      cmd_strings[i]++;

    parse_command(cmd_strings[i], cmds[i]);
  }

  // Run the full pipeline
  return run_pipeline(cmds, num_cmds);
}
