#ifndef BUILTINS_H
#define BUILTINS_H

// Function declarations
int lsh_cd(char **args);
int lsh_echo(char **args);
int lsh_pwd(char **args);
int lsh_env(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

// Expose builtin command table
extern char *builtin_str[];
extern int (*builtin_func[])(char **);
int lsh_num_builtins();

#endif
