#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef enum {APPEND, OVERWRITE} appendMode;

typedef struct {
  char* cmd;
  char** args;
  bool rdrstdout;
  bool rdrstdin;
  bool file2stdin;
  bool stdout2file;
  char* infname;
  char* outfname;
  appendMode outfmode;
} Command;

typedef struct {
  Command* cmd;
  int length;
} Pipeline;

void shell_loop();
char* shell_getline();
Command* shell_splitLine(char*);
char** split_on_token(char*, char*);
char* helper_get_token_after(char*, char*);
char* trim(char*);

int main() {
  printf("Welcome to mShell\n");

  shell_loop();
  return 0;
}


void shell_loop() {
  //print the prompt
  printf("> ");

  //get the data from the line
  char* line = shell_getline();
  printf(line);

  //pase the line
  shell_splitLine(line);
}

char* shell_getline() {
  //we set this to null so that it is free-able
  char *line = NULL;

  //getline auto alocates buffers
  size_t bufsize = 0;
  getline(&line, &bufsize, stdin);
  return line;
}

Command* shell_splitLine(char* line) {
  Command* cmds = NULL;
  Command command;
  printf("Got line: %s", line);

  //seperate the pipelines from each other
  //this way multiple lines split by ';' work
  char** pipelines = split_on_token(line, ";");

  int i;
  for(i=0; pipelines[i] != NULL; i++) {
    char** executables = NULL;
    char** args = NULL;
    
    //check if this command is anything more than a newline
    if(pipelines[i][0] == '\n') {
      //was just a newline, continue with no action
      continue;
    }
    //first we work on individual jobs
    printf("pipeline: %s\n", pipelines[i]);

    //get a list of executable portions of the pipeline
    executables = split_on_token(pipelines[i], "|");

    //at this point we can iterate on executables and process
    //specific options for redirection and options handling
    int j;
    for(j=0; executables[j] != NULL; j++) {
      printf("Exec %i of pipeline %i: %s\n", j, i, executables[j]);

      //we now need to know if there's redirection going on
      if(strstr(executables[j], ">") != NULL) {
	//output will create/overwrite a file
	command.rdrstdout = true;
	command.outfmode = OVERWRITE;
	command.outfname = helper_get_token_after(executables[j], ">");
      } else if(strstr(executables[j], ">>") != NULL) {
	//output will create/append a file
	command.rdrstdout = true;
	command.outfmode = APPEND;
	command.outfname = helper_get_token_after(executables[j], ">>");
      }

      if(strstr(executables[j], "<") != NULL) {
	//input will implicitly come from a file
	command.rdrstdin = true;
	command.infname = helper_get_token_after(executables[j], "<");
      }

      //at this point we can finally tokenize the arguments
      args = split_on_token(executables[j], " ");
      command.cmd = args[0];

      char** argsTmp = args;;
      char* arg = NULL;
      int bufsize = 64;
      command.args = malloc(bufsize * sizeof(char*));
      int k;
      for(k=0; args[k] != NULL; k++) {
	printf("outer loop\n");
	if(k>bufsize) {
	  command.args = realloc(command.args, bufsize * sizeof(char*));
	  if(!command.args) {
	    fprintf(stderr, "mShell: Couldn't expand argument buffer");
	  }
	}
	
	if(strstr(argsTmp[k], "><") != NULL) {
	  //we're at a redirection argument, fast forward
	  argsTmp = argsTmp+2;
	} else {
	  command.args[k] = arg;
	}
      }
    }
  }
  return cmds;
}

char** split_on_token(char* line, char* stoken) {
  int bufsize = 64, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "mShell: Couldn't allocate token buffer\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, stoken);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += 64;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
	fprintf(stderr, "mShell: Couldn't reallocate token buffer\n");
	exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, stoken);
  }
  tokens[position] = NULL;
  return tokens;
}

char* helper_get_token_after(char* toSplit, char* anchor) {
  char* splittableCopy = strdup(toSplit);
  char** splitLine = split_on_token(splittableCopy, anchor);
  char** split2 = NULL;
  char* token;
  
  //the token we want is in the second half
  split2 = split_on_token(splitLine[1], " ;-|");

  //necessarily, it is the first thing in the second split
  token = split2[0];

  //free up the split memory
  free(splittableCopy);
  free(splitLine);
  free(split2);

  //return the requested token
  return token;
}

char* trim(char* str) {
  const char* beginning = str;
  const char* end = beginning + strlen(str) - 1;

  //move beginning up to the first real char
  while(isspace(*beginning)) {
    beginning++;
  }

  while(end >= beginning && isspace(*end)) {
    end--;
  }
  
  return strndup(beginning, end-beginning+1);
}
