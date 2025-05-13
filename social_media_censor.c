#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE_LEN 256

typedef struct {
  char **text_vector;
  char **words_vector;
  int rows;
  int cols;
  int n_words;
} WordSearch;

typedef struct {
  int row;
  int col;
  char direction[20];
  char word[MAX_LINE_LEN];
} WordFound;

const char *DIRV[] = {
  "diagonal arriba izquierda",
  "arriba",
  "diagonal arriba derecha",
  "izquierda",
  "derecha",
  "diagonal abajo izquierda",
  "abajo",
  "diagonal abajo derecha"
};

void error(const char *err) {
  perror(err);
  exit(1);
}

WordSearch read_file(FILE *file) {
  WordSearch ws;
  if (fscanf(file, "%d %d", &ws.rows, &ws.cols) != 2)
    error("invalid format");

  ws.text_vector = malloc(ws.rows * sizeof(char *));
  if (!ws.text_vector) error("error malloc text vector");

  int ch = fgetc(file);

  for (int i = 0; i < ws.rows; ++i) {
    ws.text_vector[i] = malloc(ws.cols + 1);
    if (!ws.text_vector[i]) error("error malloc text vector row");
    int count = 0;
    while (count < ws.cols) {
      ch = fgetc(file);
      if (ch == EOF) error("unexpected eof");
      if (ch == ' ' || ch == '\t' || ch == '\r') continue;
      if (ch == '\n') continue;
      ws.text_vector[i][count++] = (char)ch;
    }
    ws.text_vector[i][ws.cols] = '\0';
    while ((ch = fgetc(file)) != '\n' && ch != EOF);
  }

  if (fscanf(file, "%d", &ws.n_words) != 1)
    error("invalid format");

  ws.words_vector = malloc(ws.n_words * sizeof(char *));
  if (!ws.words_vector) error("error malloc words vector");

  for (int i = 0; i < ws.n_words; ++i) {
    ws.words_vector[i] = malloc(MAX_LINE_LEN);
    if (!ws.words_vector[i]) error("error malloc words vector row");
    fscanf(file, "%s", ws.words_vector[i]);
  }

  fclose(file);
  return ws;
}
 
int search2D(WordSearch *ws, int x, int y, char *word, int pipe_fd) {
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    int len = strlen(word);
    int found_any = 0;

    for (int dir = 0; dir < 8; ++dir) {
        int xx = x + dx[dir] * (len - 1);
        int yy = y + dy[dir] * (len - 1);
        
        if (xx < 0 || xx >= ws->rows || yy < 0 || yy >= ws->cols)
            continue;

        int c;
        for (c = 0; c < len; ++c) {
            xx = x + dx[dir] * c;
            yy = y + dy[dir] * c;
            if (xx < 0 || xx >= ws->rows || yy < 0 || yy >= ws->cols) break;
            if (ws->text_vector[xx][yy] != word[c])
                break;
        }
        if (c == len) {
            WordFound out;
            out.row = x;
            out.col = y;
            strncpy(out.direction, DIRV[dir], sizeof(out.direction) - 1);
            strncpy(out.word, word, sizeof(out.word) - 1);
            write(pipe_fd, &out, sizeof(out));
            found_any = 1;
        }
    }
    return found_any;
}

int main(int argc, char *argv[]) {
  if (argc != 3) error("format: <filename> <n_childs>\n");

  char *filename = argv[1];
  FILE *file = fopen(filename, "r");
  if (!file) error("error opening file");

  WordSearch ws = read_file(file);

  int n_childs = atoi(argv[2]);

  int pipes[n_childs][2];
  for (int i = 0; i < n_childs; ++i) 
    if (pipe(pipes[i]) == -1)
      error("pipec");

  for (int i = 0; i < n_childs; ++i) {
    pid_t pid = fork();

    if (pid == 0) {
      for (int j = 0; j < n_childs; ++j) {
        close(pipes[j][0]);
        if (j != i) close(pipes[j][1]);
      }
      int total = ws.n_words;
      int start_idx = i * (total / n_childs) + (i < total % n_childs ? i : total % n_childs);
      int end_idx = start_idx + (total / n_childs) - 1 + (i < total % n_childs ? 1 : 0);

      for (int w = start_idx; w <= end_idx; ++w)
        for (int r = 0; r < ws.rows; ++r){
          for (int c = 0; c < ws.cols; ++c) {
            search2D(&ws, r, c, ws.words_vector[w], pipes[i][1]);    
          }
        }
      close(pipes[i][1]);
      exit(0);
    }
  }

  for (int i = 0; i < n_childs; ++i)
    close(pipes[i][1]);

  while (wait(NULL) > 0);

  for (int i = 0; i < n_childs; ++i) {
    WordFound fnd;
    while (read(pipes[i][0], &fnd, sizeof(fnd)) == sizeof(fnd)) {
      printf("Palabra: '%s', encontrada en (%d,%d), direccion: hacia %s\n",
             fnd.word, fnd.row, fnd.col, fnd.direction);
    }
    close(pipes[i][0]);
  }

  for (int i = 0; i < ws.rows; ++i)
    free(ws.text_vector[i]);
  free(ws.text_vector);

  for (int i = 0; i < ws.n_words; ++i)
    free(ws.words_vector[i]);
  free(ws.words_vector);

  return EXIT_SUCCESS;
}
