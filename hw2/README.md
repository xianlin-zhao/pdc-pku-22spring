# hw2

MPI编程题

编译指令：可直接``make``，也可用``mpic++ -o [xxx] [xxx].cpp ``

运行指令：``mpiexec -n [number of threads] ./[xxx]``

使用课程服务器。本地配置环境较为困难，所以没有尝试

若要修改问题规模，需在源码中修改

## PSRS排序算法

hw1中排序算法的MPI实现，见``psrs.cpp``

## 并行卷积计算

基础串行版本见``convolution.cpp``，并行版本为``conv.cpp``

