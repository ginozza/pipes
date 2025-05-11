#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void error(const char *err) {
  perror(err);
  exit(1);
}

typedef struct {
  double mean;
  double variance;
  double standard_deviation;
  double variation_coefficient;
} Statistics;

int count_tokens(const char* filename) {
  FILE* file = fopen(filename, "r");
  if (!file)
    error("fileoc");

  char buffer[1024];
  int token_count = 0;

  if (fgets(buffer, sizeof(buffer), file)) {
    char* token = strtok(buffer, " \t\n");
    while(token) {
      token_count++;
      token = strtok(NULL, " \t\n");
    }
  }

  fclose(file);
  return token_count;
}

int* read_column(const char *filename, int col_index, int* out_len) {
  FILE* file = fopen(filename, "r");
  if (!file)
    error("fileor");

  char buffer[1024];
  int *column = NULL;
  int count = 0;

  while (fgets(buffer, sizeof(buffer), file)) {
    int current_col = 0;
    char* token = strtok(buffer, " \t\n");

    while (token) {
      if (current_col == col_index) {
        column = realloc(column, sizeof(int) * (count + 1));
        column[count++] = atoi(token);
        break;
      }
      token = strtok(NULL, " \t\n");
      current_col++;
    }
  }

  fclose(file);
  *out_len = count;
  return column;
}

Statistics* metrics(int* column_array, int size) {
  Statistics *metrics = malloc(sizeof(Statistics));
  if (!metrics)
    error("metrics malloc");

  double add = 0.0;
  for (int i = 0; i < size; ++i) {
    add += column_array[i];
  }

  metrics->mean = add / size;

  double sum_sq_diff = 0.0;
  for (int i = 0; i < size; ++i) {
    double diff = column_array[i] - metrics->mean;
    sum_sq_diff += diff * diff;
  }

  metrics->variance = sum_sq_diff / (size - 1);
  metrics->standard_deviation = sqrt(metrics->variance);
  metrics->variation_coefficient = metrics->standard_deviation / metrics->mean;

  return metrics;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  char *filename = argv[1];
  int n_columns = count_tokens(filename);
  Statistics *file_metrics = malloc(sizeof(Statistics) * n_columns);
  int pipes[n_columns][2];
  pid_t pids[n_columns];

  for (int i = 0; i < n_columns; ++i) {
    if (pipe(pipes[i]) == -1)
      error("pipe");

    pids[i] = fork();
    if (pids[i] == -1) {
      error("fork");
    } else if (pids[i] == 0) { 
      close(pipes[i][0]); 
      
      int column_size = 0;
      int *column = read_column(filename, i, &column_size);
      Statistics *column_metrics = metrics(column, column_size);
      
      write(pipes[i][1], column_metrics, sizeof(Statistics));
      close(pipes[i][1]);
      
      free(column);
      free(column_metrics);
      exit(0);
    } else { 
      close(pipes[i][1]); 
    }
  }

  for (int i = 0; i < n_columns; ++i) {
    ssize_t bytes_read = read(pipes[i][0], &file_metrics[i], sizeof(Statistics));
    if (bytes_read != sizeof(Statistics)) {
      error("reading from pipe for column\n");
    }
    close(pipes[i][0]);
  }

  for (int i = 0; i < n_columns; ++i) {
    waitpid(pids[i], NULL, 0);
  }

  for (int i = 0; i < n_columns; ++i) {
    printf("%.f, %.f, %.f, %f\n", 
           file_metrics[i].mean, 
           file_metrics[i].variance, 
           file_metrics[i].standard_deviation, 
           file_metrics[i].variation_coefficient);
  }

  free(file_metrics);
  return 0;
}
