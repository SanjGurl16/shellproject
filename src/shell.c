#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "builtins.h"
#include <fcntl.h>


#define PROMPT "$ "
#define MAX_ARGS 1024
#define TOKEN_SEP " \t" 
#define MAX_CMDS 32
#define MAX_CMD_ARGS MAX_ARGS

// shell.c

// Read Input
// Tokenization included; strtok

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

int is_parent_builtin(const char *cmd) {
   return (strcmp(cmd, "cd") == 0 || strcmp(cmd, "exit") == 0);
}

// new addition
struct command {
    char *argv[MAX_CMD_ARGS];
    char *in_file;
    char *out_file;
    char *append_file;
};


//Execute
int sh_execute(char *cmd, char **cmd_args) {
    if (args[0] == NULL)
	    return 1;	
    
    size_t count;
    struct builtin *table = get_builtins(&count);

    /* Check for built-in commands first */
    char *infile = NULL;
    char *outfile = NULL;
    int append = 0;


    /* Scan for redirection before checking builtins */
    for (int i = 0; args[i]; i++) {
         if (strcmp(args[i], "<") == 0 && args[i+1]) {
	     infile = args[i+1];
	     args[i] = NULL;
	     break;
	 }
	 if (strcmp(args[i], ">") == 0 && args[i+1]) {
	     outfile = args[i+1];
	     append = 0;
	     args[i] = NULL;
	     break;
	 }
        if (strcmp(args[i], ">>") == 0 && args[i+1]) {
	    outfile = args[i+1];
	    append = 1;
	    args[i] = NULL;
	    break;
	}
    }

  /* Check for builtins */
    for(size_t j = 0; j < count; j++) {
      if (strcmp(args[0], table[j].name) == 0) {
          int saved_stdin = -1, saved_stdout = -1;
  
	  // Input redirection
	  if (infile) {
	     int fd = open(infile, O_RDONLY);
	     if (fd < 0) { perror(infile); return 1; }
	     saved_stdin = dup(0);
	     dup2(fd, 0);
	     close(fd);
	  }
          //Output redirection
	  if (outfile) {
	     int fd;
	     if (append)
		 fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
	     else 
		 fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	     if (fd < 0) { perror(outfile); return 1; }
	     saved_stdout = dup(1);
	     dup2(fd, 1);
	     close(fd);
	  }
          int result = table[j].func(args);

	  // Restore STDIN/STDOUT
	  if (saved_stdin != -1) {
	     dup2(saved_stdin, 0);
	     close(saved_stdin);
	  }
	  if (saved_stdout != -1) {
	     dup2(saved_stdout, 1);
	     close(saved_stdout);
	  }
	  return result;
      }
        
    }

    pid_t pid = fork();
    int status;

    if (pid == 0) { // child
        execvp(args[0], args);
	perror(args[0]);
	exit(1);
    } else if (pid > 0 ) {
     int status;
     waitpid(pid, &status, 0);
     return 1;
   }else {
      perror("fork");
      return 1;
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

  int args_read = sh_read(line, args);

  if (args_read == 0) 
       continue;

     sh_execute(args[0], args);
  }
  free(line);
  return 0; 

}
