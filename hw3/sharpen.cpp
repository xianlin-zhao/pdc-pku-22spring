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

const int fil_size = 3;
const int out_size = 2048;  // 输出图像的边长，可调整
const int in_size = out_size + fil_size - 1;
const int work_group_size = 32;  // 工作组大小，可调整

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

    commandQueue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, NULL);

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
                                        NULL, NULL);

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

    return program;
}

void Cleanup(cl_context context, cl_command_queue commandQueue,
             cl_program program, cl_kernel kernel, cl_mem memObjects[3])  // 回收清理资源
{
    for (int i = 0; i < 3; i++)
    {
        if (memObjects[i] != 0)
            clReleaseMemObject(memObjects[i]);
    }
    if (commandQueue != 0)
        clReleaseCommandQueue(commandQueue);

    if (kernel != 0)
        clReleaseKernel(kernel);

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
    cl_kernel kernel = 0;
    cl_mem memObjects[3] = {0, 0, 0};
    cl_int errNum;

    const char* filename = "./conv.cl";
    context = CreateContext();

    commandQueue = CreateCommandQueue(context, &device);

    program = CreateProgram(context, device, filename);

    kernel = clCreateKernel(program, "conv", NULL);  // kernel函数，作用是对输入图像进行卷积，通过特殊的卷积核实现图像锐化

    float img[in_size * in_size] = {};  // 输入图像
    float out[out_size * out_size] = {};  // 输出图像
    float f[fil_size * fil_size] = {1, 1, 1, 1, -9, 1, 1, 1, 1};  // 卷积核，是题目要求的
    
    for(int i = 0; i < in_size; ++i)
        for(int j = 0; j < in_size; ++j)
            img[i * in_size + j] = i + j;  // 初始化，和串行代码一样
    
    // 三个内存对象，分别是输入图像、卷积核、输出图像
    memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * in_size * in_size, NULL, NULL);
    memObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * fil_size * fil_size, NULL, NULL);
    memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * out_size * out_size, NULL, NULL);
    
    clEnqueueWriteBuffer(commandQueue, memObjects[0], CL_TRUE, 0, sizeof(float) * in_size * in_size, 
                        img, 0, NULL, NULL);
    clEnqueueWriteBuffer(commandQueue, memObjects[1], CL_TRUE, 0, sizeof(float) * fil_size * fil_size, 
                        f, 0, NULL, NULL);
    
    errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &memObjects[0]);  // 输入图像
    errNum |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &memObjects[1]);  // 卷积核
    errNum |= clSetKernelArg(kernel, 2, sizeof(int), &fil_size);  // 卷积核的边长
    errNum |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &memObjects[2]);  // 输出图像
    
    size_t globalWorkSize[2] = {out_size, out_size};  // 全局的，大小就是图像大小
    size_t localWorkSize[2] = {work_group_size, work_group_size};  // 工作组大小，可以调整
    
    cl_event event;
    errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, &event);  // 绑定event，用于计时
    clWaitForEvents(1, &event);
    clFinish(commandQueue);
    cl_ulong time_start;
    cl_ulong time_end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
    unsigned long elapsed = (unsigned long)(time_end - time_start);  // 时间单位是纳秒

    errNum = clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE,
                                 0, sizeof(float) * out_size * out_size, out,
                                 0, NULL, NULL);  // 最终写回out数组中，也就是输出图像
    
    cout << "finish! time elapsed: " << (double)elapsed / 1000000.0 << endl;
    Cleanup(context, commandQueue, program, kernel, memObjects);
    return 0;
}
