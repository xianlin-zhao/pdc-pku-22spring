# 课程项目：二维迷宫寻路问题的并行算法

问题描述：在一个矩阵中，有源点和目的点，除了代表障碍物的格子之外，经过其他格子都会付出一定的代价（本题为1-5的整数），需要找到从源点到目的点的最小代价路径

难点在于：矩阵的大小为10000*10000，需要尽力降低时间和空间开销

## 文件说明

共实现了三种算法：BFS, delta-stepping, Bellman-Ford。后两种分别用openmp和cuda实现了并行版本

* **bfs_seq.cpp**  串行BFS
* **delta_seq.cpp**  串行delta-stepping
* **delta_para.cpp**  并行delta-stepping（openmp实现）
* **BF_seq.cpp**  串行Bellman-Ford
* **BF_para.cu**  并行Bellman-Ford（cuda实现）
* **thread_safe_vector.h**  delta_para.cpp所需的头文件，是一个线程安全的vector库

``make``可编译所有文件

测试数据集过大，这里没有提供。数据集的输入格式详见课程项目说明文档

## 算法说明

### BFS

串行的实现较为容易，但比较难以改为并行版本

### delta-stepping

vector并不是线程安全的，同时push_back、erase会出现竞争问题

我试图查阅是否有能替代vector的线程安全容器，并没有查到。但搜索到了开源代码：https://github.com/tashaxing/thread_safe_stl。这是用C++11基于标准STL封装的、加锁的线程安全版本的STL库，它保持了变量名和调用接口的一致。我使用了这个库中的`thread_safe_vector.h`

然而，并行版本的效果非常不好，甚至比串行版本慢

### Bellman-Ford

每次循环中都检查每一条边。在这个问题中，“边”其实是和“点”完全挂钩的，我们可以检查每一个点，其不同方向就代表不同的边

**注意：**串行版本的时间复杂度很高，在10000*10000的矩阵规模下，有限的时间内无法得到所有测试样例的结果

并行版本使用cuda（参考的cuda教程：https://blog.csdn.net/chongbin007/category_11706610.html）

加速比超过**130**

