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
Pipeline* shell_splitLine(char*);
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

Pipeline* shell_splitLine(char* line) {
  int numPipelines = 0;
  int pipelineBufferSize = 1;
  Pipeline pipeline;
  Pipeline* pipelines = malloc(pipelineBufferSize * sizeof(Pipeline*));

  printf("Got line: %s", line);

  //seperate the pipelines from each other
  //this way multiple lines split by ';' work
  char** pipelineStr = split_on_token(line, ";");

  int i;
  for(i=0; pipelineStr[i] != NULL; i++) {
    int numCommands = 0;
    int commandBufferSize = 5;
    Command* cmds = malloc(commandBufferSize*sizeof(Command*));
    Command command;
    
    //check if this command is anything more than a newline
    if(pipelineStr[i][0] == '\n') {
      //was just a newline, continue with no action
      continue;
    }
    //first we work on individual jobs
    printf("pipeline: %s\n", pipelineStr[i]);

    //get a list of executable portions of the pipeline
    char** executables = split_on_token(pipelineStr[i], "|");

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
      char** args = split_on_token(executables[j], " ");
      command.cmd = args[0];

      char** argsTmp = args;;
      char* arg = NULL;
      int bufsize = 64;
      command.args = malloc(bufsize * sizeof(char*));
      int k;

      //we have to iterate here instead of a direct copy because some
      //file redirection arguments need to be discarded since they are
      //stored seperately in the command struct.
      for(k=0; args[k] != NULL; k++) {
	if(k>bufsize) {
	  //oops more args than we thought, realloc accordingly...
	  bufsize += 64;
	  command.args = realloc(command.args, bufsize * sizeof(char*));
	  if(!command.args) {
	    //realloc failed, bail out now!
	    fprintf(stderr, "mShell: Couldn't expand argument buffer");
	  }
	}
	
	if(strstr(argsTmp[k], "><") != NULL) {
	  //we're at a redirection argument, fast forward
	  argsTmp = argsTmp+2;
	} else {
	  //normal argument, add to the buffer
	  command.args[k] = arg;
	}
      }
      //at this point we have a fully built command and can add it to the pipeline
      cmds[numCommands] = command;
      numCommands++;
      //the now normal buffer expansion checks
      if(numCommands > commandBufferSize) {
	commandBufferSize += 5; //add space for 5 more
	cmds = realloc(cmds, commandBufferSize * sizeof(Command*));
	if(!cmds) {
	  //this time its basically unrecoverable, so we notify FATAL status
	  fprintf(stderr, "mShell: FATAL ERROR - Could not expand command buffer");
	}
      }
    }
    //assign our command pointer so we can get it back
    pipeline.cmd = cmds;
    pipeline.length = numCommands;

    pipelines[numPipelines] = pipeline;
    numPipelines++;

    //once more we grow the buffer if needed
    if(numPipelines > pipelineBufferSize) {
      pipelineBufferSize += 2;
      pipelines = realloc(pipelines, pipelineBufferSize * sizeof(Pipeline*));
      if(!pipelines) {
	//failed to realloc
	fprintf(stderr, "mShell: FATAL ERROR - Failed to expand the pipeline buffer");
      }
    }
  }
  return pipelines;
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
