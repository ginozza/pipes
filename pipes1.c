#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

void edit_file(const char *file_name, const char *message) {
  FILE *file = fopen(file_name, "w");
  if (file == NULL) {
    perror("Error opening file");
    exit(1);
  }
  fprintf(file, "%s", message);
  fclose(file);
}

int main() {
  const char *file_name = "file.txt";
  const char *message = "Hello World (2)\nhola mundo\nc++ >> python\nhola diablo";
  edit_file(file_name, message);

  int p1[2], p2[2];
  if (pipe(p1) < 0 || pipe(p2) < 0) exit(1);

  for (int i = 0; i < 3; i++) {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (!pid) {
      if (i == 0) {
        close(p1[0]);
        close(p2[0]);
        close(p2[1]);

        FILE *file = fopen(file_name, "r");
        if (!file) exit(1);

        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file)) {
          write(p1[1], buffer, strlen(buffer));
        }
        fclose(file);

        close(p1[1]);

        exit(0);
      } else if (i == 1) {
        close(p1[1]);
        close(p2[0]);

        char buffer[1024];
        int n;
        while ((n = read(p1[0], buffer, sizeof(buffer))) > 0) {
          write(p2[1], buffer, n);
        }
        close(p1[0]);
        close(p2[1]);

        exit(0);
      } else if (i == 2) {
        close(p1[0]);
        close(p1[1]);
        close(p2[1]);

        char buffer[1024];
        int n;
        while ((n = read(p2[0], buffer, sizeof(buffer))) > 0) {
          printf("Hijo 3 recibió: %s", buffer);
        }
        close(p2[0]);

        exit(0);
      }
    }
  }

  close(p1[0]);
  close(p1[1]);
  close(p2[0]);
  close(p2[1]);

  while (wait(NULL) > 0);

  return 0;
}

