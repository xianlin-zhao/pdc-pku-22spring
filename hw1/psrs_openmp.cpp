#include <omp.h>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <random>

#define num_threads 16  // 在这里修改线程数目，问题规模在main函数里修改

using namespace std;

/*
 * 整体的逻辑和psrs_seq完全一样，只是把for循环改成由每个线程执行
 * 因此与算法逻辑相关的注释就不写了，重点说一下openmp相关的部分
 * 记住：编译的时候要 -fopenmp
 */

void Sublists(vector<int> & array, int start, int End, vector<int> & subsize, int at,
            vector<int> & pivots, int fp, int lp)
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

void Merge(vector<int> & array, vector<int> & subsize, int p, int at, int id, vector<int> & copied)
{
    vector<int> tmp;
    priority_queue<int, vector<int>, greater<int> > pq;
    int sumLen = 0;
    for (int i = 0; i < p; i++)
    {
        int len = subsize[id + i * (p + 1) + 1] - subsize[id + i * (p + 1)];
        sumLen += len;
        tmp.resize(len);
        copy(copied.begin() + subsize[id + i * (p + 1)], copied.begin() + subsize[id + i * (p + 1) + 1], tmp.begin());
        for (int j = 0; j < len; j++)
            pq.push(tmp[j]);
    }
    tmp.resize(sumLen);
    for (int i = 0; i < sumLen; i++)
    {
        int now = pq.top();
        pq.pop();
        tmp[i] = now;
    }
    copy(tmp.begin(), tmp.end(), array.begin() + at);
}

void psrs(vector<int> & array, int n, int p)
{
    int size = (n + p - 1) / p;
    int rsize = (size + p - 1) / p;
    vector<int> sample(p * (p - 1));
    vector<int> pivots(p);  // 存入[1, p - 1]
    vector<int> subsize(p * (p + 1));
    vector<int> bucksize(p);
    vector<int> copied(n);
    int start, End, id;  // 每个进程有自己的id. start和End就是这个进程负责的那段的起止下标

    omp_set_num_threads(num_threads);  // 设置线程的数量，可以在define里面修改
    #pragma omp parallel private(id, start, End)
    {
        id = omp_get_thread_num();
        int start = id * size;
        int End = (id + 1) * size - 1;
        if (End >= n)
            End = n - 1;
        sort(array.begin() + start, array.begin() + End + 1);
        for (int j = 1; j < p; j++)
        {
            if (start + j * rsize <= End)
                sample[id * (p - 1) + j - 1] = array[start + j * rsize];
            else
                sample[id * (p - 1) + j - 1] = array[End];
        }
        #pragma omp barrier
        // 所有线程必须都把自己负责的sample算出来，在这里设置barrier，防止下面的sort出问题
        
        #pragma omp master
        {
            // 这部分只由主线程来做，对sample排序，然后选出p-1个pivots
            sort(sample.begin(), sample.end());
            for (int i = 0; i < p - 1; i++)
                pivots[i + 1] = sample[i * p + (p / 2)];
            // 其他线程必须等待主线程选好pivots，否则下面对每一段的划分会有问题
        }
        #pragma omp barrier

        // 每个线程负责划分自己那部分，划分结束后必须有barrier
        subsize[id * (p + 1)] = start;
        subsize[id * (p + 1) + p] = End + 1;
        Sublists(array, start, End, subsize, id * (p + 1), pivots, 1, p - 1);
        #pragma omp barrier

        // 线程id将会处理各大段的第id小段，所以现在计算一下自己接下来一共要处理多少个元素
        bucksize[id] = 0;
        for (int j = id; j < p * (p + 1); j = j + p + 1)
            bucksize[id] = bucksize[id] + subsize[j + 1] - subsize[j];
        #pragma omp barrier

        #pragma omp master
        {
            // 只由主线程来完成，前缀和，计算最终的每一大段要从哪个下标开始
            int last = bucksize[0];
            bucksize[0] = 0;
            for (int i = 1; i < p; i++)
            {
                int now = bucksize[i];
                bucksize[i] = bucksize[i - 1] + last;
                last = now;
            }
            copy(array.begin(), array.end(), copied.begin()); // 把现在的array拷贝下来，接下来所有线程都要使用
            // 不能把copy移到下面，否则每个线程看到的就是修改过的array
        }
        #pragma omp barrier

        Merge(array, subsize, p, bucksize[id], id, copied);
    }
}

bool test(vector<int> & check, int sz)
{
    for (int i = 0; i < sz; i++)
        if (check[i] != i)
            return false;
    return true;
}

int main()
{
    int n = 8000000;  // 在这里修改问题规模
    vector<int> params(n);
    for (int i = 0; i < n; i++)
        params[i] = i;
    random_shuffle(params.begin(), params.end());
    clock_t startTime = clock();
    psrs(params, n, num_threads);
    clock_t endTime = clock();
    cout << "The total time is " << (double)(endTime - startTime) / CLOCKS_PER_SEC << endl;
    if (test(params, n))
        cout << "Success!!!" << endl;
    else
        cout << "fail..." << endl;
    return 0;
}
