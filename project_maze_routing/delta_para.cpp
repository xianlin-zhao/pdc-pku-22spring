#include <iostream>
#include <fstream>
#include <omp.h>
#include <string>
#include <vector>
#include <ctime>
#include <algorithm>
#include <cmath>
#include "thread_safe_vector.h"
#include "stddef.h"

// using namespace std;

#define N 10000 
#define BLK -1
#define inf 1 << 30
#define FILENUM 5
#define DELTA 1
#define THREAD_NUM 16  // 线程个数，可调
#define NBUCKET 13

double consume_time[FILENUM]{};

struct req {
    int w, r_cost, v;
    req(int ww, int cc, int vv): w(ww), r_cost(cc), v(vv){}
};

thread_safe::vector<int> B[10 * N];
thread_safe::vector<int>::iterator it;
int max_bucket = 0;
int *dist;
int *pre;
short *maze;
// omp_lock_t locks[NBUCKET];

bool allBempty()
{
    int ii = 0;
    for (ii = 0; ii <= max_bucket; ii++) {
        if (!B[ii].empty())
            return false;
    }
    printf("break main loop, max_bucket + 1 = %d\n", ii);  // DEBUG
    return true; 
}

void relax(int w, int cost, int v)
{
    // omp_set_lock(&locks[w % NBUCKET]);
    if (cost < dist[w]) {
        if (dist[w] != inf) {
            it = find(B[dist[w] / DELTA].begin(), B[dist[w] / DELTA].end(), w);
            if (it != B[dist[w] / DELTA].end()) {
                B[dist[w] / DELTA].erase(it);
            }
        }
        B[cost / DELTA].push_back(w);
        max_bucket = std::max(max_bucket, cost / DELTA);
        // #pragma omp critical 
        // max_bucket = std::max(max_bucket, cost / DELTA);
        dist[w] = cost, pre[w] = v;
    }
    // omp_unset_lock(&locks[w % NBUCKET]);
}

