#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

void error(const char *err) {
  perror(err);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  char *filename = argv[1];
  FILE *config = fopen(filename, "r");
  if (!config) error("error opening file");

  int n_proc = 0, n_rep = 0;
  fscanf(config, "%d", &n_proc);
  fscanf(config, "%d", &n_rep);

  int *order = malloc(n_proc * n_rep * sizeof(int));
  for (int i = 0; i < n_proc * n_rep; ++i)
    fscanf(config, "%d", &order[i]);

  fclose(config);

  int pipes[n_proc][2];
  for (int i = 0; i < n_proc; ++i)
    pipe(pipes[i]);

  for (int i = 0; i < n_proc; ++i) {
    pid_t pid = fork();
    if (pid == 0) {
      close(pipes[i][1]);

      for(int rep = 0; rep < n_rep; ++rep) {
        char c;
        read(pipes[i][0], &c, 1);
        printf("Proceso %d: iteracion %d (pid=%d)\n", 
               i + 1, rep + 1, getpid());
        fflush(stdout);
      }
      close(pipes[i][0]);
      exit(0);
    } else {
      close(pipes[i][0]);
    }
  }

  for (int i = 0; i < n_proc * n_rep; ++i) {
    int id = order[i] - 1;
    write(pipes[id][1], "e", 1);
    usleep(1000);
  }

  for (int i = 0; i < n_proc; ++i) {
    close(pipes[i][1]);
  }

  while (wait(NULL) > 0);

  printf("Pipeline finalizado");

  return EXIT_SUCCESS;
}
