#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Commands {
  char* cmd;
  char* args;
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

int main(int argc, char** argv) {
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
  ssize_t bufsize = 0;
  getline(&line, &bufsize, stdin);
  return line;
}

Command* shell_splitLine(char* line) {
  Command* cmds = NULL;
  char** jobs = NULL;
  printf("Got line: %s", line);
  jobs = split_on_token(line, ";");

  int i;
  for(i=0; jobs[i] != NULL; i++) {
    //first we work on individual jobs
    printf("job: %s\n", jobs[i]);
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