int main()
{
    std::ofstream fout;
    fout.open("result.txt");
    int dx[4] = {-1, 0, 1, 0};
    int dy[4] = {0, -1, 0, 1};

    dist = (int *) malloc(sizeof(int) * N * N);
    pre = (int *) malloc(sizeof(int) * N * N);
    maze = (short *) malloc(sizeof(short) * N * N);
    thread_safe::vector<req> R;
    thread_safe::vector<int> S;
    R.reserve(5 * N);
    S.reserve(5 * N);
    // for (int i = 0; i < NBUCKET; i++) {
    //     omp_init_lock(&locks[i]);
    // }

    for (int casenum = 0; casenum < FILENUM; casenum++) {
        std::ifstream fin;
        std::string str;
        // char num_str[10];
        // itoa(casenum, num_str, 10);
        // string case_path = "./benchmark/benchmark/case" + string(num_str) + ".txt";
        std::string case_path = "./benchmark/case" + std::to_string(casenum) + ".txt";
        fin.open(case_path);
        if (fin.is_open())
            std::cout << "open success " << casenum << std::endl;  // DEBUG
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
        int bi = 0;
        bool Bempty = false;
        bool biempty = false;
        // clock_t startTime = clock();
        double startTime = omp_get_wtime();
        // omp_set_num_threads(THREAD_NUM);
        #pragma omp parallel num_threads(THREAD_NUM)
        {
            // 初始化的循环都是可以并行的
            #pragma omp for
            for (int i = 0; i < N * N; i++) {
                dist[i] = inf, pre[i] = -1;
            }
            #pragma omp for
            for (int i = 0; i < 10 * N; i++) {
                B[i].clear();
                B[i].reserve(5 * N);
            }
            #pragma omp single
            {
                relax(src_id, 0, src_id);
                std::cout << "init stage complete" << std::endl;
            }

            // 各种clear操作都只能由1个线程来做
            while (!Bempty) {
                #pragma omp single
                {
                    // std::cout << "now bi " << bi << "and bi size " << B[bi].size() << std::endl;
                    S.clear();
                }
                while (!biempty) {
                    #pragma omp single
                    {
                        R.clear();
                    }
                    #pragma omp for
                    for (int i = 0; i < B[bi].size(); i++) {
                        int node_v = B[bi][i];
                        int v_x = node_v / N, v_y = node_v % N;
                        for (int j = 0; j < 4; j++) {
                            int xx = v_x + dx[j], yy = v_y + dy[j];
                            if (xx < 0 || xx >= N || yy < 0 || yy >= N)
                                continue;
                            if (maze[xx * N + yy] == BLK)
                                continue;
                            if (maze[xx * N + yy] <= DELTA) {
                                // #pragma omp critical
                                R.push_back(req(xx * N + yy, dist[node_v] + maze[xx * N + yy], node_v));
                            }
                        }
                        // #pragma omp critical
                        S.push_back(node_v);
                    }

                    // 按说对R中的松弛操作应该是并行的，但又涉及到加锁解锁的问题，对性能影响过大，干脆就串行了
                    #pragma omp single
                    {
                        B[bi].clear();
                        for (int i = 0; i < R.size(); i++) {
                            relax(R[i].w, R[i].r_cost, R[i].v);
                        }
                        biempty = B[bi].empty();
                    }
                    // std::cout << "empty? " << biempty << std::endl;
                }

                #pragma omp single
                {
                    R.clear();
                    // std::cout << "R clear. bi Ssize " << bi << " " << S.size() << std::endl;
                }

                #pragma omp for
                for (int i = 0; i < S.size(); i++) {
                    int node_v = S[i];
                    int v_x = node_v / N, v_y = node_v % N;
                    for (int j = 0; j < 4; j++) {
                        int xx = v_x + dx[j], yy = v_y + dy[j];
                            if (xx < 0 || xx >= N || yy < 0 || yy >= N)
                                continue;
                            if (maze[xx * N + yy] == BLK)
                                continue;
                            if (maze[xx * N + yy] > DELTA) {
                                // #pragma omp critical
                                R.push_back(req(xx * N + yy, dist[node_v] + maze[xx * N + yy], node_v));
                            }
                    }
                }

                // 这里本来应该是并行，但性能差，还是干脆用串行
                #pragma omp single
                {
                    // std::cout << "fin bi Rsize " << bi << " " << R.size() << std::endl;
                    for (int i = 0; i < R.size(); i++) {
                        relax(R[i].w, R[i].r_cost, R[i].v);
                    }
                    bi++;
                    Bempty = allBempty();
                    biempty = B[bi].empty();
                    // std::cout << "wowow" << std::endl;
                }
                
            }

        }


        std::cout << "delta finish, dist is " << dist[dst_id] << std::endl;  // DEBUG
        if (dist[dst_id] == inf) {
            fout << "case" << casenum << " -1" << std::endl;
            // consume_time[casenum] = (double)(clock() - startTime) / CLOCKS_PER_SEC;
            consume_time[casenum] = omp_get_wtime() - startTime;
            continue;
        }

        fout << "case" << casenum << " " << dist[dst_id] << " ";
        std::vector<int> path;
        path.push_back(dst_id);
        int now_nd = dst_id;
        while(pre[now_nd] != now_nd) {
            path.push_back(pre[now_nd]);
            now_nd = pre[now_nd];
        }

        // clock_t endTime = clock();
        double endTime = omp_get_wtime();
        // consume_time[casenum] = (double)(endTime - startTime) / CLOCKS_PER_SEC;
        consume_time[casenum] = endTime - startTime;
        int sz = path.size();

        for (int i = sz - 1; i >= 0; i--) {
            fout << "(" << path[i] / N << ", " << path[i] % N << ") ";
        }
        fout << std::endl;
        fin.close();
    }
    
    free(maze);
    free(dist);
    free(pre);

    fout.close();
    std::ofstream fout_t;
    fout_t.open("delta_para_time.txt");

    double avg_time = 0.0;
    for (int i = 0; i < FILENUM; i++) {
        avg_time += consume_time[i];
    }
    avg_time = avg_time / FILENUM;
    fout_t << "average time: " << avg_time << std::endl << std::endl;
    for (int casenum = 0; casenum < FILENUM; casenum++) {
        fout_t << "case" << casenum << " " << consume_time[casenum] << std::endl; 
    }
    fout_t.close();

    return 0;
}
