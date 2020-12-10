/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

#define TAG(addr,s,b) addr>>(s+b)
#define SET(addr,s,b) ({                     \
            unsigned int tag=TAG(addr,s,b);  \
            tag=tag<<s;                      \
            unsigned int tag_set=addr>>b;    \
            tag_set^tag;                     \
        })

/*************************************************************************************
 *                              基于cache的矩阵转置优化
 * 0.先学习先关讲义:http://www.cs.cmu.edu/afs/cs/academic/class/15213-f15/www/recitations/rec07.pdf
 * 1.cache模拟器默认使用直接映射:s=5,E=1,b=5 => 32组,每组一行,每行32字节
 * 2.registerTransFunction的写法很有借鉴意义
 * 3.必读test-trans.c源码、tracegen.c源码!!!
 * 4.这里只是模拟,对命中、缺失的统计并非实际运行时的情况; 这里的结果只是使用运行时地址与强行
 *      给定的s E b计算所得; 
 * 5.需明白判断命中/缺失与地址中的块内偏移字段无关
 * 6.partB中b的位数很关键,它决定了块内能存储的字节数,从而决定了分块大小
 * 
 * ...
 * n.为什么矩阵分块有效?
 *  => 当数组大小增加时,时间局部性会明显降低,高速缓存中不命中数目增加;
 *     使用分块时,时间局部性由分块大小来决定,而不是数组总大小来决定(子块内的数据短期内可再次被
 *     访问,这时候直接缓存命中;如果不分块,数组又非常大,刚访问过的数据可能很长时间后才再次访问,
 *     而这时已经缺页)
 * n.若有不懂,参考最详细的资料:https://blog.csdn.net/xbb224007/article/details/81103995
 * **********************************************************************************/




