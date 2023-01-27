#include <mpi.h>
#include <ctime>
#include <cstdio>
#include <algorithm>
#include <vector>

using namespace std;

/*
 * 用pivots数组的[fp, lp]下标范围的枢轴值来划分array数组[start, End]的范围，划分结果放入从subsize数组
 * 的从at下标开始的位置。和上次作业一模一样（只不过把vector改成了数组）
 */
void Sublists(int *array, int start, int End, int *subsize, int at,
            int *pivots, int fp, int lp)
{
    int mid = (fp + lp) / 2;
    int pv = pivots[mid]; 
    int lb = start, ub = End;
    while (lb <= ub)
    {
        int center = (lb + ub) / 2;
        if (array[center] > pv)
            ub = center - 1;
        else
            lb = center + 1;
    }
    subsize[at + mid] = lb;
    if (fp < mid)
        Sublists(array, start, lb - 1, subsize, at, pivots, fp, mid - 1);
    if (mid < lp)
        Sublists(array, lb, End, subsize, at, pivots, mid + 1, lp);
}

bool test(int *check, int sz) // 测试正确性，已经测试通过了，就不再调用了
{
    for (int i = 0; i < sz; i++)
        if (check[i] != i)
            return false;
    return true;
}

int main(int argc, char *argv[])
{
    int n = 8000000;  // 在这里修改问题规模
    vector<int> params(n);
    for (int i = 0; i < n; i++)
        params[i] = i;
    random_shuffle(params.begin(), params.end()); // 为了得到乱序的数组（因为我不知道数组本身如何打乱）
    int *a = new int[n];
    for (int i = 0; i < n; i++)
        a[i] = params[i];
    
    clock_t startTime = clock();
    int p, id; // 线程数目，当前线程的id
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    int size = (n + p - 1) / p;
    int rsize = (size + p - 1) / p;
    int start = id * size;
    int End = (id + 1) * size - 1;
    if (End >= n)
        End = n - 1;
    sort(a + start, a + End + 1);  // 将数组分成size个大段，每个段先自己排序

    int *sample = new int[p * (p - 1)];  // 各个大段分别取p-1个样本，放入sample数组中
    for (int i = 1; i < p; i++)
    {
        if (start + i * rsize <= End)
            sample[id * (p - 1) + i - 1] = a[start + i * rsize];
        else
            sample[id * (p - 1) + i - 1] = a[End];
    }
    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 0; i < p; i++)
        MPI_Bcast(sample + i * (p - 1), p - 1, MPI_INT, i, MPI_COMM_WORLD);  // 各大段把取出的样本广播给其他线程
    
    int *pivots = new int[p];
    if (id == 0)  // 主线程负责把sample数组排序，然后取出p-1个枢轴值，放入pivots数组的[1, p-1]
    {
        sort(sample, sample + p * (p - 1));
        for (int i = 0; i < p - 1; i++)
            pivots[i + 1] = sample[i * p + (p / 2)];
    }
    MPI_Bcast(pivots, p, MPI_INT, 0, MPI_COMM_WORLD);  // 取完pivots后，广播给其他所有线程

    int *subsize = new int[p + 1];  // 对于这个线程负责的大段，用p-1个枢轴值划分，再加上头尾，共有p+1个划分点
    subsize[0] = start;
    subsize[p] = End + 1;
    Sublists(a, start, End, subsize, 0, pivots, 1, p - 1);

    // 下面这部分和论文中的代码不一样，用MPI收发的过程难以套用论文中的伪代码
    int *persize = new int[p];  // 这个大段的每个小段的元素个数
    for (int i = 0; i < p; i++)
        persize[i] = subsize[i + 1] - subsize[i];

    int *aftersize = new int[p];  // 对于线程i，它要接收其他所有线程的第i小段，aftersize[j]是j号线程的第i小段的长度
    MPI_Alltoall(persize, 1, MPI_INT, aftersize, 1, MPI_INT, MPI_COMM_WORLD);  // 互相通知要发送的元素个数
    int tmp = 0;  // 计算全局交换后，这个线程接下来一共要处理多少个元素，就是对aftersize求和
    for (int i = 0; i < p; i++)
        tmp += aftersize[i];
    
    int *newnum = new int[tmp];  // 准备存放自己接下来要负责排序的所有元素
    int *senddis = new int[p];  // 自己要发送的各小段数据的偏移量
    int *recdis = new int[p];  // 自己要接收的各小段数据相对newnum起点的偏移量
    senddis[0] = 0, recdis[0] = 0;  // 接下来就是用前缀和算出来
    for (int i = 1; i < p; i++)
    {
        senddis[i] = senddis[i - 1] + persize[i - 1];
        recdis[i] = recdis[i - 1] + aftersize[i - 1];
    }
    MPI_Barrier(MPI_COMM_WORLD);  // 等待所有线程都算出来收发的偏移，防止出错，然后全局交换
    MPI_Alltoallv(a + start, persize, senddis, MPI_INT, newnum, aftersize, recdis, MPI_INT, MPI_COMM_WORLD);

    sort(newnum, newnum + tmp);  // 按道理要多路归并的，但其实差不了多少，没必要

    int *everytotal = new int[p];  // 各个线程分别负责了多少个元素，用于最后整合到原数组时确定偏移量
    MPI_Gather(&tmp, 1, MPI_INT, everytotal, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (id == 0)  // 大家把各自的元素个数整合到一起之后，用前缀和计算最终的偏移量（这里直接把recdis又拿来用了，反正之前已经用完了）
    {
        recdis[0] = 0;
        for (int i = 1; i < p; i++)
            recdis[i] = recdis[i - 1] + everytotal[i - 1];
    }
    MPI_Gatherv(newnum, tmp, MPI_INT, a, everytotal, recdis, MPI_INT, 0, MPI_COMM_WORLD);  // 收集到原数组中

    clock_t endTime = clock();
    double t = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    MPI_Barrier(MPI_COMM_WORLD);
    double *allTime = new double[p];  // 记录每个线程所用的时间，为了选出一个最大值作为实际运行的时间
    MPI_Gather(&t, 1, MPI_DOUBLE, allTime, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    if (id == 0)  // 主线程负责挑出运行时间的最大值
    {
        double timeAns = allTime[0];
        for (int i = 1; i < p; i++)
            if (allTime[i] > timeAns)
                timeAns = allTime[i];
        printf("The total time is %lf\n", timeAns);
        // if (test(a, n))
        //     printf("Success!!\n");
        // else
        //     printf("fail...\n");    
    }

    // 把new出来的数组都释放掉
    free(sample);
    free(pivots);
    free(subsize);
    free(persize);
    free(aftersize);
    free(newnum);
    free(senddis);
    free(recdis);
    free(everytotal);
    free(allTime);
    MPI_Finalize();
    return 0;
}
