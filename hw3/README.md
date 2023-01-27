# hw3

OpenCL加速

编译指令：``make``

运行指令：``./[xxx]``

使用课程服务器

若要改问题规模或工作组大小，需要在cpp文件里修改

## 灰度图直方图均衡化

课程提供的串行代码为``cpu_histeq.cpp``

并行代码为``histeq.cpp``，其kernel函数为``hist.cl``

## 图像锐化

本质上就是对图像进行卷积操作

课程提供的串行代码为``cpu_sharpen.cpp``

并行代码为``sharpen.cpp``，其kernel函数为``conv.cl``