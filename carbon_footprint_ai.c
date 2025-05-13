#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <stdlib.h>

#define N_CHILDS 2
#define MAX_LINE 256
#define FACTOR_EMISSION 0.4

typedef struct {
  int jobID;
  float kWh;
  int renewable;
} Job;

void error(const char *err) {
  perror(err);
  exit(EXIT_FAILURE);
}

char *trim_leading(char *str) {
  while (isspace(*str)) ++str;
  return str;
}

Job *read_file(const char *filename, int *n_jobs) {
  FILE *file = fopen(filename, "r");
  if (!file) error("error opening file");

  char buffer[MAX_LINE];
  if (fgets(buffer, sizeof(buffer), file) == NULL)
    error("error reading first line");

  *n_jobs = atoi(buffer);

  Job *jobs = malloc(*n_jobs * sizeof(Job));
  if (!jobs) error("error malloc jobs");

  for (int i = 0; i < *n_jobs; ++i) {
    if (fgets(buffer, sizeof(buffer), file) == NULL)
      error("error reading jobs");

    char *token = strtok(buffer, ";");
    if (!token) continue;
    jobs[i].jobID = atoi(token);

    token = strtok(NULL, ";");
    if (!token) continue;
    jobs[i].kWh = atof(token);

    token = strtok(NULL, ";");
    if (!token) continue;
    jobs[i].renewable = atoi(token);
  }

  fclose(file);

  return jobs;
}

int main(int argc, char *argv[]) {
  char *filename = argv[1];

  int n_jobs;
  Job *jobs = read_file(filename, &n_jobs);

  int pipes[N_CHILDS][2];

  for (int i = 0; i < N_CHILDS; ++i) {
    pipe(pipes[i]);
    
    pid_t pid = fork();
    if (pid == 0) {
      if (i == 0) {
        close(pipes[i][0]);
        float total_emission = .0;
        for (int j = 0; j < n_jobs; ++j)
          if (jobs[j].renewable == 0)
            total_emission += jobs[j].kWh * FACTOR_EMISSION;
        write(pipes[i][1], &total_emission, sizeof(float));
        close(pipes[i][1]);
        exit(EXIT_SUCCESS);
      } else if (i == 1) {
        close(pipes[i][0]);
        float potential_savings = .0;
        for (int j = 0; j < n_jobs; ++j)
          if (jobs[j].renewable)
            potential_savings += jobs[j].kWh * FACTOR_EMISSION;
        write(pipes[i][1], &potential_savings, sizeof(float));
        close(pipes[i][1]);
        exit(EXIT_SUCCESS);
      }
    }
  }

  close(pipes[0][1]);
  close(pipes[1][1]);

  float footprint, savings;
  read(pipes[0][0], &footprint, sizeof(float));
  read(pipes[1][0], &savings, sizeof(float));

  close(pipes[0][0]);
  close(pipes[1][0]);

  printf("Huella de carbono total: %2f kg CO2\n", footprint);
  printf("Ahorro potencial: %.2f kg CO2\n", savings);
  
  while (wait(NULL) > 0);

  return EXIT_SUCCESS;
}
