#ifndef BUILTINS_H
#define BUILTINS_H

#include <stddef.h>

// Structure describing a builtin command
struct builtin {
  const char *name;
  int (*handler)(char **);
};

// Accessors for builtin table
struct builtin *get_builtins(size_t *count);
size_t builtin_count(void);

// Builtin command handlers
int cmd_cd(char **args);
int cmd_echo(char **args);
int cmd_pwd(char **args);
int cmd_env(char **args);
int cmd_help(char **args);
int cmd_exit(char **args);

#endif
