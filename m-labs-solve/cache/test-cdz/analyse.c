#include<stdio.h>

#define TAG(addr,s,b) addr>>(s+b)
#define SET(addr,s,b) ({                     \
            unsigned int tag=TAG(addr,s,b);  \
            tag=tag<<s;                      \
            unsigned int tag_set=addr>>b;    \
            tag_set^tag;                     \
        })


// 计算一个矩阵中各元素映射到哪一个cache行
// 矩阵原本256x256, 使用时只需要其前row行,前col列; addr是矩阵的内存地址
void mapCacheLine(unsigned int addr,int row, int col){
    for(unsigned i=0;i<row;i++){
        for(unsigned j=0;j<col;j++){
            unsigned int taddr=addr+i*4*col+j*4;        //一个元素4byte,乘以4!!!
            printf("%2d ",SET(taddr,5,5));
        }
        printf("\n");
    }
}


// 12.08 => 不能这样计算,虽然A、B最初是256x256,但是当传入转置函数时,已经被解释成了MXN的数组!!!

int main(){
    printf("map A:\n");
    mapCacheLine(0x6031a0,64,64);

    // printf("map B:\n");
    // mapCacheLine(0x6431a0,32,32);
    return 0;
}