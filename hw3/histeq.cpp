#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.h>
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/time.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>

using namespace std;

const unsigned int img_size = 4096 * 4096;  // 数组大小，可调整

const int work_group_size = 16 * 16;  // 工作组大小，可调整

cl_context CreateContext()  // 获得context
{
    cl_int errNum;
    cl_uint numPlatforms;
    cl_platform_id firstPlatformId;
    cl_context context = NULL;

    errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
    if (errNum != CL_SUCCESS || numPlatforms <= 0)
    {
        std::cout << "Failed to find any OpenCL platforms." << std::endl;
        return NULL;
    }

    cl_context_properties contextProperties[] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)firstPlatformId,
        0
    };
    context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,
                                      NULL, NULL, &errNum);

    return context;
}

cl_command_queue CreateCommandQueue(cl_context context, cl_device_id *device)  // 创建command queue
{
    cl_int errNum;
    cl_device_id *devices;
    cl_command_queue commandQueue = NULL;
    size_t deviceBufferSize = -1;

    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);

    if (deviceBufferSize <= 0)
    {
        std::cout << "No devices available.";
        return NULL;
    }

    devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
    errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);

    commandQueue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, NULL);  // 第三个参数是为了计时

    *device = devices[0];
    delete[] devices;
    return commandQueue;
}

cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName)  // 创建并编译kernel程序
{
    cl_int errNum;
    cl_program program;

    std::ifstream kernelFile(fileName, std::ios::in);
    if (!kernelFile.is_open())
    {
        std::cout << "Failed to open file for reading: " << fileName << std::endl;
        return NULL;
    }

    std::ostringstream oss;
    oss << kernelFile.rdbuf();

    std::string srcStdStr = oss.str();
    const char *srcStr = srcStdStr.c_str();
    program = clCreateProgramWithSource(context, 1,
                                        (const char**)&srcStr,
                                        NULL, NULL);  // 程序里可能有多个kernel函数

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

    return program;
}

void Cleanup(cl_context context, cl_command_queue commandQueue,
             cl_program program, cl_kernel kernel[3], cl_mem memObjects[4])  // 回收清理资源
{
    for (int i = 0; i < 4; i++)
    {
        if (memObjects[i] != 0)
            clReleaseMemObject(memObjects[i]);
    }
    
    if (commandQueue != 0)
        clReleaseCommandQueue(commandQueue);

    for (int i = 0; i < 3; i++)
    {
        if (kernel[i] != 0)
            clReleaseKernel(kernel[i]);
    }

    if (program != 0)
        clReleaseProgram(program);

    if (context != 0)
        clReleaseContext(context);
}

int main(int argc, char** argv)
{
    cl_context context = 0;
    cl_command_queue commandQueue = 0;
    cl_program program = 0;
    cl_device_id device = 0;
    cl_kernel kernel[3] = {0, 0, 0};
    cl_mem memObjects[4] = {0, 0, 0, 0};
    cl_int errNum;

    const char* filename = "./hist.cl";
    context = CreateContext();

    commandQueue = CreateCommandQueue(context, &device);

    program = CreateProgram(context, device, filename);

    unsigned char in_img[img_size] = {};  // 输入图片，值为0~255的整数
    unsigned char out_img[img_size] = {};  // 调整后的图片
    for(int i = 0; i < img_size; ++i)
        in_img[i] = (i + 10) % 256;

    memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   sizeof(unsigned char) * img_size, in_img, NULL);  // 输入图片
    memObjects[1] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * 256, NULL, NULL);  // 对应串行代码中的hist数组
    
    // 作用与串行代码中的cdf类似，但这里的cdf并不是概率的前缀和，而是数量的前缀和（如果要算概率，除以img_size即可）
    memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * 256, NULL, NULL);
    memObjects[3] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned char) * img_size, NULL, NULL);  // 调整后的图片

    kernel[0] = clCreateKernel(program, "init_hist", NULL);  // 第一个kernel函数，用于统计各像素的数量，填hist
    errNum = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), &memObjects[0]);  // 输入图像
    errNum |= clSetKernelArg(kernel[0], 1, sizeof(cl_mem), &memObjects[1]);  // hist
    errNum |= clSetKernelArg(kernel[0], 2, sizeof(unsigned int), &img_size);  // 图像大小

    kernel[1] = clCreateKernel(program, "calc_cdf", NULL);  // 第二个kernel函数，用于计算数量的前缀和，即cdf（先不算概率）
    errNum = clSetKernelArg(kernel[1], 0, sizeof(cl_mem), &memObjects[1]);  // hist
    errNum |= clSetKernelArg(kernel[1], 1, sizeof(cl_mem), &memObjects[2]);  // cdf
    errNum |= clSetKernelArg(kernel[1], 2, sizeof(unsigned int) * 256, NULL);  // local memory

    kernel[2] = clCreateKernel(program, "final_img", NULL);  // 第三个kernel函数，用于根据cdf，调整原图像
    errNum = clSetKernelArg(kernel[2], 0, sizeof(cl_mem), &memObjects[0]);  // 输入图像
    errNum |= clSetKernelArg(kernel[2], 1, sizeof(cl_mem), &memObjects[3]);  // 调整后的图像
    errNum |= clSetKernelArg(kernel[2], 2, sizeof(cl_mem), &memObjects[2]);  // cdf
    errNum |= clSetKernelArg(kernel[2], 3, sizeof(unsigned int), &img_size);  // 图像大小

    cl_ulong time_start;
    cl_ulong time_end;
    unsigned long elapsed[3] = {0};  // 三个kernel函数的时间分别统计

    // 全局、局部work group的大小
    size_t globalWorkSize[1] = {img_size};
    size_t localWorkSize[1] = {work_group_size};
    cl_event event1;  // 绑定事件，计时用
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[0], 1, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, &event1);
    clWaitForEvents(1, &event1);
    clGetEventProfilingInfo(event1, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event1, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
    elapsed[0] = (unsigned long)(time_end - time_start);  // 计时的操作就是这样，下面都是重复的步骤

    globalWorkSize[0] = 256;  // 这个kernel只负责算前缀和，数组大小是256，就都设置为256
    localWorkSize[0] = 256;
    cl_event event2;
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[1], 1, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, &event2);
    clWaitForEvents(1, &event2);
    clGetEventProfilingInfo(event2, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event2, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
    elapsed[1] = (unsigned long)(time_end - time_start);

    globalWorkSize[0] = img_size;
    localWorkSize[0] = work_group_size;
    cl_event event3;
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel[2], 1, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, &event3);
    clWaitForEvents(1, &event3);
    clGetEventProfilingInfo(event3, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event3, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
    elapsed[2] = (unsigned long)(time_end - time_start);

    errNum = clEnqueueReadBuffer(commandQueue, memObjects[3], CL_TRUE,
                                 0, sizeof(unsigned char) * img_size, out_img,
                                 0, NULL, NULL);  // 最终把内存对象的结果写回out_img中，得到调整后的图像
    
    Cleanup(context, commandQueue, program, kernel, memObjects);  // 释放各种资源

    cout << "finish! " << elapsed[0] / 1000000.0 << " " << elapsed[1] / 1000000.0 << " " << elapsed[2] / 1000000.0 << endl;
    cout << "all time: " << (elapsed[0] + elapsed[1] + elapsed[2]) / 1000000.0 << endl;
    return 0;
}
