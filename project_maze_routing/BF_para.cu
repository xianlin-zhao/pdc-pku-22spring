#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <string>
#include <vector>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

using namespace std;

#define N 10000
#define BLK -1
#define inf 1 << 30
#define FILENUM 5

// 分别是每个grid中的block数，每个block中的线程数。一共1e8个格子，大约是1<<27，每个block中设1<<9个线程，所以block数设置为1<<18
#define BLOCK_NUM 1 << 18
#define THREAD_NUM 512

// double consume_time[FILENUM]{};

/*
 * kernel函数
 * 前4个参数：迷宫矩阵、每个点距源点的距离、每个点的前驱、临时的dist（防止读写竞争，先把结果写到别处，最后再复制到dist里）
 * 后4个参数：本轮循环中是否有dist改变、四个方向的dx、四个方向的dy，矩阵边长（就是N，但是要搬运到cuda上）
 */
__global__ void BF_algo(short *d_maze, int *d_dist, int *d_prev, int *d_dist_tmp, 
                        bool *d_has_next, int *d_dx, int *d_dy, int n)
{
    int tid = blockDim.x * blockIdx.x + threadIdx.x;  // 线程的全局id

    if (tid >= n * n || d_maze[tid] == BLK)  // 是否超过了格子的最大编号？是否本身是障碍物？
        return;
    int i = tid / n, j = tid % n;  // 计算出二维坐标

    for (int k = 0; k < 4; k++) {  // 探测四个方向的边
        int newx = i + d_dx[k], newy = j + d_dy[k];
        if (newx < 0 || newx >= n || newy < 0 || newy >= n)
            continue;
        if (d_maze[newx * n + newy] == BLK)
            continue;
        int newdist = d_dist[newx * n + newy] + d_maze[i * n + j];
        if (newdist < d_dist[i * n + j]) {  // 松弛操作，新的距离比原来的dist小，更新
            d_dist_tmp[i * n + j] = newdist, d_prev[i * n + j] = newx * n + newy;
            *d_has_next = true;  // 有dist被更新了，所以还会有下一轮循环
        }
    }
}

