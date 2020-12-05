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

/*************************************************************************************
 *                              基于cache的矩阵转置优化
 * 0.cache模拟器默认使用直接映射:s=5,E=1,b=5 => 32组,每组一行,每行32字节
 * 1.registerTransFunction的写法很有借鉴意义
 * 2.必读test-trans.c源码、tracegen.c源码!!!
 * 3.这里只是模拟,对命中、缺失的统计并非实际运行时的情况; 这里的结果只是使用运行时地址与强行
 *      给定的s E b计算所得; 
 * 4.需明白判断命中/缺失与地址中的块内偏移字段无关
 * 5.partB中b的位数很关键,它决定了块内能存储的字节数,从而决定了分块大小
 * 
 * ...
 * n.为什么矩阵分块有效?
 *  => 当数组大小增加时,时间局部性会明显降低,高速缓存中不命中数目增加;
 *     使用分块时,时间局部性由分块大小来决定,而不是数组总大小来决定(子块内的数据短期内可再次被
 *     访问,这时候直接缓存命中;如果不分块,数组又非常大,刚访问过的数据可能很长时间后才再次访问,
 *     而这时已经缺页)
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
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
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

/**************************** 自定义 ***************************************/
char trans_32x32_v1_desc[] = "trans_32x32_v1";
void trans_32x32_v1(int M, int N, int A[N][M], int B[M][N])
{
    int i,j,m,n;
    if(M==32){
        for(i=0;i<N;i+=8){              // i,j用于确定块
            for(j=0;j<M;j+=8){
                for(n=i;n<i+8;n++){     // n,m用于确定块内数据
                    for(m=j;m<j+8;m++){
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
    registerTransFunction(trans, trans_desc); 

    // 自定义
    registerTransFunction(trans_32x32_v1,trans_32x32_v1_desc);
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

