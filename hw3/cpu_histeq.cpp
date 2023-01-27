#include <cstdio>
void cpu_histeq(unsigned char* in_img, unsigned char* out_img, unsigned int size) {
    unsigned int hist[256] = {};  
    for(int i = 0; i < size; ++i) {  
        unsigned int pixel = in_img[i];  
        hist[pixel]++;  
    }
    float pdf[256] = {};  
    for(int i = 0; i < 256; ++i) {  
        pdf[i] = hist[i] * 1.0 / size;  
    }  
    float cdf[256] = {0};
    for(int i = 0; i < 256; ++i) {  
        if (i == 0) 
            cdf[i] = pdf[i];  
        else 
            cdf[i] = cdf[i-1] + pdf[i];  
    }
    int _map[256] = {0};  
    for (int i = 0; i < 256; ++i) {  
        _map[i] = (int)(255.0 * cdf[i] + 0.5);  
    }
    for (int i = 0; i < size; ++i) {  
        unsigned int pixel = in_img[i];  
        out_img[i] = _map[pixel];  
    }  
}

int main() {
    unsigned char in_img[16]  = {};
    unsigned char out_img[16] = {};
    for(int i = 0; i < 16; ++i) {
        in_img[i] = i + 10;
    }
    cpu_histeq(in_img, out_img, 16);
    for(int i = 0; i < 16; ++i) {
        printf("%d %d\n", in_img[i], out_img[i]);
    }
}
	