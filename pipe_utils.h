#ifndef PIPE_UTILS_H
#define PIPE_UTILS_H

// Max arguments per command
#define MAX_ARGS 100

// Max number of commands in a pipeline
#define MAX_CMDS 20

// Parses a string into argv tokens 
void parse_command(char *cmd, char **argv);

// Runs a pipeline of N commands
int run_pipeline(char *cmds[][MAX_ARGS], int num_cmds);

#endif
