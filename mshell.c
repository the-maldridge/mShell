#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

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
  Command** cmd;
  int length;
} Pipeline;

typedef struct {
  Pipeline** pipelines;
  int numPipeLines;
} ShellContext;

void shell_loop();
char* shell_getline();
ShellContext* shell_splitLine(char*);
char** split_on_token(char*, char*);
char* helper_get_token_after(char*, char*);
char* trim(char*);
void debug_printPipelines(ShellContext*);

int main() {
  printf("Welcome to mShell\n");

  shell_loop();
  return 0;
}


void shell_loop() {
  bool exit=false;
  while(!exit) {
    //get the data from the line
    char* line = shell_getline();

    if(strcmp(line, "exit")==0) {
      exit=true;
    } else {
      //parse the line
      ShellContext* context = shell_splitLine(line);

      //debugging
      debug_printPipelines(context);
    }
  }
}

char* shell_getline() {
  //we set this to null so that it is free-able
  char *line = NULL;

  #ifdef USE_READLINE
  line = readline("> ");
  if(line && *line) {
    //only save lines with text in them
    add_history(line);
  }
  #else
  printf("> ");

  size_t bufsize = 0;
  getline(&line, &bufsize, stdin);
  #endif

  return line;
}

ShellContext* shell_splitLine(char* line) {
  int numPipelines = 0;
  int pipelineBufferSize = 2;
  Pipeline** pipelines = malloc(pipelineBufferSize * sizeof(Pipeline*));
  
  //seperate the pipelines from each other
  //this way multiple lines split by ';' work
  char** pipelineStr = split_on_token(line, ";");

  //set up the pipeline processing loop
  //the interior of this loop deals with single pipelines
  int i;
  int numCommands = 0;
  int commandBufferSize = 5;
  Command** cmds = malloc(commandBufferSize*sizeof(Command*));
  for(i=0; pipelineStr[i] != NULL; i++) {
    printf("Pipeline contains: %s\n", pipelineStr[i]);
    Pipeline* pipeline = malloc(sizeof(Pipeline));
    //check if this command is anything more than a newline
    if(pipelineStr[i][0] == '\n') {
      //was just a newline, continue with no action
      continue;
    }
    //first we work on individual jobs
    //get a list of executable portions of the pipeline
    char** executables = split_on_token(pipelineStr[i], "|");

    //at this point we can iterate on executables and process
    //specific options for redirection and options handling
    int j;
    for(j=0; executables[j] != NULL; j++) {
      Command* command = malloc(sizeof(Command));

      //initialize everything in that struct to avoid
      //valgrind's dreaded "Uninitialized value"
      memset(command, 0, sizeof(Command));
    
      //we now need to know if there's redirection going on
      if(strstr(executables[j], ">>") != NULL) {
	//output will create/append a file
	command->rdrstdout = true;
	command->stdout2file = true;
	command->outfmode = APPEND;
	command->outfname = helper_get_token_after(executables[j], ">>");
      } else if(strstr(executables[j], ">") != NULL) {
	//output will create/overwrite a file
	command->rdrstdout = true;
	command->stdout2file = true;
	command->outfmode = OVERWRITE;
	command->outfname = helper_get_token_after(executables[j], ">");
      } else if(executables[j+1] != NULL) {
	//there's another command in the pipeline after us
	command->rdrstdout = true;
	command->stdout2file = false;
      } else {
	command->rdrstdout = false;
      }
      
      if(strstr(executables[j], "<") != NULL) {
	printf("Got an input token\n");
	//input will implicitly come from a file
	command->rdrstdin = true;
	command->file2stdin = true;
	command->infname = helper_get_token_after(executables[j], "<");
      } else if(j != 0) {
	//we exist in the middle of a pipeline
	command->rdrstdin = true;
	command->file2stdin = false;
      } else {
	command->rdrstdin = false;
      }
      
      
      //at this point we can finally tokenize the arguments
      char** args = split_on_token(executables[j], " ");
      command->cmd = args[0];
      
      int bufsize = 64;
      command->args = malloc(bufsize * sizeof(char*));
      memset(command->args, 0, bufsize * sizeof(sizeof(char*)));
      int k;
      
      //we have to iterate here instead of a direct copy because some
      //file redirection arguments need to be discarded since they are
      //stored seperately in the command struct.  This loop starts at
      //1 since the 0th argument is for the command itself
      int copyLoc = 0;
      for(k=1; args[k] != NULL; k++) {
	if(copyLoc>bufsize) {
	  //oops more args than we thought, realloc accordingly...
	  bufsize += 64;
	  command->args = realloc(command->args, bufsize * sizeof(char*));
	  if(!command->args) {
	    //realloc failed, bail out now!
	    fprintf(stderr, "mShell: Couldn't expand argument buffer");
	  }
	}
	printf("Considering argument %i: %s\n", k, args[k]);
	if(strstr(args[k], ">") != NULL) {
	  //redirection argument, we should fast forward
	  printf("Ignoring argument %i: %s\n", k, args[k]);
	  k += 1;
	  printf("Ignoring argument %i: %s\n", k, args[k]);
	} else if(strstr(args[k], "<") != NULL) {
	  //redirection argument, we should fast forward
	  printf("Ignoring argument %i: %s\n", k, args[k]);
	  k += 1;
	  printf("Ignoring argument %i: %s\n", k, args[k]);
	} else {
	  //normal argument, add to the buffer
	  printf("accepting argument %i: %s\n", k, args[k]);
	  command->args[copyLoc] = args[k];
	  copyLoc++;
	}
      }
      
      //at this point we have a fully built command and can add it to the pipeline
      cmds[numCommands] = command;
      numCommands++;
      //the now normal buffer expansion checks
      if(numCommands > commandBufferSize) {
	commandBufferSize += numCommands*2; //add space for 5 more
	cmds = realloc(cmds, commandBufferSize * sizeof(Command*));
	if(!cmds) {
	  //this time its basically unrecoverable, so we notify FATAL status
	  fprintf(stderr, "mShell: FATAL ERROR - Could not expand command buffer");
	}
      }
    }
    //assign our command pointer so we can get it back
    pipeline->cmd = cmds;
    pipeline->length = numCommands;

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

  ShellContext* context = malloc(sizeof(ShellContext));
  context->pipelines = pipelines;
  context->numPipeLines = numPipelines;
  return context;
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

  //if split2 isn't null, then the filename is the 0th line
  //from the second split, otherwise, its the first split's
  //result trimmed of whitespace
  if(split2 != NULL) {
    token = strdup(split2[0]);
  } else {
    token = trim(splitLine[1]);
  }

  //free up the split memory
  free(splittableCopy);
  free(splitLine);
  free(split2);


  printf("filename: %s\n", token);
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
  
  char* nStr = strndup(beginning, end-beginning+1);
  free(str);
  return nStr;
}

void debug_printPipelines(ShellContext* context) {
  int i, j, k;
  Pipeline** pipelines = context->pipelines;
  printf("Processed %i pipelines:\n", context->numPipeLines);						       
  for(i = 0; i<context->numPipeLines; i++) {
    printf("Pipeline: %i\n", i);
    printf("length: %i\n", pipelines[i]->length);
    for(j = 0; j<pipelines[i]->length; j++) {
      printf("\tCommand: %s\n", pipelines[i]->cmd[j]->cmd);

      //print out the stdin redirected status
      printf("\t\tstdin:\n");
      if(pipelines[i]->cmd[j]->rdrstdin) {
	printf("\t\t\tStatus: Redirected\n");
	if(pipelines[i]->cmd[j]->file2stdin){
	  printf("\t\t\tSource: File\n");
	  printf("\t\t\tFile: %s\n", pipelines[i]->cmd[j]->infname);
	} else {
	  printf("\t\t\tSource: Pipeline\n");
	}
      } else {
	printf("\t\t\tStatus: Not Redirected\n");
      }

      //print out the stdout redirected status
      printf("\t\tstdout:\n");
      if(pipelines[i]->cmd[j]->rdrstdout) {
	printf("\t\t\tStatus: Redirected\n");
	if(pipelines[i]->cmd[j]->stdout2file) {
	  printf("\t\t\tDestination: File\n");
	  printf("\t\t\tFile: %s\n", pipelines[i]->cmd[j]->outfname);
	  if(pipelines[i]->cmd[j]->outfmode == APPEND) {
	    printf("\t\t\tMode: Append\n");
	  } else {
	    printf("\t\t\tMode: Overwrite\n");
	  }
	} else {
	  printf("\t\t\tDestination: Pipeline\n");
	}
      } else {
	printf("\t\t\tStatus: Not Redirected\n");
      }

      //finally print out the arguments
      printf("\t\tArguments:\n");
      if(pipelines[i]->cmd[j]->args == NULL) {
	printf("\t\t\tNo Arguments\n");
      } else {
	for(k = 0; pipelines[i]->cmd[j]->args[k] != NULL; k++) {
	  printf("\t\t\t%s\n", pipelines[i]->cmd[j]->args[k]);
	}
      }
    }
  }
}
