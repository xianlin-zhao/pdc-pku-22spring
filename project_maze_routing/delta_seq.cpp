#include <iostream>
#include <fstream>
#include <omp.h>
#include <string>
#include <vector>
#include <sys/time.h>
#include <algorithm>
#include <cmath>

using namespace std;

#define N 10000
#define BLK -1
#define inf 1 << 30
#define FILENUM 5
#define DELTA 1

double consume_time[FILENUM]{};

struct req {  // 即requests，w是到达点的编号，r_cost是新计算出来的代价，v是过来的点，用于更新prev
    int w, r_cost, v;
    req(int ww, int cc, int vv): w(ww), r_cost(cc), v(vv){}
};

vector<int> B[10 * N];  // B[i]是第i个桶，当delta=1时，所需的桶不超过30000个
vector<int>::iterator it;
int max_bucket = 0;
int *dist;
int *pre;
short *maze;
int max_Rsize = 0, max_Ssize = 0, max_Bsize = 0;  // DEBUG

void relax(int w, int cost, int v)  // 松弛操作
{
    if (cost < dist[w]) {
        if (dist[w] != inf) {
            it = find(B[dist[w] / DELTA].begin(), B[dist[w] / DELTA].end(), w);
            if (it != B[dist[w] / DELTA].end()) {
                B[dist[w] / DELTA].erase(it);  // 从原来的桶里删除
            }
        }
        B[cost / DELTA].push_back(w);  // 根据cost算出新的桶，加进去
        max_bucket = max(max_bucket, cost / DELTA);  // 更新最大桶编号
        dist[w] = cost, pre[w] = v;  // 更新距离和前驱
    }
}

int main()
{
    ofstream fout;
    fout.open("result.txt");
    int dx[4] = {-1, 0, 1, 0};
    int dy[4] = {0, -1, 0, 1};

    dist = (int *) malloc(sizeof(int) * N * N);
    pre = (int *) malloc(sizeof(int) * N * N);
    maze = (short *) malloc(sizeof(short) * N * N);
    vector<req> R;
    vector<int> S;

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
        // cout << "maze constructed " << casenum << endl;

        max_bucket = 0;
        timeval startTime, endTime;
        gettimeofday(&startTime, NULL);
        // clock_t startTime = clock();

        for (int i = 0; i < N * N; i++) {
            dist[i] = inf, pre[i] = -1;
        }
        for (int i = 0; i < 10 * N; i++) {
            B[i].clear();  // 初始把每个桶都清空
        }
        relax(src_id, 0, src_id);  // 把源点加入第0个桶
        int bi = 0;
        while (1) {
            int ii = 0;
            for (ii = 0; ii <= max_bucket; ii++) {  // 先判断是否所有桶都空了
                if (!B[ii].empty())
                    break;
            }
            if (ii == max_bucket + 1) {  // 所有桶都空了，结束
                cout << "break main loop, max_bucket + 1 = " << ii << endl;  // DEBUG
                break;
            } 
            
            S.clear();
            max_Bsize = max(max_Bsize, (int) B[bi].size());  // DEBUG
            while (!B[bi].empty()) {
                R.clear();
                for (int i = 0; i < B[bi].size(); i++) {  // 考察bi号桶内的所有点
                    int node_v = B[bi][i];
                    int v_x = node_v / N, v_y = node_v % N;
                    for (int j = 0; j < 4; j++) {  // 往四个方向寻找
                        int xx = v_x + dx[j], yy = v_y + dy[j];
                        if (xx < 0 || xx >= N || yy < 0 || yy >= N)
                            continue;
                        if (maze[xx * N + yy] == BLK)
                            continue;
                        if (maze[xx * N + yy] <= DELTA) {  // 轻边，就创造requests
                            R.push_back(req(xx * N + yy, dist[node_v] + maze[xx * N + yy], node_v));
                        }
                    }
                    S.push_back(node_v);  // S = S∪B[bi]
                }
                max_Rsize = max(max_Rsize, (int) R.size());  // DEBUG
                B[bi].clear();  // bi号桶清空
                for (int i = 0; i < R.size(); i++) {  // 对刚才加进去的所有requests做松弛操作
                    relax(R[i].w, R[i].r_cost, R[i].v);
                }
            }
            R.clear();
            
            max_Ssize = max(max_Ssize, (int) S.size());  // DEBUG
            for (int i = 0; i < S.size(); i++) {
                int node_v = S[i];
                int v_x = node_v / N, v_y = node_v % N;
                for (int j = 0; j < 4; j++) {
                    int xx = v_x + dx[j], yy = v_y + dy[j];
                        if (xx < 0 || xx >= N || yy < 0 || yy >= N)
                            continue;
                        if (maze[xx * N + yy] == BLK)
                            continue;
                        if (maze[xx * N + yy] > DELTA) {  // 找所有重边，加入requests集合
                            R.push_back(req(xx * N + yy, dist[node_v] + maze[xx * N + yy], node_v));
                        }
                }
            }
            max_Rsize = max(max_Rsize, (int) R.size());  // DEBUG
            for (int i = 0; i < R.size(); i++) {
                relax(R[i].w, R[i].r_cost, R[i].v);  // 松弛
            }
            bi++;
        }


        cout << "delta finish, dist is " << dist[dst_id] << endl;  // DEBUG
        if (dist[dst_id] == inf) {
            fout << "case" << casenum << " -1" << endl;
            gettimeofday(&endTime, NULL);
            consume_time[casenum] = (endTime.tv_sec - startTime.tv_sec) + (double)(endTime.tv_usec - startTime.tv_usec) / 1000000.0;
            // consume_time[casenum] = (double)(clock() - startTime) / CLOCKS_PER_SEC;
            continue;
        }

        fout << "case" << casenum << " " << dist[dst_id] << " ";
        vector<int> path;
        path.push_back(dst_id);
        int now_nd = dst_id;
        while(pre[now_nd] != now_nd) {
            path.push_back(pre[now_nd]);
            now_nd = pre[now_nd];
        }

        // clock_t endTime = clock();
        // consume_time[casenum] = (double)(endTime - startTime) / CLOCKS_PER_SEC;
        gettimeofday(&endTime, NULL);
        consume_time[casenum] = (endTime.tv_sec - startTime.tv_sec) + (double)(endTime.tv_usec - startTime.tv_usec) / 1000000.0;
        int sz = path.size();

        for (int i = sz - 1; i >= 0; i--) {
            fout << "(" << path[i] / N << ", " << path[i] % N << ") ";
        }
        fout << endl;
        fin.close();
    }
    
    free(maze);
    free(dist);
    free(pre);

    fout.close();
    ofstream fout_t;
    fout_t.open("delta_seq_time.txt");

    double avg_time = 0.0;
    for (int i = 0; i < FILENUM; i++) {
        avg_time += consume_time[i];
    }
    avg_time = avg_time / FILENUM;
    fout_t << "average time: " << avg_time << endl << endl;
    for (int casenum = 0; casenum < FILENUM; casenum++) {
        fout_t << "case" << casenum << " " << consume_time[casenum] << endl; 
    }
    fout_t.close();
    cout << "R " << max_Rsize << " S " << max_Ssize << " B " << max_Bsize << endl; // DEBUG

    return 0;
}
