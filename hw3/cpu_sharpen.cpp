#include <cstdio>

typedef float img_t;
typedef float fil_t;

const int fil_size = 3;
const int in_size  = 4;
const int out_size = in_size - fil_size + 1;
img_t img[in_size][in_size] = {};
img_t out[out_size][out_size] = {};

const fil_t f[fil_size][fil_size] = {1,  1, 1,
                                     1, -9, 1,
                                     1,  1, 1};

int main() {
    for(int i = 0; i < in_size; ++i) {
        for(int j = 0; j < in_size; ++j) {
            img[i][j] = i + j;
        }
    }
    for(int i = 0; i < out_size; ++i) {
        for(int j = 0; j < out_size; ++j) {
            out[i][j] = 0;
            for(int fi = 0; fi < fil_size; ++fi) {
                for(int fj = 0; fj < fil_size; ++fj) {
                    out[i][j] += img[i+fi][j+fj] * f[fi][fj];
                }
            }
        }
    }
    for(int i = 0; i < out_size; ++i) {
        for(int j = 0; j < out_size; ++j) {
            printf("%f ", out[i][j]);
        }
        printf("\n");
    }
}
                               
