#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void shell_loop();
char* shell_getline();

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
}

char* shell_getline() {
  //we set this to null so that it is free-able
  char *line = NULL;

  //getline auto alocates buffers
  ssize_t bufsize = 0;
  getline(&line, &bufsize, stdin);
  return line;
}
