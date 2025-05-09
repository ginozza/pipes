#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

void error(const char *err) {
  perror(err);
  exit(1);
}

int main(int argc, char *argv[]) {
  int n_childs = atoi(argv[1]);

  size_t messageLen = 0;
  // Conocer el tamaño del mensaje
  for (int i = 2; i < argc; ++i) {
    messageLen += strlen(argv[i]) + 1;
  }

  // Alocar el tamaño del mensaje
  char *message = malloc(messageLen);
  message[0] = '\0';

  // Copiar el mensaje a message
  for (int i = 2; i < argc; ++i) {
    strcat(message, argv[i]);
    if (i < argc - 1)
      strcat(message, " ");
  }

  // Creacion de tuberias
  int p[n_childs][2];
  for (int i = 0; i < n_childs; ++i) {
    if (pipe(p[i]) == -1) {
      error("pipec");
    }
  }
 
  // Estructura de procesos y logica
  int i;
  for (i = 0; i < n_childs; ++i) {
    pid_t pid = fork();
    if(pid < 0) error("forkc");
    if(!pid) {
      char buffer[1024] = {0};
      if (i == 0) {
        close(p[i][0]);

        strcpy(buffer, message);
        write(p[i][1], buffer, strlen(buffer));

        close(p[i][1]);
      } else {
        close(p[i][0]);
        close(p[i-1][1]);

        read(p[i-1][0], buffer, sizeof(buffer) - 1);
        close(p[i-1][0]);

        write(p[i][1], buffer, strlen(buffer));
        close(p[i][1]);
      }      
      printf("Hijo %d, recibió: %s\n", i + 1, buffer);
      free(message);
      exit(0);
    }
  } 

  close(p[n_childs - 1][1]);
  char end_message[1024] = {0};
  read(p[n_childs - 1][0], end_message, sizeof(end_message) - 1);
  close(p[n_childs - 1][0]);

  printf("Padre recibió: %s \n", end_message);

  while (wait(NULL) > 0);

  return 0;
}
