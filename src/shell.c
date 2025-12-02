#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "builtins.h"

#define PROMPT "$ "
#define MAX_ARGS 1024
#define TOKEN_SEP " \t"


// Read Input
// Tokenization included; strtok

int s_read(char *input, char **args) {
   int i = 0;
   char *token = strtok(input, TOKEN_SEP);
   while (token != NULL && i < (MAX_ARGS - 1)) {
      args[i++] = token;
      token = strtok(NULL, TOKEN_SEP);
   }
   args[i] = NULL;
   return i; 
}

//Execute Helper
int s_execute(char *cmd, char **cmd_args) {
   
   pid_t pid = fork();
   int status;
   
   if (pid < 0) {
      perror("fork");
      return -1;

   }

   if (pid == 0) {
    execvp(cmd, cmd_args);
    perror(cmd);
   } else {

   // parent: wait for child
   if (waitpid (pid, &status, 0) != pid) {
     perror("waitpid");
     return -1;
   }
 }
 return status;
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

  int args_read = s_read(line, args);

  if (args_read == 0) 
       continue;

     s_execute(args[0], args);
  }
  free(line);
  return 0; 

}
