#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int m, n, num_children;

void generate_matrix(int** matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            matrix[i][j] = rand() % 10;
}

void generate_vector(int* vector, int size) {
    for (int i = 0; i < size; i++)
        vector[i] = rand() % 10;
}

int main() {
    srand(time(NULL));

    printf("Enter matrix size (rows m, columns n): ");
    scanf("%d %d", &m, &n);

    printf("Enter number of child processes: ");
    scanf("%d", &num_children);

    int** matrix = malloc(m * sizeof(int*));
    for (int i = 0; i < m; i++)
        matrix[i] = malloc(n * sizeof(int));

    int* vector = malloc(n * sizeof(int));

    generate_matrix(matrix, m, n);
    generate_vector(vector, n);

    printf("Matrix A:\n");
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) printf("%d ", matrix[i][j]);
        printf("\n");
    }

    printf("Vector B:\n");
    for (int i = 0; i < n; i++) printf("%d ", vector[i]);
    printf("\n");

    int child_pipes[num_children][2];
    int parent_pipes[num_children][2];

    for (int i = 0; i < num_children; i++) {
        pipe(child_pipes[i]);
        pipe(parent_pipes[i]);

        pid_t pid = fork();
        if (pid == 0) {
            close(child_pipes[i][1]);
            close(parent_pipes[i][0]);

            while (1) {
                int row[n];
                int row_idx;

                if (read(child_pipes[i][0], &row_idx, sizeof(int)) <= 0)
                    break;

                read(child_pipes[i][0], row, sizeof(row));

                int result = 0;
                for (int j = 0; j < n; j++)
                    result += row[j] * vector[j];

                write(parent_pipes[i][1], &row_idx, sizeof(int));
                write(parent_pipes[i][1], &result, sizeof(int));
            }

            close(child_pipes[i][0]);
            close(parent_pipes[i][1]);
            exit(0);
        }
        close(child_pipes[i][0]);
        close(parent_pipes[i][1]);
    }

    int current_row = 0;
    int* final_result = malloc(m * sizeof(int));

    while (current_row < m) {
        for (int i = 0; i < num_children && current_row < m; i++) {
            write(child_pipes[i][1], &current_row, sizeof(int));
            write(child_pipes[i][1], matrix[current_row], sizeof(int) * n);
            current_row++;
        }

        for (int i = 0; i < num_children; i++) {
            int idx, res;
            if (read(parent_pipes[i][0], &idx, sizeof(int)) > 0) {
                read(parent_pipes[i][0], &res, sizeof(int));
                final_result[idx] = res;
            }
        }
    }

    for (int i = 0; i < num_children; i++) {
        close(child_pipes[i][1]);
        close(parent_pipes[i][0]);
        wait(NULL);
    }

    printf("Result A * B:\n");
    for (int i = 0; i < m; i++)
        printf("%d\n", final_result[i]);

    for (int i = 0; i < m; i++)
        free(matrix[i]);
    free(matrix);
    free(vector);
    free(final_result);

    return 0;
}
