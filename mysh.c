/*
   | Author: Konstantinos  Bourantas
   |
   | Computer Engineering & Informatics
   | Department, University of Patras
   |
   | 2016 - 2017
*/
///////////////////////////////////////////////////////////////////////////////////////////////////
/*includes*/

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
/* GLOBAL VARIABLES*/

#define BUFSIZE 1024
# define DELIMITERS " \t\r\n\a|"

int args_left = 0;
int args_right = 0;
int pipes_counter = 0;
int total_cmds = 0;
int input_numb[500];
int total_args = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////

char ** split_line(char * line) {

  int bufsize = BUFSIZE;
  int i = 0;
  char ** inputs_ar = malloc(bufsize * sizeof(char * ));
  char * input;

  if (!inputs_ar) {

    fprintf(stderr, "Allocation error\n");
    exit(EXIT_FAILURE);

  }

  input = strtok(line, DELIMITERS);

  while (input != NULL) {

    inputs_ar[i] = input;

    i++;

    if (i >= bufsize) {

      bufsize += BUFSIZE;
      inputs_ar = realloc(inputs_ar, bufsize * sizeof(char * ));

      if (!inputs_ar) {

        fprintf(stderr, "Allocation error!\n");
        exit(EXIT_FAILURE);
      }
    }

    input = strtok(NULL, DELIMITERS);

  }

  inputs_ar[i] = NULL;

  return inputs_ar;

}

///////////////////////////////////////////////////////////////////////////////////////////////////

char * read_user_input() {

  char * line = NULL;
  size_t bufsize = 0;

  getline( & line, & bufsize, stdin);

  return line;

}

///////////////////////////////////////////////////////////////////////////////////////////////////

char * prepare_line_4exec(char * line) {

  int i = 0;
  int j;
  int found_word = 0;
  int cmds = 0;
  int where = 0;

  pipes_counter = 0;
  args_right = 0;

  while (line[i] != '\0') {
    if (line[i] == '|') {
      pipes_counter++;
    }

    i++;
  }

  if (pipes_counter > 0) {

    for (cmds = 0; cmds < pipes_counter; cmds++) {

      args_left = 0;

      i = where;
      while (line[i] != '\0') {

        if (line[i] == '|') {

          input_numb[cmds] = args_left;
          where = i + 1;

          break;
        } else {
          if (isspace(line[i]) || (line[i] == '|')) {
            found_word = 0;

          } else {

            if (line[i] == ';') {
              break;
            }

            if (found_word == 0)
              args_left++;

            found_word = 1;

          }
        }

        if (line[i] == ';') {

          break;
        }
        i++;
      }

    }

    where = where - 1;
    j = where;

    while (line[j] != '\0') {

      if (isspace(line[j]) || (line[j] == '|')) {
        found_word = 0;
      } else {
        if (line[j] == ';') {

          break;
        }

        if (found_word == 0) {
          args_right++;

        }

        found_word = 1;

      }
      if (line[j] == ';') {

        break;
      }

      j++;
    }

    input_numb[cmds] = args_right;

    total_args = 0;

    total_cmds = cmds;

    for (i = 0; i < cmds; i++) {

      total_args = total_args + input_numb[i];

    }
  } else {

    args_left = 0;

    if (isspace(line[i]) || (line[i] == '\n') || (line[i] == '\t')) {
      found_word = 0;

    } else {
      if (found_word == 0) {
        args_left++;
      }

      found_word = 1;

    }

  }

  return line;

}

///////////////////////////////////////////////////////////////////////////////////////////////////

int change_dir(char ** cmd) {

  chdir(cmd[1]);

  return 0;

}

///////////////////////////////////////////////////////////////////////////////////////////////////