int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i,j;
    int n,m;
    int t0,t1,t2,t3,t4,t5,t6,t7;
    if(M==32 && N==32){
        for(i=0;i<N;i+=8){
            for(j=0;j<M;j+=8){
                for(n=i;n<i+8;n++){             //每次将A一个小块的一行数据缓存在局部变量
                    t0=A[n][j];
                    t1=A[n][j+1];
                    t2=A[n][j+2];
                    t3=A[n][j+3];
                    t4=A[n][j+4];
                    t5=A[n][j+5];
                    t6=A[n][j+6];
                    t7=A[n][j+7];
            
                    B[j][n]=t0;               // 写转置结果
                    B[j+1][n]=t1;
                    B[j+2][n]=t2;
                    B[j+3][n]=t3;
                    B[j+4][n]=t4;
                    B[j+5][n]=t5;
                    B[j+6][n]=t6;
                    B[j+7][n]=t7;
                }
            }
        }

    }  
    else if(M==64 && N==64){
        for(i=0;i<N;i+=8){            // A第i行           
            for(j=0;j<M;j+=8){        // A第j列
                /****************************************************************************
                 * 对于内部的的4x4小块,注意对于2,必须A按照列读,B按照行写!!!
                 * **************************************************************************/

                // 1.A的8x8矩阵的左上块,转置到B对应8x8矩阵的左上块
                //   同时,A的8x8矩阵的右上块,转置到B对应8x8矩阵的右上块(暂存)
                //   => 暂存只是为了充分利用B的缓存,减少冲突不命中
                for(n=i;n<i+4;n++){                                           // 对A的前四行
                    t0=A[n][j]; t1=A[n][j+1]; t2=A[n][j+2]; t3=A[n][j+3];
                    t4=A[n][j+4];t5=A[n][j+5];t6=A[n][j+6];t7=A[n][j+7];

                    B[j][n]=t0; B[j+1][n]=t1; B[j+2][n]=t2; B[j+3][n]=t3;
                    B[j][n+4]=t4; B[j+1][n+4]=t5; B[j+2][n+4]=t6; B[j+3][n+4]=t7;
                }

                // 2.A的8x8矩阵的左下块,转置到B对应8x8矩阵的右上块; 同时将B8x8矩阵的右上块暂存的数据放到最终位置
                // 此时与2中暂存在B的一行数据冲突:
                //   => a.先将A中一列放入临时变量t0~t3(A也要缓存到临时变量且A比B先缓存=>解决对角线问题!)
                //      b.将B中会发生冲突的一列放入临时变量t4~t7
                //      c.将临时变量t0~t3放入到B中刚空出的一行(即完成A中一列的转置) 
                //      d.将临时变量t4~t7的数据放入到正确位置(即之前暂存在B中的一行放入正确的位置)
                for(m=j;m<j+4;m++){           // A的第m列 <=> B的第m行; 为了先缓存B的一行到临时变量,所以第A而言需按列读!!!
                    t0=A[i+4][m]; t1=A[i+5][m]; t2=A[i+6][m]; t3=A[i+7][m];        
                    t4=B[m][i+4]; t5=B[m][i+5]; t6=B[m][i+6]; t7=B[m][i+7]; 
                    B[m][i+4]=t0; B[m][i+5]=t1; B[m][i+6]=t2; B[m][i+7]=t3;       // 下面两行不能对换,否则冲突不命中!
                    B[m+4][i]=t4; B[m+4][i+1]=t5; B[m+4][i+2]=t6; B[m+4][i+3]=t7;   
                }

                // 3.A的8x8矩阵的右下块,转置到B对应8x8矩阵的右下块
                for(n=i+4;n<i+8;n++){
                    t0=A[n][j+4]; t1=A[n][j+5]; t2=A[n][j+6]; t3=A[n][j+7];
                    B[j+4][n]=t0; B[j+5][n]=t1; B[j+6][n]=t2; B[j+7][n]=t3;
                }
            }
        }
    }
    else if(M==61 && N==67){
        for(i=0;i<N;i+=17){
            for(j=0;j<M;j+=17){
                for(n=i;n<i+17 && n<N;n++){
                    for(m=j;m<j+17 && m<M;m++){
                        B[m][n]=A[n][m];
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
// =>  hits:870, misses:1183, evictions:1151
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    
}




/*************************** 1. 32x32 **********************************
 * 1.32x32的理论最少miss
 *   => 此时缓存足够,理论上只有冷不命中,没有冲突不命中和容量不命中
 *   对于32x32,可划分为8x8的小块,共4x4=16块
 *   对于每个小块,恰好是8个cache行,所以一个小块的理想miss数是8,进行转置的A B两个
 *   小块共16行,小于cache能提供的32行,所以缓存容量足够
 *   综上,总的理论最少miss为8*(4x4)*2=256,乘2是因为A B两个数组
 * 2.为什么A[i][j]与B[i][j]总是映射到同一个缓存位置?
 *   => 参考tracegen.c知,A[256][256],B[256][256],由于A的大小是缓存字节数的整数倍,
 *   且B挨着A存放,所以B的映射规律其实与A一样(可在A B间插入其他数据,能验证miss情况会
 *   发生改变!).
 * 3.为什么trans_32x32_v2没有达到理想的256次miss?
 *   => A是在每对8个int操作时,会不命中一次,即每个cache行中有且只有一个元素不被命中(第一个);
 *   而B是不仅在开头有不命中(对应A的第一行),在对角线上也有不命中(共32个对角点,每个点miss一cache行)!
 *   如果能把B中的对角线元素命中的话,就可以达到/接近理论上的256次miss
 *   实现细节可参考:https://zhuanlan.zhihu.com/p/79058089
 * *********************************************************************/

// 8x8 => hits:1710, misses:343, evictions:311
char trans_32x32_v1_desc[] = "trans_32x32_v1:simple blocking";
void trans_32x32_v1(int M, int N, int A[N][M], int B[M][N])
{
    // 这里最内层循环,对块a而言恰好是一行,空间局部性高
    // 对块b而言,虽然最开始连续miss 8次(每次都是写每行第一个数),但是之后每列都能命中,空间局部性依然高
    int i,j,m,n;
    if(M==32){
        for(i=0;i<N;i+=8){              // i,j用于确定块
            for(j=0;j<M;j+=8){
                for(n=i;n<i+8;n++){     // n,m用于确定块内数据
                    for(m=j;m<j+8;m++){   // 这个循环对块a而言恰好是一行,空间局部性高,对块b...
                        B[m][n]=A[n][m];  
                    }
                }
            }
        }
    }   
}


// 8x8 => hits:1766, misses:287, evictions:255
char trans_32x32_v2_desc[] = "trans_32x32_v2:blocking+diagnal";
void trans_32x32_v2(int M, int N, int A[N][M], int B[M][N])
{
    // 由于A[i][j]与B[i][j]映射到同一个位置(为什么映射到同一个位置,见1.32x32下备注),
    // 所以在trans_32x32_v1中,对于对角线上的元素,A不命中(换入A) => B不命中(驱逐A) => A再次不命中(驱逐B,这时A取对角线的右边元素)
    // 这一部分造成理想情况之外的不命中,且这种冲突只会发生在处理对角线的块上
    // => 将A中一个小块的一行数据一次全部读完,从而避免A驱逐B后再次被B驱逐的情况 .....
    int i,j,n;
    if(M==32){
        int t0,t1,t2,t3,t4,t5,t6,t7;
        for(i=0;i<N;i+=8){
            for(j=0;j<M;j+=8){
                for(n=i;n<i+8;n++){             //每次将A一个小块的一行数据缓存在局部变量
                    t0=A[n][j];
                    t1=A[n][j+1];
                    t2=A[n][j+2];
                    t3=A[n][j+3];
                    t4=A[n][j+4];
                    t5=A[n][j+5];
                    t6=A[n][j+6];
                    t7=A[n][j+7];
            
                    B[j][n]=t0;               // 写转置结果
                    B[j+1][n]=t1;
                    B[j+2][n]=t2;
                    B[j+3][n]=t3;
                    B[j+4][n]=t4;
                    B[j+5][n]=t5;
                    B[j+6][n]=t6;
                    B[j+7][n]=t7;
                }
            }
        }
    }
    
}


/************************ 2. 64x64 ************************************/
// 8x8 =>  hits:3474, misses:4723, evictions:4691
char trans_64x64_v1_desc[] = "trans_64x64_v1:simple blocking";
void trans_64x64_v1(int M, int N, int A[N][M], int B[M][N])
{
    int i,j,n,m;
    if(M==64){
        for(i=0;i<M;i+=8){
            for(j=0;j<N;j+=8){
                for(n=i;n<i+8;n++){
                    for(m=j;m<j+8;m++){
                        B[m][n]=A[n][m];
                    }
                }
            }
        }
    }    
}

// 8x8 =>  hits:3586, misses:4611, evictions:4579
char trans_64x64_v2_desc[] = "trans_64x64_v2:blocking+diagnal";
void trans_64x64_v2(int M, int N, int A[N][M], int B[M][N])
{
    int i,j,n;
    if(M==64){
        int t0,t1,t2,t3,t4,t5,t6,t7;
        for(i=0;i<N;i+=8){
            for(j=0;j<M;j+=8){
                for(n=i;n<i+8;n++){             //每次将A一个小块的一行数据缓存在局部变量
                    t0=A[n][j];
                    t1=A[n][j+1];
                    t2=A[n][j+2];
                    t3=A[n][j+3];
                    t4=A[n][j+4];
                    t5=A[n][j+5];
                    t6=A[n][j+6];
                    t7=A[n][j+7];
            
                    B[j][n]=t0;               // 写转置结果
                    B[j+1][n]=t1;
                    B[j+2][n]=t2;
                    B[j+3][n]=t3;
                    B[j+4][n]=t4;
                    B[j+5][n]=t5;
                    B[j+6][n]=t6;
                    B[j+7][n]=t7;
                }
            }
        }
    }
     
}


// 4x4 =>  hits:6498, misses:1699, evictions:1667
char trans_64x64_v3_desc[] = "trans_64x64_v3:blocking(4)+diagnal";
void trans_64x64_v3(int M, int N, int A[N][M], int B[M][N])
{
    int i,j,n;
    if(M==64){
        int t0,t1,t2,t3;
        for(i=0;i<N;i+=4){
            for(j=0;j<M;j+=4){
                for(n=i;n<i+4;n++){             //每次将A一个小块的一行数据缓存在局部变量
                    t0=A[n][j];
                    t1=A[n][j+1];
                    t2=A[n][j+2];
                    t3=A[n][j+3];

                    B[j][n]=t0;               // 写转置结果
                    B[j+1][n]=t1;
                    B[j+2][n]=t2;
                    B[j+3][n]=t3;
                }
            }
        }
    }
     
}

// 8x8 =>  hits:8810, misses:1435, evictions:1403 => 没有解决对角线问题
char trans_64x64_v4_desc[] = "trans_64x64_v4:blocking(8)+spaceCompaction ";
void trans_64x64_v4(int M, int N, int A[N][M], int B[M][N])
{
    int i,j;
    int n,m;
    int t0,t1,t2,t3;
    if(M==64){
        for(i=0;i<N;i+=8){            // A第i行           
            for(j=0;j<M;j+=8){        // A第j列
                /****************************************************************************
                 * 对于内部的的4x4小块,A按列读,B按行写
                 * **************************************************************************/

                // 1.A的8x8矩阵的左上块,转置到B对应8x8矩阵的左上块
                for(m=j;m<j+4;m++){         // A的第m列
                    for(n=i;n<i+4;n++){     // A的第n行
                        B[m][n]=A[n][m];
                    }
                }

                // 2.A的8x8矩阵的右上块,转置到B对应8x8矩阵的右上块(暂存)
                //   => 暂存只是为了充分利用B的缓存,减少冲突不命中
                for(m=j+4;m<j+8;m++){           // A的第m列
                    for(n=i;n<i+4;n++){     // A的第n行
                        B[m-4][n+4]=A[n][m];
                    }
                }

                // 3.A的8x8矩阵的左下块,转置到B对应8x8矩阵的右上块; 同时将B8x8矩阵的右上块暂存的数据放到最终位置
                // 此时与2中暂存的数据冲突:
                //   => a.先将B中冲突的一行数据缓存到临时变量
                //      b.将A的中一列写到B中已缓存临时变量的一行
                //      c.将已缓存到临时变量的数据放置到B中最终正确的位置上
                for(m=j;m<j+4;m++){           // A的第m列 => B的第m行
                    n=i+4;
                    t0=B[m][n]; t1=B[m][n+1]; t2=B[m][n+2]; t3=B[m][n+3];                           //a
                    B[m][n]=A[n][m]; B[m][n+1]=A[n+1][m]; B[m][n+2]=A[n+2][m]; B[m][n+3]=A[n+3][m]; //b
                    B[m+4][n-4]=t0; B[m+4][n+1-4]=t1; B[m+4][n+2-4]=t2; B[m+4][n+3-4]=t3;           //c
                }

                // 4.A的8x8矩阵的右下块,转置到B对应8x8矩阵的右下块
                for(m=j+4;m<j+8;m++){       // A的第m列
                    for(n=i+4;n<i+8;n++){
                        B[m][n]=A[n][m];
                    }
                }
            }
        }
    }
     
}

// 8x8 =>   hits:9066, misses:1179, evictions:1147(内循环中的循序很重要,完全不能交换!!!)
char trans_64x64_v5_desc[] = "trans_64x64_v5:blocking(8)+diagnal+spaceCompaction ";
void trans_64x64_v5(int M, int N, int A[N][M], int B[M][N])
{
    int i,j;
    int n,m;
    int t0,t1,t2,t3,t4,t5,t6,t7;                            // 把整行存入临时变量,解决对角线问题
    if(M==64){
        for(i=0;i<N;i+=8){            // A第i行           
            for(j=0;j<M;j+=8){        // A第j列
                /****************************************************************************
                 * 对于内部的的4x4小块,注意对于2,必须A按照列读,B按照行写!!!
                 * **************************************************************************/

                // 1.A的8x8矩阵的左上块,转置到B对应8x8矩阵的左上块
                //   同时,A的8x8矩阵的右上块,转置到B对应8x8矩阵的右上块(暂存)
                //   => 暂存只是为了充分利用B的缓存,减少冲突不命中
                for(n=i;n<i+4;n++){                                           // 对A的前四行
                    t0=A[n][j]; t1=A[n][j+1]; t2=A[n][j+2]; t3=A[n][j+3];
                    t4=A[n][j+4];t5=A[n][j+5];t6=A[n][j+6];t7=A[n][j+7];

                    B[j][n]=t0; B[j+1][n]=t1; B[j+2][n]=t2; B[j+3][n]=t3;
                    B[j][n+4]=t4; B[j+1][n+4]=t5; B[j+2][n+4]=t6; B[j+3][n+4]=t7;
                }

                // 2.A的8x8矩阵的左下块,转置到B对应8x8矩阵的右上块; 同时将B8x8矩阵的右上块暂存的数据放到最终位置
                // 此时与2中暂存在B的一行数据冲突:
                //   => a.先将A中一列放入临时变量t0~t3(A也要缓存到临时变量且A比B先缓存=>解决对角线问题!)
                //      b.将B中会发生冲突的一列放入临时变量t4~t7
                //      c.将临时变量t0~t3放入到B中刚空出的一行(即完成A中一列的转置) 
                //      d.将临时变量t4~t7的数据放入到正确位置(即之前暂存在B中的一行放入正确的位置)
                for(m=j;m<j+4;m++){           // A的第m列 <=> B的第m行; 为了先缓存B的一行到临时变量,所以第A而言需按列读!!!
                    t0=A[i+4][m]; t1=A[i+5][m]; t2=A[i+6][m]; t3=A[i+7][m];        
                    t4=B[m][i+4]; t5=B[m][i+5]; t6=B[m][i+6]; t7=B[m][i+7]; 
                    B[m][i+4]=t0; B[m][i+5]=t1; B[m][i+6]=t2; B[m][i+7]=t3;       // 下面两行不能对换,否则冲突不命中!
                    B[m+4][i]=t4; B[m+4][i+1]=t5; B[m+4][i+2]=t6; B[m+4][i+3]=t7;   
                }

                // 3.A的8x8矩阵的右下块,转置到B对应8x8矩阵的右下块
                for(n=i+4;n<i+8;n++){
                    t0=A[n][j+4]; t1=A[n][j+5]; t2=A[n][j+6]; t3=A[n][j+7];
                    B[j+4][n]=t0; B[j+5][n]=t1; B[j+6][n]=t2; B[j+7][n]=t3;
                }
            }
        }
    }
     
}


/************************ 3. 61x67 **********************************
 * 由于形状不规则,无法像1、2那样充分利用对角线问题; 
 * 这里暂时只是简单的使用分块,尝试不停分块尺寸!
 * 尺寸2,3,4,5,.....17,18,19,20,21.. => 最终17最佳
 * *****************************************************************/
char trans_61x67_v1_desc[] = "trans_61x67_v1:blocking(4)";
void trans_61x67_v1(int M, int N, int A[N][M], int B[M][N]){
    int i,j;
    int n,m;
    if(M==61){
        for(i=0;i<N;i+=4){
            for(j=0;j<M;j+=4){
                for(n=i;n<i+4 && n<N;n++){
                    for(m=j;m<j+4 && m<M;m++){
                        B[m][n]=A[n][m];
                    }
                }
            }
        }
    }
}

// =>  hits:6229, misses:1950, evictions:1918
char trans_61x67_v2_desc[] = "trans_61x67_v2:blocking(17)";
void trans_61x67_v2(int M, int N, int A[N][M], int B[M][N]){
    int i,j;
    int n,m;
    if(M==61){
        for(i=0;i<N;i+=17){
            for(j=0;j<M;j+=17){
                for(n=i;n<i+17 && n<N;n++){
                    for(m=j;m<j+17 && m<M;m++){
                        B[m][n]=A[n][m];
                    }
                }
            }
        }
    }
}




/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    //registerTransFunction(trans, trans_desc); 

    /************* 自定义  *************************/
    // 32x32
    //registerTransFunction(trans_32x32_v1,trans_32x32_v1_desc);
    //registerTransFunction(trans_32x32_v2,trans_32x32_v2_desc);

    //64x64
    //registerTransFunction(trans_64x64_v1,trans_64x64_v1_desc);
    //registerTransFunction(trans_64x64_v2,trans_64x64_v2_desc);
    //registerTransFunction(trans_64x64_v3,trans_64x64_v3_desc);
    //registerTransFunction(trans_64x64_v4,trans_64x64_v4_desc);
    //registerTransFunction(trans_64x64_v5,trans_64x64_v5_desc);

    //61x67
    //registerTransFunction(trans_61x67_v1,trans_61x67_v1_desc);
    //registerTransFunction(trans_61x67_v2,trans_61x67_v2_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

