#include <omp.h>
#include <ctime>
#include <cstdio>
#include <iostream>

using namespace std;

static long num_steps = 1000000;
double step = 0.0;

int main(){
    clock_t startTime = clock();
    
    // 为了尽可能减小误差，让程序运行1000次，计算总运行时间
    for (int k = 0; k < 1000; k++)
    {
        int i;
        double x, pi, sum = 0.0;

        step = 1.0 / (double)num_steps;

        // 在num_threads后面的括号里修改线程数目
        #pragma omp parallel for private(x) reduction(+: sum) num_threads(2)
        for (i = 1; i <= num_steps; i++) {
            x = (i - 0.5) * step;
            sum = sum + 4.0 / (1.0 + x * x);
        }

        pi = step * sum;
    }
    clock_t endTime = clock();
    cout << "The total time is " << (double)(endTime - startTime) / CLOCKS_PER_SEC << endl;
}
