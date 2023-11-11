#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>

#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

char *words[MAX_WORDS];
int background_pid = 0;
int last_exit_status = 0;
size_t wordsplit(char const *line);
char * expand(char const *word);

int main(int argc, char *argv[])
{
  FILE *input = stdin;
  char *input_fn = "(stdin)";
  if (argc == 2) {
    input_fn = argv[1];
    input = fopen(input_fn, "re");
    if (!input) err(1, "%s", input_fn);
  } else if (argc > 2) {
    errx(1, "too many arguments");
  }

  char *line = NULL;
  size_t n = 0;
  for (;;) {
prompt:;
    /* TODO: Manage background processes */

    /* DONE! TODO: prompt */
    if (input == stdin) {
      if (getenv("PS1")) fprintf(stderr, "%s", getenv("PS1"));
      else fprintf(stderr, "%s", "");
    }
    ssize_t line_len = getline(&line, &n, input);
    if (line_len < 0) err(1, "%s", input_fn);
    
    int run_in_background = 0;
    char *exec_args[MAX_WORDS] = {0};
    size_t nwords = wordsplit(line);
    for (size_t i = 0; i < nwords; ++i) {
      if (!strncmp(words[i], "$", 1)) {
        // fprintf(stderr, "Word %zu: %s\n", i, words[i]);
        char *exp_word = expand(words[i]);
        free(words[i]);
        words[i] = exp_word;
        // fprintf(stderr, "Expanded Word %zu: %s\n", i, words[i]);
      }
      if (nwords - 1 == i && !strcmp(words[i], "&")) {
         run_in_background = 1;
         // fprintf(stderr, "Background Status %d", run_in_background);
      } 
    }
    /* Built in exit and cd functions */
    if (!strcmp(words[0], "exit")) {
      if (nwords == 1) exit(last_exit_status);
      else if (nwords == 2){
        char *ptr;
        long exit_status;
        errno = 0;
        exit_status = strtol(words[1], &ptr, 10);
        if (errno == ERANGE || (errno != 0 && exit_status ==0) || ptr == words[1]) {
          fprintf(stderr, "Error with desired exit status\n");
          goto prompt;
        } else exit(exit_status);
      }
      else {
        fprintf(stderr, "Too many arguments\n");
        goto prompt;
      }
    }
    else if (!strcmp(words[0], "cd")) {
      errno = 0;
      if (nwords == 1) {
        chdir(getenv("HOME"));
        if (errno != 0) {
          fprintf(stderr, "Invalid path: %s\n", words[1]);
          goto prompt;
        }
      }
      else if (nwords == 2) {
        chdir(words[1]);
        if (errno != 0) {
          fprintf(stderr, "Invalid path: %s\n", words[1]);
          goto prompt;
        }
      }
      else {
        fprintf(stderr, "Too many arguments.\n");
        goto prompt;
      }
    }
  /* Start the non-built in functions here. Adapted from the modules */
    else {
      pid_t fork_id = -2;
      int child_status;
      int child_pid;

      fork_id = fork();
      switch (fork_id) {
        case -1:
          // Fork failed. Report as needed.
          fprintf(stderr, "Failed to fork process. Try again\n");
          exit(1);
          break;
        case 0:
          // Inside the child process here fork_id is 0
          //
          char *exec_args[MAX_WORDS] = {0};
          int arg_items = 1;
          exec_args[0] = words[0];

          for (size_t i = 1; i < nwords; ++i) {
            if (!strcmp(words[i], "<")){
              if (nwords > i + 1) {
                int input_fd = open(words[i + 1], O_RDONLY);
                if (input_fd == -1) {
                  fprintf(stderr, "Error opening input fd\n");
                  exit(5);
                }
                ++i;
                int dup_status = dup2(input_fd, 0);
                if (dup_status == -1) {
                  fprintf(stderr, "Error redirecting to stdin\n");
                  exit(5);
                }
              }
              else {
                fprintf(stderr, "Could not locate the desired input file\n");
                exit(5);
              }
            }
            else if (!strcmp(words[i], ">")) {
              if (nwords > i + 1) {
                int output_fd = open(words[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
                if (output_fd == -1){
                  fprintf(stderr, "Error opening output fd\n");
                  exit(5);
                }
                ++i;
                int dup_status = dup2(output_fd, 1);
                if (dup_status == -1) {
                  fprintf(stderr, "Error redirecting to stdout\n");
                  exit(5);
                }
              }
              else {
                fprintf(stderr, "Could not locate the desired output file\n");
                exit(5);
              }
            }
            else if (!strcmp(words[i], ">>")) {
              if (nwords > i + 1) {
                int output_fd = open(words[i + 1], O_WRONLY | O_CREAT, 0777);
                if (output_fd == -1){
                  fprintf(stderr, "Error opening output fd\n");
                  exit(5);
                }
                ++i;
                int dup_status = dup2(output_fd, 1);
                if (dup_status == -1) {
                  fprintf(stderr, "Error redirecting to stdout\n");
                  exit(5);
                }
              }
              else {
                fprintf(stderr, "Could not locate the desired output file\n");
                exit(5);
              }
            }
            else if (nwords - 1 == i && !strcmp(words[i], "&")) {
                // Do nothing, we already flagged the command as a background process.
            }
            else {
              exec_args[arg_items] = words[i];
              ++arg_items;
            }
          }
          exec_args[arg_items] = NULL;
          errno = 0;
          int ex_return = execvp(words[0], exec_args);
          if (ex_return == -1) {
            fprintf(stderr, "Execvp failure.\n");
          }
          break;
        default:
          // Inside parent. fork_id is child pid.
          break;
      }
    }
  }
}

char *words[MAX_WORDS] = {0};


/* Splits a string into words delimited by whitespace. Recognizes
 * comments as '#' at the beginning of a word, and backslash escapes.
 *
 * Returns number of words parsed, and updates the words[] array
 * with pointers to the words, each as an allocated string.
 */
size_t wordsplit(char const *line) {
  size_t wlen = 0;
  size_t wind = 0;

  char const *c = line;
  for (;*c && isspace(*c); ++c); /* discard leading space */

  for (; *c;) {
    if (wind == MAX_WORDS) break;
    /* read a word */
    if (*c == '#') break;
    for (;*c && !isspace(*c); ++c) {
      if (*c == '\\') ++c;
      void *tmp = realloc(words[wind], sizeof **words * (wlen + 2));
      if (!tmp) err(1, "realloc");
      words[wind] = tmp;
      words[wind][wlen++] = *c; 
      words[wind][wlen] = '\0';
    }
    ++wind;
    wlen = 0;
    for (;*c && isspace(*c); ++c);
  }
  return wind;
}


/* Find next instance of a parameter within a word. Sets
 * start and end pointers to the start and end of the parameter
 * token.
 */
char
param_scan(char const *word, char **start, char **end)
{
  static char *prev;
  if (!word) word = prev;
  
  char ret = 0;
  *start = NULL;
  *end = NULL;
  char *s = strchr(word, '$');
  if (s) {
    char *c = strchr("$!?", s[1]);
    if (c) {
      ret = *c;
      *start = s;
      *end = s + 2;
    }
    else if (s[1] == '{') {
      char *e = strchr(s + 2, '}');
      if (e) {
        ret = '{';
        *start = s;
        *end = e + 1;
      }
    }
  }
  prev = *end;
  return ret;
}

/* Simple string-builder function. Builds up a base
 * string by appending supplied strings/character ranges
 * to it.
 */
char *
build_str(char const *start, char const *end)
{
  static size_t base_len = 0;
  static char *base = 0;

  if (!start) {
    /* Reset; new base string, return old one */
    char *ret = base;
    base = NULL;
    base_len = 0;
    return ret;
  }
  /* Append [start, end) to base string 
   * If end is NULL, append whole start string to base string.
   * Returns a newly allocated string that the caller must free.
   */
  size_t n = end ? end - start : strlen(start);
  size_t newsize = sizeof *base *(base_len + n + 1);
  void *tmp = realloc(base, newsize);
  if (!tmp) err(1, "realloc");
  base = tmp;
  memcpy(base + base_len, start, n);
  base_len += n;
  base[base_len] = '\0';

  return base;
}

/* Expands all instances of $! $$ $? and ${param} in a string 
 * Returns a newly allocated string that the caller must free
 */
char *
expand(char const *word)
{
  char const *pos = word;
  char *start, *end;
  char c = param_scan(pos, &start, &end);
  build_str(NULL, NULL);
  build_str(pos, start);
  while (c) {
    if (c == '!') {
      if (background_pid) {
        char pid_string[21];
        sprintf(pid_string, "%d", background_pid);
        build_str(pid_string, NULL);
      }
      else build_str("", NULL);
    }
    else if (c == '$') {
      int pid = getpid();
      char pid_string[21];
      sprintf(pid_string, "%d", pid);
      build_str(pid_string, NULL);
    }
    else if (c == '?') {
      char string[3];
      sprintf(string, "%d", last_exit_status);
      build_str(string, NULL);
    } 
    else if (c == '{') {
      char *variable;
      memcpy(variable, start + 2, strlen(start) - 3);
      if (getenv(variable)) build_str(getenv(variable), NULL);
      else build_str("", NULL);
      // build_str("<Parameter: ", NULL);
      // build_str(start + 2, end - 1);
      // build_str(">", NULL);
    }
    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }
  return build_str(start, NULL);
}
