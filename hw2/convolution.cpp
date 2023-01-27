#include <ctime>
#include <cstdlib>

#define Matrix_N 4
// For simplicity, we assume kernel size is an odd number
#define Kernel_N 3

using namespace std;

// You can ignore this part when evaluating performance
void generate_data(int matrix[Matrix_N][Matrix_N], int kernel[Kernel_N][Kernel_N]) {
    srand((int) time(nullptr));
    for (int i = 0; i < Matrix_N; i++)
        for (int j = 0; j < Matrix_N; j++)
            matrix[i][j] = rand() % 20;

    for (int i = 0; i < Kernel_N; i++)
        for (int j = 0; j < Kernel_N; j++)
            kernel[i][j] = (rand() * (-1 ^ rand()))%5;
}

void kernel_rotate(int kernel[Kernel_N][Kernel_N]) {

    for (int i = 0; i < int((Kernel_N + 1) / 2); i++) {
        for (int j = 0; j < Kernel_N; j++) {
            int t = kernel[i][j];
            kernel[i][j] = kernel[Kernel_N - 1 - i][Kernel_N - 1 - j];
            kernel[Kernel_N - 1 - i][Kernel_N - 1 - j] = t;
        }
    }
    for (int j = 0; j < int(Kernel_N / 2); j++) {
        int t = kernel[int(Kernel_N / 2)][j];
        kernel[int(Kernel_N / 2)][j] = kernel[int(Kernel_N / 2)][Kernel_N - 1 - j];
        kernel[int(Kernel_N / 2)][Kernel_N - 1 - j] = t;
    }

}

void convolution(int matrix[Matrix_N][Matrix_N], int kernel[Kernel_N][Kernel_N], int result[Matrix_N][Matrix_N]) {
    int temp[Matrix_N + Kernel_N - 1][Matrix_N + Kernel_N - 1]{};
    for (int i = 0; i < Matrix_N; i++)
        for (int j = 0; j < Matrix_N; j++)
            temp[int(Kernel_N / 2) + i][int(Kernel_N / 2) + j] = matrix[i][j];

    for (int i = 0; i < Matrix_N; i++)
        for (int j = 0; j < Matrix_N; j++) {
            int sum = 0;
            for (int s = 0; s < Kernel_N; s++)
                for (int t = 0; t < Kernel_N; t++)
                    sum += kernel[s][t] * temp[i + s][j + t];
            result[i][j] = sum;
        }
}

int main() {
    int matrix[Matrix_N][Matrix_N]{}, kernel[Kernel_N][Kernel_N]{};
    int result[Matrix_N][Matrix_N];

    generate_data(matrix, kernel);
    kernel_rotate(kernel);
    convolution(matrix, kernel, result);
    return 0;
}