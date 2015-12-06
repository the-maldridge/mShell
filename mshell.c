#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Commands {
  char* cmd;
  char** args;
  bool rdrstdout;
  bool rdrstdin;
  bool file2stdin;
  bool stdout2file;
  char* infname;
  char* outfname;
} Command;

void shell_loop();
char* shell_getline();
Command* shell_splitLine(char*);
char** split_on_token(char*,char*);

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
  char** pipelines = NULL;
  char** executables = NULL;
  char** args = NULL;
  printf("Got line: %s", line);
  pipelines = split_on_token(line, ";");

  int i;
  for(i=0; pipelines[i] != NULL; i++) {
    //check if this command is anything more than a newline
    if(pipelines[i][0] == '\n') {
      //was just a newline, continue with no action
      continue;
    }
    //first we work on individual jobs
    printf("pipeline: %s\n", pipelines[i]);

    //get a list of executable portions of the pipeline
    executables = split_on_token(pipelines[i], "|");

    int j;
    for(j=0; executables[j] != NULL; j++) {
      printf("Exec %i of pipeline %i: %s\n", j, i, executables[j]);
      args = split_on_token(executables[j], " ");
      command.cmd = args[0];
      args++;
      command.args = args;
      //debug all the things!!
      printf("\tExec %i executable: %s\n", j, command.cmd);
      int k;
      for(k=0; command.args[k] != NULL; k++) {
	printf("\tExec %i argument: %s\n", j, command.args[k]);
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
