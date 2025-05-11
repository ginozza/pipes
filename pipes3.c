#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// Exponenciacion binaria
uint16_t mod_exp(uint16_t base, uint16_t exp, uint16_t mod) {
  uint16_t result = 1;
  base %= mod;

  while (exp > 0) {
    if (exp & 1) result = (result * base) % mod;
    base = (base * base) % mod;
    exp >>= 1;
  }

  return result;
}

int main(int argc, char *argv[]) {
  uint16_t x0 = atoi(argv[1]);
  uint16_t p = atoi(argv[2]);
  uint16_t q = atoi(argv[3]);
  int n = atoi(argv[4]);
  int k = atoi(argv[5]);

  uint16_t M = p * q;
  uint16_t phi = (p - 1) * (q - 1);
  int l = n / k;
  uint16_t *rn = malloc(sizeof(uint16_t) * l);

  int pipes[l][2];
  
  for (int i = 0; i < l; ++i) {
    pipe(pipes[i]);
    pid_t pid = fork();

    if (pid == 0) {
      close(pipes[i][0]);
      uint16_t exp = mod_exp(2, i, phi);
      uint16_t xi = mod_exp(x0, exp, M);
      write(pipes[i][1], &xi, sizeof(uint16_t));
      close(0);
      exit(0);
    } else {
      close(pipes[i][1]);
    }
  }

  for (int i = 0; i < l; ++i) {
    read(pipes[i][0], &rn[i], sizeof(uint16_t));
    close(pipes[i][0]);
  }

  while (wait(NULL) > 0);

  for (int i = 0; i < l; ++i) {
    printf("%" PRIu16, rn[i]);
    if (i < l - 1) printf(", ");
  }

  free(rn);
  return 0;
}
