#include <omp.h>
#include <cstdio>

using namespace std;

#define TEST_NUM 50
static long num_steps = 1000000;
double step = 0.0;

int main(){
	int i;

    // 运行多次，观察结果是否都一样。可以改变TEST_NUM的值，运行更多次数。
    for (int k = 0; k < TEST_NUM; k++)
    {
        double x, pi, tmp, sum = 0.0;

        step = 1.0 / (double)num_steps;

        #pragma omp parallel private(tmp)
        {
            tmp = 0.0;
            #pragma omp for private(x)
                for (i = 1; i <= num_steps; i++) {
                    x = (i - 0.5) * step;
                    tmp += 4.0 / (1.0 + x * x);
                }
                #pragma omp critical
                sum += tmp;
        }

        pi = step * sum;
        printf("%lf\n", pi);
    }
}
