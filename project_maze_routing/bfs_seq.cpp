#include <iostream>
#include <fstream>
#include <omp.h>
#include <string>
#include <vector>
#include <queue>
#include <sys/time.h>

using namespace std;

#define N 10000 
#define BLK -1
#define inf 1 << 30
#define FILENUM 5
double consume_time[FILENUM]{};

struct node {  // 队列里的元素，node的nid代表当前所处格子的编号[0, 100000000)，cost是迄今为止的代价总和
    int nid, cost;
    node(int n, int c): nid(n), cost(c){}
    bool friend operator < (node node1, node node2) {  // 优先队列里，cost小的在队首
        return node1.cost > node2.cost;
    }
};

int main()
{
    ofstream fout;
    fout.open("result.txt");
    int dx[4] = {-1, 0, 1, 0};
    int dy[4] = {0, -1, 0, 1};
    short *maze = (short *) malloc(sizeof(short) * N * N);
    int *dist = (int *) malloc(sizeof(int) * N * N);
    int *prev = (int *) malloc(sizeof(int) * N * N);
    bool *visit = (bool *) malloc(sizeof(bool) * N * N);  // bfs算法专用，记录每个格子是否已经访问过

    for (int casenum = 0; casenum < FILENUM; casenum++) {
        ifstream fin;
        string str;
        // char num_str[10];
        // itoa(casenum, num_str, 10);
        // string case_path = "./benchmark/benchmark/case" + string(num_str) + ".txt";
        std::string case_path = "./benchmark/case" + std::to_string(casenum) + ".txt";
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

        // 初始化，除了源点外，其他点的dist都是inf，前驱都是-1（即没有前驱），都未曾访问过
        for (int i = 0; i < N * N; i++)
            dist[i] = inf, prev[i] = -1, visit[i] = false;
        dist[src_id] = 0, prev[src_id] = src_id, visit[src_id] = true;

        timeval startTime, endTime;
        gettimeofday(&startTime, NULL);
        // clock_t startTime = clock();
        priority_queue<node> pq;
        pq.push(node(src_id, 0));

        while (!pq.empty()) {
            node now = pq.top();  // 队首的元素，是当前队列中cost最小的
            pq.pop();
            int now_id = now.nid, now_cost = now.cost;
            if (now_id == dst_id)
                break;
            int x = now_id / N, y = now_id % N;
            for (int d = 0; d < 4; d++) {
                int xx = x + dx[d], yy = y + dy[d];
                if (xx < 0 || xx >= N || yy < 0 || yy >= N)
                    continue;
                if (maze[xx * N + yy] == BLK || visit[xx * N + yy])
                    continue;
                pq.push(node(xx * N + yy, now_cost + maze[xx * N + yy]));
                visit[xx * N + yy] = true;
                dist[xx * N + yy] = now_cost + maze[xx * N + yy];
                prev[xx * N + yy] = now_id;
            }
        }

        if (dist[dst_id] == inf) {  // 目的点不可达，输出-1
            fout << "case" << casenum << " -1" << endl;
            gettimeofday(&endTime, NULL);
            consume_time[casenum] = (endTime.tv_sec - startTime.tv_sec) + (double)(endTime.tv_usec - startTime.tv_usec) / 1000000.0;
            // consume_time[casenum] = (double)(clock() - startTime) / CLOCKS_PER_SEC;
            continue;
        }

        cout << "case " << casenum << " dist is " << dist[dst_id] << endl;
        fout << "case" << casenum << " " << dist[dst_id] << " ";
        // 根据prev数组，从目的点倒着往回寻路
        vector<int> path;
        path.push_back(dst_id);
        int now_nd = dst_id;
        while(prev[now_nd] != now_nd) {  // 源点的prev被我们设置成了它本身
            path.push_back(prev[now_nd]);
            now_nd = prev[now_nd];
        }

        gettimeofday(&endTime, NULL);
        consume_time[casenum] = (endTime.tv_sec - startTime.tv_sec) + (double)(endTime.tv_usec - startTime.tv_usec) / 1000000.0;
        cout << "case " << casenum << " finish, time is " << consume_time[casenum] << endl;
        // clock_t endTime = clock();
        // consume_time[casenum] = (double)(endTime - startTime) / CLOCKS_PER_SEC;
        int sz = path.size();

        for (int i = sz - 1; i >= 0; i--) {  // 把path数组反着输出，就是一条从源点到目的点的路
            fout << "(" << path[i] / N << ", " << path[i] % N << ") ";
        }
        fout << endl;
        fin.close();
    }
    
    free(maze);
    free(dist);
    free(prev);
    free(visit);

    fout.close();
    ofstream fout_t;
    fout_t.open("bfs_seq_time.txt");

    double avg_time = 0.0;
    for (int i = 0; i < FILENUM; i++) {
        avg_time += consume_time[i];
    }
    avg_time = avg_time / FILENUM;
    fout_t << "average time: " << avg_time << endl << endl;
    for (int casenum = 0; casenum < FILENUM; casenum++) {
        fout_t << "case" << casenum << " " << consume_time[casenum] << endl; 
    }

    return 0;
}
