/* 
 * tracegen.c - Running the binary tracegen with valgrind produces
 * a memory trace of all of the registered transpose functions. 
 * 
 * The beginning and end of each registered transpose function's trace
 * is indicated by reading from "marker" addresses. These two marker
 * addresses are recorded in file for later use.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include "cachelab.h"
#include <string.h>

/* External variables declared in cachelab.c */
extern trans_func_t func_list[MAX_TRANS_FUNCS];
extern int func_counter; 

/* External function from trans.c */
extern void registerFunctions();

/* Markers used to bound trace regions of interest */
/* volatile的变量是说这变量可能会被意想不到地改变,这样,编译器就不会去假设这个变量的值了 */
volatile char MARKER_START, MARKER_END;

static int A[256][256];
static int B[256][256];
static int M;
static int N;


int validate(int fn,int M, int N, int A[N][M], int B[M][N]) {
    int C[M][N];
    memset(C,0,sizeof(C));
    correctTrans(M,N,A,C);
    for(int i=0;i<M;i++) {
        for(int j=0;j<N;j++) {
            if(B[i][j]!=C[i][j]) {
                printf("Validation failed on function %d! Expected %d but got %d at B[%d][%d]\n",fn,C[i][j],B[i][j],i,j);
                return 0;
            }
        }
    }
    return 1;
}

int main(int argc, char* argv[]){
    int i;

    char c;
    int selectedFunc=-1;
    while( (c=getopt(argc,argv,"M:N:F:")) != -1){
        switch(c){
        case 'M':
            M = atoi(optarg);
            break;
        case 'N':
            N = atoi(optarg);
            break;
        case 'F':
            selectedFunc = atoi(optarg);
            break;
        case '?':
        default:
            printf("./tracegen failed to parse its options.\n");
            exit(1);
        }
    }
  

    /*  Register transpose functions */
    registerFunctions();

    /* Fill A with data */
    initMatrix(M,N, A, B); 

    /* Record marker addresses */
    /* 起始、终点地址写到文件中,用于告诉vargrind监测哪一部分的内存访问! 
    *  => 不是检测这一地址范围内的数据,只是作为提取数据时的标记(详见test-trans.c)
    * */
    FILE* marker_fp = fopen(".marker","w");
    assert(marker_fp);
    fprintf(marker_fp, "%llx %llx", 
            (unsigned long long int) &MARKER_START,
            (unsigned long long int) &MARKER_END );
    fclose(marker_fp);

    if (-1==selectedFunc) {
        /* Invoke registered transpose functions */
        for (i=0; i < func_counter; i++) {
            MARKER_START = 33;
            (*func_list[i].func_ptr)(M, N, A, B);
            MARKER_END = 34;
            if (!validate(i,M,N,A,B))
                return i+1;
        }
    } else {
        MARKER_START = 33;
        (*func_list[selectedFunc].func_ptr)(M, N, A, B);
        MARKER_END = 34;
        if (!validate(selectedFunc,M,N,A,B))
            return selectedFunc+1;

    }
    return 0;
}


