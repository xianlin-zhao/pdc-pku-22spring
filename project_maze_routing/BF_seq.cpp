#include <iostream>
#include <fstream>
#include <omp.h>
#include <string>
#include <vector>

using namespace std;

#define N 10000
#define BLK -1
#define inf 1 << 30
int main()
{
    ifstream fin;
    // ofstream fout;
    string str;
    
    fin.open("./benchmark/case0.txt");  // 串行版本的时间复杂度惊为天人，不要妄想跑出结果，象征性地跑case0
    cout << "open success" << endl;
    // fout.open("result.txt");
    short *maze = (short *) malloc(sizeof(short) * N * N);
    int src_id, dst_id;

    for (int i = 0; i < N; i++) {  // 一行一行读入数据，构建迷宫矩阵
        getline(fin, str);
        int ptr = 0, j = 0;
        while (j < N) {
            if (str[ptr] == '-') 
                maze[i * N + j] = BLK, ptr += 3, j++;
            else if (str[ptr] == 'S')
                maze[i * N + j] = 0, ptr += 2, src_id = i * N + j, j++;
            else if (str[ptr] == 'D')
                maze[i * N + j] = 0, ptr += 2, dst_id = i * N + j, j++;
            else
                maze[i * N + j] = (short)(str[ptr] - '0'), ptr += 2, j++;
        }
    }
    cout << "finsih read from txt" << endl;

    int *dist = (int *) malloc(sizeof(int) * N * N);
    int *prev = (int *) malloc(sizeof(int) * N * N);
    for (int i = 0; i < N * N; i++)
        dist[i] = inf, prev[i] = -1;
    dist[src_id] = 0, prev[src_id] = src_id;  // 初始化过程

    int dx[4] = {-1, 0, 1, 0};
    int dy[4] = {0, -1, 0, 1};
    for (int k = 0; k < N * N - 1; k++) {
        if (k % 50 == 0)
            printf("the %dth loop\n", k);
        bool if_change = false;  // 在一轮循环中，各点的dist是否改变，如果都没变，就可以结束了
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++) {
                if (maze[i * N + j] == BLK)  // 探测四个方向的边，如果是障碍物，就肯定没边
                    continue;
                for (int d = 0; d < 4; d++) {
                    int new_x = i + dx[d], new_y = j + dy[d];
                    if (new_x < 0 || new_x >= N || new_y < 0 || new_y >= N)  // 是否越界
                        continue;
                    if (maze[new_x * N + new_y] == BLK)  // 另一个点是障碍物，也不行
                        continue;
                    int new_dist = dist[new_x * N + new_y] + maze[i * N + j];  // 松弛，计算新的距离
                    if (new_dist < dist[i * N + j]) {  // 如果新的距离更小，就更新，并且把if_change设为true，循环还需继续
                        dist[i * N + j] = new_dist, prev[i * N + j] = new_x * N + new_y;
                        if_change = true;
                    }
                }
            }
        
        if (!if_change) {
            printf("finish in the %dth loop\n", k);
            break;
        }
    }

    cout << dist[dst_id] << endl;

    free(maze);
    free(dist);
    free(prev);

    fin.close();
    // fout.close();
    return 0;
}