#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define N_CHILDS 4
#define N_NEIGHBORS 8

void error(const char *err) {
  perror(err);
  exit(EXIT_FAILURE);
}

typedef struct {
  int pos_x;
  int pos_y;
} Mine;

int **read_file(FILE *file, int *n_rows, int *n_cols) {

  fscanf(file, "%d", n_rows);
  fscanf(file, "%d", n_cols);

  int **matrix = calloc(*n_rows, sizeof(int *));
  for (int i = 0; i < *n_rows; ++i)
    matrix[i] = calloc(*n_cols, sizeof(int));

  for (int i = 0; i < *n_rows; ++i)
    for (int j = 0; j < *n_cols; ++j)
      fscanf(file, "%d", &matrix[i][j]);

  fclose(file);

  return matrix;
}

Mine find_mine(int **matrix, int x, int y, int n_rows, int n_cols, int pipe_fd) {
  int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
  int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  int mine_flag = 0;

  for (int i = 0; i < N_NEIGHBORS; ++i) {
    int nx = x + dx[i];
    int ny = y + dy[i];

    if (nx >= 0 && ny >= 0 && nx < n_rows && ny < n_cols) {
      if (matrix[nx][ny] == 2 && matrix[x][y] == 1)
        mine_flag = 1;
    }
  }

  Mine mine = {0};

  if (mine_flag == 1){
    mine.pos_x = x;
    mine.pos_y = y;
    write(pipe_fd, &mine, sizeof(mine));
    return mine;
  }

  return mine;
}

int main(int argc, char *argv[]) {
  char *filename = argv[1];
  FILE *file = fopen(filename, "r");
  if (!file) error("error opening file");

  int n_rows = 0, n_cols = 0;
  int **matrix = read_file(file, &n_rows, &n_cols);

  int pipes[N_CHILDS][2];
  for (int i = 0; i < N_CHILDS; ++i)
    if (pipe(pipes[i]) == -1)
      error("pipec");

  Mine found_mine;

  for (int i = 0; i < N_CHILDS; ++i) {
    pid_t pid = fork();

    if (pid == 0) {
      for (int j = 0; j < N_CHILDS; ++j){
        close(pipes[j][0]);
        if (j != i) close(pipes[j][1]);
      }

      int start = i * (n_rows / N_CHILDS);
      int end = (i == N_CHILDS - 1) ? n_rows : start + (n_rows / N_CHILDS);

      for (int r = start; r < end; ++r) {
        for (int c = 0; c < n_cols; ++c) {
          find_mine(matrix, r, c, n_rows, n_cols, pipes[i][1]);
        }
      }
      
      close(pipes[i][1]);
      exit(EXIT_SUCCESS);
    }
  }

  for (int i = 0; i < N_CHILDS; ++i)
    close(pipes[i][1]);

  while (wait(NULL) > 0);

  for (int i = 0; i < N_CHILDS; ++i) {
    while(read(pipes[i][0], &found_mine, sizeof(found_mine)) == sizeof(found_mine)) {
      printf("Mina encontrada en la posición [%d,%d]\n", found_mine.pos_x, found_mine.pos_y);
    }
    close(pipes[i][0]);
  }

  for (int i = 0; i < n_rows; ++i)
    free(matrix[i]);
  free(matrix);
  
  return EXIT_SUCCESS;
}
