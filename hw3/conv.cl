// 四个参数为：输入图像、卷积核、卷积核的边长、输出图像
__kernel void conv(__global float *img_in,
                   __global float *f_in,
                   int fil_size,
                   __global float *out)
{
    int out_size, in_size;
    int x, y;
    float sum = 0.0;

    out_size = get_global_size(0);
    x = get_global_id(1);
    y = get_global_id(0);  // 这个work item负责计算out[x][y]
    in_size = out_size + fil_size - 1;  // 根据输出图像的边长和卷积核的边长，计算输入图像的边长

    for (int i = 0; i < fil_size; i++)
        for (int j = 0; j < fil_size; j++)
            sum += f_in[i * fil_size + j] * img_in[(x + i) * in_size + y + j];
    
    out[x * out_size + y] = sum;
}
