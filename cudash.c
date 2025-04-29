#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMANDS 100
#define MAX_LINE 256

void error(const char *message) {
  perror(message);
  exit(1);
}

int read_lines(const char *filename, char commands[MAX_COMMANDS][MAX_LINE]) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    error("error abriendo archivo");
  }

  int count = 0;
  while (fgets(commands[count], MAX_LINE, file)) {
    size_t len = strlen(commands[count]);
    if (commands[count][len - 1] == '\n')
      commands[count][len - 1] = '\0';
    count++;
  }

  fclose(file);
  return count;
}

void exec_command(const char *cmd, int write_fd) {
  dup2(write_fd, STDOUT_FILENO);
  close(write_fd);

  execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);

  error("execl fallo");
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Uso: %s archivo_comandos.txt\n", argv[0]);
    exit(1);
  }

  char commands[MAX_COMMANDS][MAX_LINE];
  int num_cmds = read_lines(argv[1], commands);

  int pipefd[2];
  if (pipe(pipefd) == -1) {
    error("pipe fallo");
  }

  int i, j;
  for (i = 0; i < num_cmds; ++i) {
    pid_t pid = fork();
    if (!pid) {
      pid_t epid = fork();
      if (epid < 0)
        error("epid fallo");
      if (!epid) {
        close(pipefd[0]);
        exec_command(commands[i], pipefd[1]);
      }
    } else if (pid < 0)
      error("pid fallo");
  }

  close(pipefd[1]);

  char buffer[1024];
  int bytes;
  while ((bytes = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes] = '\0';
    printf("%s", buffer);
  }

  close(pipefd[0]);

  for (int j = 0; j < num_cmds; ++j) {
    wait(NULL);
  }

  return 0;
}
