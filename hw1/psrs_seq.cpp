#include <omp.h>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <random>

using namespace std;

// 论文中原封不动的Sublists函数，用pivots中[fp, lp]的枢轴值对array的[start, End]进行划分
// 每一段对应着p+1个划分点，存入subsize从at开始的位置
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

// copied是原本的数组。作为id线程，把每个大段的第id小段归并起来，然后存入array的从下标at开始的位置
// 这里没有用什么高级的归并排序算法，2个数组还比较容易，但现在是多个数组，直接都放入优先队列，然后一个个弹出来
// 二分归并其实也可以，但空间复杂度太高，而且实现起来比较麻烦
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
    vector<int> sample(p * (p - 1));  // p段的每一段都取p-1个点
    vector<int> pivots(p);  // 存到[1, p - 1]
    vector<int> subsize(p * (p + 1));  // 每一小段的起始下标
    vector<int> bucksize(p);  // 每一段最终应该从哪个下标开始
    int start, End;
    for (int i = 0; i < p; i++)  // 把整个数组分成p大段，每段先内部排序，然后取p-1个样本
    {
        start = i * size;
        End = (i + 1) * size - 1;
        if (End >= n)
            End = n - 1;
        sort(array.begin() + start, array.begin() + End + 1);
        for (int j = 1; j < p; j++)
        {
            if (start + j * rsize <= End)
                sample[i * (p - 1) + j - 1] = array[start + j * rsize];
            else
                sample[i * (p - 1) + j - 1] = array[End];
        }
    }

    sort(sample.begin(), sample.end());  // 把取出的样本排序
    for (int i = 0; i < p - 1; i++)  // 把p-1个枢轴点存入pivots[1, p - 1]
        pivots[i + 1] = sample[i * p + (p / 2)];
    
    for (int i = 0; i < p; i++)  // 把每一大段分割成p个小段，下标都存入subsize[]
    {
        start = i * size;
        End = (i + 1) * size - 1;
        if (End >= n)
            End = n - 1;
        subsize[i * (p + 1)] = start;
        subsize[i * (p + 1) + p] = End + 1;
        Sublists(array, start, End, subsize, i * (p + 1), pivots, 1, p - 1);
    }

    for (int i = 0; i < p; i++)  // 所有大段里的第i小段将会由线程i来处理，计算每个线程要处理多少个元素
    {
        bucksize[i] = 0;
        for (int j = i; j < p * (p + 1); j = j + p + 1)
            bucksize[i] = bucksize[i] + subsize[j + 1] - subsize[j];
    }

    int last = bucksize[0];
    bucksize[0] = 0;
    for (int i = 1; i < p; i++)  // 计算最终每个大段的起始下标
    {
        int now = bucksize[i];
        bucksize[i] = bucksize[i - 1] + last;
        last = now;
    }

    vector<int> copied(array);
    for (int i = 0; i < p; i++)  // 线程i负责合并每个大段中的第i小段，并搬移到array相应的位置
        Merge(array, subsize, p, bucksize[i], i, copied);
}

bool test(vector<int> & check, int sz)  // 测试程序的正确性
{
    for (int i = 0; i < sz; i++)
        if (check[i] != i)
            return false;
    return true;
}

int main()
{
    int n = 8000000, p = 16;  // 在这里修改n和p，进行测试
    vector<int> params(n);
    for (int i = 0; i < n; i++)
        params[i] = i;
    random_shuffle(params.begin(), params.end());  // 数组被随机打乱顺序
    clock_t startTime = clock();
    psrs(params, n, p);
    clock_t endTime = clock();
    cout << "The total time is " << (double)(endTime - startTime) / CLOCKS_PER_SEC << endl;
    if (test(params, n))
        cout << "Success!!!" << endl;
    else
        cout << "fail..." << endl;
    return 0;
}
