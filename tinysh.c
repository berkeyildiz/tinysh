#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <dirent.h>
#include <stdint.h>
#include <fcntl.h>


int tinysh_cd(char **args);
int tinysh_help(char **args);
int tinysh_exit(char **args);
int tinysh_ls(char **args);
int tinysh_pwd();
int tinysh_touch(char **args);


char *supported_args[] = {
  "cd",
  "ls",
  "pwd",
  "touch",
  "help",
  "exit"
};

int (*supported_func[]) (char **) = {
  &tinysh_cd,
  &tinysh_ls,
  &tinysh_pwd,
  &tinysh_touch,
  &tinysh_help,
  &tinysh_exit
};

int tinysh_num_Supported() {
  return sizeof(supported_args) / sizeof(char *);
}

#define MAXPATHLEN 1024
int tinysh_cd(char **args){
  char buf[MAXPATHLEN];
  if (args[1] == NULL) {
    fprintf(stderr, "tinysh: need a directory! \"cd\" <directory>\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("tinysh no dir");
    }
    printf("cwd is: %s\n", getcwd(buf, MAXPATHLEN));
  }

  return 1;
}

int tinysh_ls(char **args) {

  char *curr_dir = NULL;
	DIR *dp;
	struct dirent *dirp;
  struct stat mystat;
  struct tm *tm;
  struct passwd *pwd;
  struct group *grp;

  curr_dir = getenv("PWD");
  dp = opendir((const char*)curr_dir);
  char buf[4096];
  char datestring[256];
  if(args[1]==NULL){
    while((dirp = readdir(dp))!=NULL){
      printf("%s ", dirp->d_name);
    }
    printf("\n");
  }
  else{
    while((dirp = readdir(dp))!=NULL){
      sprintf(buf, "%s%s", args[1], dirp->d_name);
      stat(buf,&mystat);
      printf(" %9jd", (intmax_t)mystat.st_size);
      if ((pwd = getpwuid(mystat.st_uid)) != NULL)
          printf(" %-8.8s", pwd->pw_name);
        else
          printf(" %-8d", mystat.st_uid);

      tm = localtime(&mystat.st_mtime);
      strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);
      printf(" %s %s\n", datestring, dirp->d_name);
    }
  }
  closedir(dp);
  return 1;
}

int tinysh_pwd(){
  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  printf("Current working directory(pwd): %s\n", cwd);
  return 1;
}

int tinysh_touch(char **args){
  if(args[1]==NULL){
    fprintf(stderr, "tinysh: need a file name! \"touch\" <file_name>\n");
  }
  else{
    char *file_name = args[1];
    FILE* file_ptr = fopen(file_name, "w");
    fclose(file_ptr);
  }
  return 1;
}

int tinysh_help(char **args){
  int i;
  printf("***Tiny Linux Shell***\n");
  printf("Following arguments are supported:\n");

  for (i = 0; i < tinysh_num_Supported(); i++) {
    printf("  %s\n", supported_args[i]);
  }

  return 1;
}

int tinysh_exit(char **args){
  printf("You are exiting from the tiny shell!");
  return 0;
}

int tinyshLaunch(char **args){
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {

    if (execvp(args[0], args) == -1) {
      perror("Given argument is not supported");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {

    perror("tinysh: launching error.");
  } else {

    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int tinyshExecute(char **args)
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < tinysh_num_Supported(); i++) {
    if (strcmp(args[0], supported_args[i]) == 0) {
      return (*supported_func[i])(args);
    }
  }

  return tinyshLaunch(args);
}

#define TINYSH_RL_BUFSIZE 1024


 char *tinyshRead(void){
   char *line = NULL;
   ssize_t bufsize = 0;
   getline(&line, &bufsize, stdin);
   return line;
 }

#define TINYSH_TOK_BUFSIZE 64
#define TINYSH_TOK_DELIM " \t\r\n\a"


char **tinyshSplit(char *line){
  int bufsize = TINYSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "tinysh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TINYSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += TINYSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "tinysh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TINYSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

void tinyshLoop(void){
  char *line;
  char **args;
  int status;

  do {
    printf("tinysh> ");
    line = tinyshRead();
    args = tinyshSplit(line);
    status = tinyshExecute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv){

  tinyshLoop();

  return EXIT_SUCCESS;
}
