// 用于计算hist数组，统计in_img中各像素的数量
__kernel void init_hist(__global unsigned char *in_img,
                        __global unsigned int *hist,
                        unsigned int img_size)
{
    int gid = get_global_id(0);
    if (gid < img_size) {
        unsigned int pixel = in_img[gid];
        atomic_inc(&hist[pixel]);  // 用原子操作来对hist[pixel]加1，防止并发访问导致错误
    }
}

// 计算hist的前缀和，结果填入cdf数组，与串行代码不同，这里的前缀和是数量，而不是概率
// local_mem用于计算前缀和
__kernel void calc_cdf(__global unsigned int *hist,
                       __global unsigned int *cdf,
                       __local unsigned int *local_mem)
{
    int g_size = get_global_size(0);
    int l_size = get_local_size(0);
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    
    // 以下都是Bleeloch scan算法，作用是计算前缀和
    local_mem[lid] = hist[gid];
    unsigned int offset = 1;
    for (int step = l_size >> 1; step > 0; step >>= 1) {
        barrier(CLK_LOCAL_MEM_FENCE);
        
        if (lid < step) {
            int i = offset * (2 * lid + 1) - 1;
            int j = offset * (2 * lid + 2) - 1;
            local_mem[j] += local_mem[i];
        }
        offset <<= 1;
    }

    barrier(CLK_LOCAL_MEM_FENCE);
    if (lid == 0) 
        local_mem[l_size - 1] = 0;
    
    for (int step = 1; step < l_size; step <<= 1) {
        offset >>= 1;
        barrier(CLK_LOCAL_MEM_FENCE);

        if (lid < step) {
            int i = offset * (2 * lid + 1) - 1;
            int j = offset * (2 * lid + 2) - 1;

            unsigned int tmp = local_mem[i];
            local_mem[i] = local_mem[j];
            local_mem[j] += tmp;
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE); 
    // 把local_mem里的最终结果写入cdf数组
    if (gid < g_size - 1)
        cdf[gid] = local_mem[lid + 1];
    if (lid == l_size - 1)
        cdf[gid] = cdf[gid - 1] + hist[gid];
}

// 根据cdf，调整in_img中每个点的像素，写入out_img中
__kernel void final_img(__global unsigned char *in_img,
                        __global unsigned char *out_img,
                        __global unsigned int *cdf,
                        unsigned int img_size)
{
    int gid = get_global_id(0);
    if (gid < img_size) {
        unsigned int pixel = in_img[gid];
        int ans = (int)((255.0 * cdf[pixel] / img_size) + 0.5);  // 除以img_size是为了把cdf转换成概率
        out_img[gid] = ans;
    }
}