int launch_pipe(char ** cmd) {

  int i = 0;
  int q;
  int pipes_fd[2 * pipes_counter];
  int pid = 0;
  int status = 0;
  int j = 0;
  int seen_args = 0;
  char ** exec_inputs;

  for (i = 0; i < pipes_counter; i++) {
    if (pipe(pipes_fd + i * 2) < 0) {
      fprintf(stderr, "Pipe[%d] failed!\n", i + 1);
      exit(EXIT_FAILURE);
    }
  }

  exec_inputs = malloc(BUFSIZE * sizeof(char * ));

  if (!exec_inputs) {

    fprintf(stderr, "Allocation error\n");
    exit(EXIT_FAILURE);

  }

  for (i = 0; i < total_cmds + 1; i++) {

    int loop_counter = 0;
    int index = seen_args;

    while (loop_counter < input_numb[i]) {

      exec_inputs[loop_counter] = malloc(strlen(cmd[index]) * sizeof(char)); //bug
      if (!exec_inputs[loop_counter]) {

        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);

      }

      strcpy(exec_inputs[loop_counter], cmd[index]);

      index++;
      loop_counter++;

    }

    exec_inputs[loop_counter] = NULL;

    pid = fork();

    if (pid == 0) {

      //if not first command&& j!= 2*pipes_counter
      if (j != 0) {
        if (dup2(pipes_fd[j - 2], 0) < 0) {
          fprintf(stderr, "dup2 failed!\n");
          exit(EXIT_FAILURE);
        }
      }

      //if not last command
      if (i < pipes_counter) {
        if (dup2(pipes_fd[j + 1], 1) < 0) {
          fprintf(stderr, "dup2 failed!\n");
          exit(EXIT_FAILURE);
        }
      }

      for (q = 0; q < 2 * pipes_counter; q++) {
        close(pipes_fd[q]);
      }

      if (execvp(exec_inputs[0], exec_inputs) < 0) {
        fprintf(stderr, "MyShell: %s: command not found!\n", exec_inputs[0]);
        exit(EXIT_FAILURE);
      }
    } else if (pid < 0) {
      fprintf(stderr, "fork failed!\n");
      exit(EXIT_FAILURE);
    }

    j += 2;
    seen_args = seen_args + input_numb[i];

  }

  for (i = 0; i < 2 * pipes_counter; i++) {
    close(pipes_fd[i]);
  }

  for (i = 0; i < pipes_counter + 1; i++) {
    wait( & status);
  }

  return 1;

}

///////////////////////////////////////////////////////////////////////////////////////////////////

int launch(char ** cmd) {

  int status = 0;
  int pid;

  pid = fork();

  if (pid < 0) {

    fprintf(stderr, "unable to fork!\n");
    return (-1);
  } else if (pid == 0) {

    if (cmd) {

      if (execvp(cmd[0], cmd) == -1) {
        fprintf(stderr, "MyShell: %s: command not found!\n", cmd[0]);
      }
    }
    _exit(1);
  } else {

    wait( & status);

  }

  return 1;

}

///////////////////////////////////////////////////////////////////////////////////////////////////

void about_me(void) {

  fprintf(stderr, "                                         \n");
  fprintf(stderr, "  ****************************************\n");
  fprintf(stderr, "  *     'SHELL IMPLEMENTATION IN C'      *\n");
  fprintf(stderr, "  *                                      *\n");
  fprintf(stderr, "  *                                      *\n");
  fprintf(stderr, "  *  Author: Konstantinos  Bourantas     *\n");
  fprintf(stderr, "  *                                      *\n");
  fprintf(stderr, "  *                                      *\n");
  fprintf(stderr, "  *  Computer Engineering & Informatics  *\n");
  fprintf(stderr, "  *  Department, University of Patras    *\n");
  fprintf(stderr, "  *                                      *\n");
  fprintf(stderr, "  *  2017 - 2018                         *\n");
  fprintf(stderr, "  *                                      *\n");
  fprintf(stderr, "  ****************************************\n");
  fprintf(stderr, "                                          \n");

}

///////////////////////////////////////////////////////////////////////////////////////////////////

int main(void) {

  char * line;
  char * newline;
  char ** args;
  int status = 1;
  int i = 0;
  int index = 0;

  do {
    int count_ls = 0;
    printf("$ ");
    line = read_user_input();

    int line_length = 0;

    do {

      line_length++;
    } while (line[line_length] != '\0');

    args = (char ** ) calloc(line_length, sizeof(char * ));

    if (!args) {

      fprintf(stderr, "Allocation error : args\n");
      exit(EXIT_FAILURE);

    }

    index = 0;

    while (index < line_length - 1) {
      i = 0;
      int command_length = 0;

      newline = (char * ) calloc(line_length, sizeof(char * ));

      if (!newline) {

        fprintf(stderr, "Allocation error : newline\n");
        exit(EXIT_FAILURE);

      }

      while (line && (index < line_length) && (line[index] != ';')) {
        newline[i] = line[index];
        if (line[index] != ' ' && line[index] != ';' && line[index] != '\r' && line[index] != '\n') {
          command_length++;
        }
        i++;
        index++;
      }

      if (line[index] == ';') {
        index++;
      }

      newline = prepare_line_4exec(newline);

      if (strlen(line) > 1 && command_length > 1) {

        args = split_line(newline);
        count_ls++;

        if (!strcmp(args[0], "exit")) {
          exit(1);
        } else if (!strcmp(args[0], "cd")) {
          change_dir(args);
        } else if (!strcmp(args[0], "about")) {
          about_me();
        } else {
          if (pipes_counter == 0) {
            status = launch(args);
          } else {
            status = launch_pipe(args);
          }
        }
      }

    }
  } while (1);

  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////