int main()
{
    ofstream fout;
    fout.open("result.txt");
    short *maze = (short *) malloc(sizeof(short) * N * N);
    int *dist = (int *) malloc(sizeof(int) * N * N);
    int *prev = (int *) malloc(sizeof(int) * N * N);
    int *dist_tmp = (int *) malloc(sizeof(int) * N * N);  // dist的副本，所有线程结束后，把它复制到dist里

    for (int casenum = 0; casenum < FILENUM; casenum++) {
        ifstream fin;
        string str;
        // char num_str[10];
        // itoa(casenum, num_str, 10);
        // string case_path = "./benchmark/case" + string(num_str) + ".txt";
        string case_path = "./benchmark/case" + to_string(casenum) + ".txt";
        fin.open(case_path);
        if (fin.is_open())
            cout << "open success " << casenum << endl;  // DEBUG
        else
            break;

        int src_id, dst_id;
        for (int i = 0; i < N; i++) {
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

        for (int i = 0; i < N * N; i++)
            dist[i] = inf, prev[i] = -1, dist_tmp[i] = inf;
        dist[src_id] = 0, prev[src_id] = src_id, dist_tmp[src_id] = 0;


        cudaDeviceReset();
        short *d_maze;
        int *d_dist, *d_prev, *d_dist_tmp, *d_dx, *d_dy;
        bool *d_has_next, has_next;
        int dx[4] = {-1, 0, 1, 0};
        int dy[4] = {0, -1, 0, 1};

        // cuda显存的分配，以及把主机上对应的数组内容拷贝到device上
        cudaMalloc(&d_maze, sizeof(short) * N * N);
        cudaMalloc(&d_dist, sizeof(int) * N * N);
        cudaMalloc(&d_prev, sizeof(int) * N * N);
        cudaMalloc(&d_dist_tmp, sizeof(int) * N * N);
        cudaMalloc(&d_has_next, sizeof(bool));
        cudaMalloc(&d_dx, sizeof(int) * 4);
        cudaMalloc(&d_dy, sizeof(int) * 4);

        cudaMemcpy(d_maze, maze, sizeof(short) * N * N, cudaMemcpyHostToDevice);
        cudaMemcpy(d_dist, dist, sizeof(int) * N * N, cudaMemcpyHostToDevice);
        cudaMemcpy(d_prev, prev, sizeof(int) * N * N, cudaMemcpyHostToDevice);
        cudaMemcpy(d_dist_tmp, dist_tmp, sizeof(int) * N * N, cudaMemcpyHostToDevice);
        cudaMemcpy(d_dx, dx, sizeof(int) * 4, cudaMemcpyHostToDevice);
        cudaMemcpy(d_dy, dy, sizeof(int) * 4, cudaMemcpyHostToDevice);

        printf("ready to start BF\n");

        timeval startTime, endTime;
        gettimeofday(&startTime, NULL);
        // clock_t startTime = clock();

        for (int i = 0; i < N * N - 1; i++) {  // (N*N-1)轮循环，每轮都是所有线程并行操作的
            has_next = false;
            cudaMemcpy(d_has_next, &has_next, sizeof(bool), cudaMemcpyHostToDevice);  // 最初，d_has_next是false，松弛时可能会改变
            BF_algo<<<BLOCK_NUM, THREAD_NUM>>>(d_maze, d_dist, d_prev, d_dist_tmp, d_has_next, d_dx, d_dy, N);
            cudaDeviceSynchronize();  // 等待所有格子执行结束
            cudaMemcpy(&has_next, d_has_next, sizeof(bool), cudaMemcpyDeviceToHost);
            if (!has_next) {  // 先判断算法是否结束
                printf("finish in %d loop\n", i);  // DEBUG
                break;
            }
            cudaMemcpy(d_dist, d_dist_tmp, sizeof(int) * N * N, cudaMemcpyDeviceToDevice);  // 把新的dist_tmp拷贝到dist
            if (i % 2000 == 0) {
                printf("kernel %d\n", i);  // DEBUG
            }
        }

        gettimeofday(&endTime, NULL);
        double consume = (endTime.tv_sec - startTime.tv_sec) + (double)(endTime.tv_usec - startTime.tv_usec) / 1000000.0;
        
        // 结束后，从device上把dist和prev拷贝回主机，准备得出最短路径。然后把cudaMalloc分配的所有资源释放掉
        cudaMemcpy(dist, d_dist, sizeof(int) * N * N, cudaMemcpyDeviceToHost);
        cudaMemcpy(prev, d_prev, sizeof(int) * N * N, cudaMemcpyDeviceToHost);
        cudaFree(d_maze);
        cudaFree(d_dist);
        cudaFree(d_prev);
        cudaFree(d_dist_tmp);
        cudaFree(d_has_next);
        cudaFree(d_dx);
        cudaFree(d_dy);
 
        printf("cuda end:  dist is %d\n", dist[dst_id]);
        
        if (dist[dst_id] == inf) {
            fout << "case" << casenum << " -1" << endl;
            // consume_time[casenum] = (double)(clock() - startTime) / CLOCKS_PER_SEC;
            continue;
        }

        fout << "case" << casenum << " " << dist[dst_id] << " ";
        vector<int> path;
        path.push_back(dst_id);
        int now_nd = dst_id;
        while(prev[now_nd] != now_nd) {
            path.push_back(prev[now_nd]);
            now_nd = prev[now_nd];
        }

        // clock_t endTime = clock();
        // consume_time[casenum] = (double)(endTime - startTime) / CLOCKS_PER_SEC;
        int sz = path.size();

        for (int i = sz - 1; i >= 0; i--) {
            fout << "(" << path[i] / N << ", " << path[i] % N << ") ";
        }
        fout << endl;
        fin.close();
        cout << "case " << casenum << " " << consume << endl;  // 时间就不单独输出到文件了，直接显示在屏幕上
    }
    
    free(maze);
    free(dist);
    free(prev);
    free(dist_tmp);   

    fout.close();

    return 0;
}
