#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Builtin table
struct builtin builtins[] = {
  { "cd", cmd_cd },
  { "echo", cmd_echo },
  { "pwd", cmd_pwd },
  { "env", cmd_env },
  { "help", cmd_help },
  { "exit", cmd_exit }
}; 

// Return pointer to builtin table + count
struct builtin *get_builtins(size_t *count) {
  if (count) {
    *count = sizeof(builtins) / sizeof(builtins[0]);
  }
  return builtins;
}

// Return number of builtins only
size_t builtin_count() {
  return sizeof(builtins) / sizeof(builtins[0]);
}

// Builtin implementations
int cmd_cd(char **args) {
  if (!args[1]) {
    fprintf(stderr, "cd: missing directory argument\n");
    return 1;
  }
  if (chdir(args[1]) == -1) {
    perror("cd");
  }
  return 1;
}

int cmd_echo(char **args) {
  for (int i = 1; args[i]; i++) {
    if (i > 1) printf(" ");
    fputs(args[i], stdout);
  }
  putchar('\n');
  return 1;
}

int cmd_pwd(char **args) {
  (void)args;
  
  char buffer[1024];
  if (getcwd(buffer, sizeof(buffer)) == NULL) {
    perror("pwd");
  } else {
    puts(buffer);
  }
  return 1;
}

int cmd_env(char **args) {
  (void)args;
  
  extern char **environ;
  for (char **env = environ; *env; env++) {
    puts(*env);
  }
  return 1;
}

int cmd_help(char **args) {
  (void)args;
  
  puts("Available built-ins:");
  for (size_t i = 0; i < builtin_count(); i++) {
    printf(" - %s\n", builtins[i].name);
  }
  return 1;
}

int cmd_exit(char **args) {
  (void)args;
  return 0; // returning 0 tells main loop to exit
}
