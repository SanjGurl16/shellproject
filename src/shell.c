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
#define TOKEN_SEP " \t" 
#define MAX_CMD_ARGS MAX_ARGS

// shell.c

// Read Input
// Tokenization included; strtok

int sh_execute_pipeline_line(char *line) {
   char *input_file = NULL;
   char *output_file = NULL;
   int append = 0;

     // Detect redirection
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

    // Trim whitespace from filenames
    if (input_file) while (*input_file == ' ') input_file++;
    if (output_file) while (*output_file == ' ') output_file++;

    // Split pipeline
    char *cmd_strings[MAX_CMDS];
    int num_cmds = 0;
    char *token = strtok(line, "|");
    while (token != NULL && num_cmds < MAX_CMDS) {
        while (*token == ' ') token++; // trim leading spaces
        cmd_strings[num_cmds++] = token;
        token = strtok(NULL, "|");
    }

    char *cmds[MAX_CMDS][MAX_ARGS];
    for (int i = 0; i < num_cmds; i++)
        parse_command(cmd_strings[i], cmds[i]);

    return run_pipeline(cmds, num_cmds, input_file, output_file, append);
 }

int sh_read(char *input, char **args) {
   int i = 0;
   char *token = strtok(input, TOKEN_SEP);
   while (token != NULL && i < (MAX_ARGS - 1)) {
      args[i++] = token;
      token = strtok(NULL, TOKEN_SEP);
   }
   args[i] = NULL;
   return i; 
}

//Execute
int sh_execute(char **args) {
    if (args[0] == NULL) return 1; // empty command

    
    size_t count;
    struct builtin *b = get_builtins(&count);

    for (size_t i = 0; i < count; i++) {
       if (strcmp(args[0], b[i].name) == 0) {
         return b[i].handler(args);   
       }
    }  

    // Check for simple redirection in single commands
    char *input_file = NULL;
    char *output_file = NULL;
    int append = 0;

    for (int i = 0; args[i] != NULL; i++) {
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

    pid_t pid = fork();
    if (pid == 0) { // child
        // Input redirection
        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) { perror("open input"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // Output redirection
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
    } else if (pid > 0) { // parent
        int status;
        waitpid(pid, &status, 0);
        return 1;
    } else {
        perror("fork");
        return 1;
    }
}

// Detect if the line contains a pipeline
int contains_pipe(char *line) {
    return strchr(line, '|') != NULL;
}
 

int main(void) {  
  char *line = NULL;
  size_t len = 0;
  char *args[MAX_ARGS];
 
 
  while (1) {  
// read step
   printf("%s", PROMPT);
   fflush(stdout);
  
   ssize_t read = getline(&line, &len, stdin);

   if (read == -1) {
	 printf("\n");
	 break;
  }
  // Remove the newline 
  line[strcspn(line, "\n")] = '\0';
  if (strlen(line) == 0) continue;

  if (contains_pipe(line)) {
    sh_execute_pipeline_line(line);  
  
  } else {
   sh_read(line, args);
   sh_execute(args);

  }
}

  free(line);
  return 0; 

}
