#include <mpi.h>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <string.h>

// 矩阵规模和kernel大小都在这里修改。卷积核无非是3*3、5*5、7*7这些，差别不会太多，测试的时候都用5*5
#define Matrix_N 16000
// For simplicity, we assume kernel size is an odd number
#define Kernel_N 5

using namespace std;

// You can ignore this part when evaluating performance
// 和样例代码基本相同，只是把二维数组都改成了一维数组
void generate_data(int matrix[(Matrix_N + Kernel_N - 1) * (Matrix_N + Kernel_N - 1)], int kernel[Kernel_N * Kernel_N])
{
    srand((int) time(NULL));
    int realSize = Matrix_N + Kernel_N - 1;
    int padding = int(Kernel_N / 2);
    for (int i = 0; i < Matrix_N; i++)
        for (int j = 0; j < Matrix_N; j++)
        {
            int x = i + padding, y = j + padding;
            matrix[x * realSize + y] = rand() % 20;
        }
    
    for (int i = 0; i < Kernel_N; i++)
        for (int j = 0; j < Kernel_N; j++)
            kernel[i * Kernel_N + j] = (rand() * (-1 ^ rand())) % 5;
}

// 也是和示例代码基本相同，只是把二维数组改成了一维数组
void kernel_rotate(int kernel[Kernel_N * Kernel_N])
{
    for (int i = 0; i < int((Kernel_N + 1) / 2); i++)
        for (int j = 0; j < Kernel_N; j++)
        {
            int t = kernel[i * Kernel_N + j];
            kernel[i * Kernel_N + j] = kernel[(Kernel_N - 1 - i) * Kernel_N + Kernel_N - 1 - j];
            kernel[(Kernel_N - 1 - i) * Kernel_N + Kernel_N - 1 - j] = t;
        }
    for (int j = 0; j < int(Kernel_N / 2); j++)
    {
        int t = kernel[(int(Kernel_N / 2)) * Kernel_N + j];
        kernel[(int(Kernel_N / 2)) * Kernel_N + j] = kernel[(int(Kernel_N / 2)) * Kernel_N + Kernel_N - 1 - j];
        kernel[(int(Kernel_N / 2)) * Kernel_N + Kernel_N - 1 - j] = t;
    }
}

/*
 * 对于input数组，用kernel来卷积，计算出的结果放入output中
 * output是一维数组，对应的二维数组的高度和宽度分别是perHeight和perWidth
 * input的高度是perHeight+Kernel_N-1，宽度是perWidth+Kernel_N-1
 */
void convolution(int perHeight, int perWidth, int *input, int *output, int kernel[Kernel_N * Kernel_N])
{
    for (int i = 0; i < perHeight; i++)
    {
        for (int j = 0; j < perWidth; j++)
        {
            int sum = 0;
            for (int s = 0; s < Kernel_N; s++)
                for (int t = 0; t < Kernel_N; t++)
                    sum += kernel[s * Kernel_N + t] * input[(i + s) * (perWidth + Kernel_N - 1) + (j + t)];
            output[i * perWidth + j] = sum;
        }
    }
}

int main(int argc, char *argv[])
{
    int p, id;  // 线程数目，自己的id
    // 输入、输出、卷积核本来是二维矩阵，为了方便MPI收发，都换成了一维数组
    int matrix[(Matrix_N + Kernel_N - 1) * (Matrix_N + Kernel_N - 1)];
    int kernel[Kernel_N * Kernel_N];
    int result[Matrix_N * Matrix_N];
    memset(matrix, 0, sizeof(matrix));
    memset(kernel, 0, sizeof(kernel));
    memset(result, 0, sizeof(result));
    generate_data(matrix, kernel);
    kernel_rotate(kernel);

    clock_t startTime = clock();

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Status status;

    int perWidth = Matrix_N, perHeight = Matrix_N / p;  // 每个线程要输出的结果矩阵的维度
    int realWidth = Matrix_N + Kernel_N - 1;  // 线程实际接收的输入的宽度（因为两边加了padding）
    int realHeight = perHeight + Kernel_N - 1;  // 线程实际接收的输入的高度，为了卷积，要加上Kernel_N-1
    int *myMatrix = new int[realHeight * realWidth];  // 这个线程的输入（由主线程负责分发）
    int *myResult = new int[perHeight * perWidth];  // 这个线程卷积后的输出（各自发送给主线程）

    if (id == 0)
    {
        for (int i = 1; i < p; i++)  // 按行划分，主线程把相应的行发送给对应的线程
        {
            int inOffset = realWidth * (i * perHeight);  // 要发送的数据相对于matrix的偏移
            // 发送元素个数为realHeight*realWidth
            MPI_Send(matrix + inOffset, realHeight * realWidth, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }
    else  // 其他线程从主线程那里接收数据
        MPI_Recv(myMatrix, realHeight * realWidth, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

    MPI_Bcast(kernel, Kernel_N * Kernel_N, MPI_INT, 0, MPI_COMM_WORLD);
    if (id == 0)  // 主线程直接在原矩阵上计算就行
        convolution(perHeight, perWidth, matrix, result, kernel);
    else  // 对myMatrix卷积，结果存到myResult中
        convolution(perHeight, perWidth, myMatrix, myResult, kernel);
    MPI_Barrier(MPI_COMM_WORLD);

    if (id == 0)  // 主线程把各个线程的计算结果收集起来，放到result里
    {
        for (int i = 1; i < p; i++)
        {
            int outOffset = perWidth * (i * perHeight);  // 放到result的多少偏移量处
            MPI_Recv(result + outOffset, perHeight * perWidth, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
        }
    }
    else  // 给主线程发结果
        MPI_Send(myResult, perHeight * perWidth, MPI_INT, 0, 1, MPI_COMM_WORLD);
    
    MPI_Barrier(MPI_COMM_WORLD);
    clock_t endTime = clock();
    double t = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    MPI_Barrier(MPI_COMM_WORLD);
    double *allTime = new double[p];  // 统计每个线程的运行时间
    MPI_Gather(&t, 1, MPI_DOUBLE, allTime, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    if (id == 0)  // 由主线程负责统计运行时间的最大值，作为结果
    {
        double timeAns = allTime[0];
        for (int i = 1; i < p; i++)
            if (allTime[i] > timeAns)
                timeAns = allTime[i];
        printf("The total time is %lf\n", timeAns);
    }
    
    // 把new出来的数组释放掉
    free(myMatrix);
    free(myResult);
    free(allTime);
    MPI_Finalize();
    return 0;
}
