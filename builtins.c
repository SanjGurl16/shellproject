#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Define the builtin tables
char *builtin_str[] = {
  "cd", "echo", "pwd", "env", "help", "exit"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd, &lsh_echo, &lsh_pwd, &lsh_env, &lsh_help, &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

// Builtin function implementations
int lsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

// echo simply prints arguments
int lsh_echo(char **args) {
  int i = 1;
  while (args[i] != NULL) {
    printf("%s ", args[i]);
    i++;
  }
  printf("\n");
  return 1;
}

int lsh_pwd(char **args) {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("lsh");
  }
  return 1;
}

int lsh_env(char **args) {
  extern char **environ;
  for (int i = 0; environ[i] != NULL; i++) {
    printf("%s\n", environ[i]);
  }
  return 1;
}

int lsh_help(char **args) {
  printf("Simple Shell - Builtin commands:\n");
  for (int i = 0; i < lsh_num_builtins(); i++) {
    printf(" %s\n", builtin_str[i]);
  }
  return 1;
}

int lsh_exit(char **args) {
  return 0; // returning 0 tells main loop to exit
}
