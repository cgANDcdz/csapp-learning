/* $begin sharing */
#include "csapp.h"
#define N 2
void *thread(void *vargp);

char **ptr;  /* Global variable */ 

int main() 
{
    int i;  
    pthread_t tid;
    char *msgs[N] = {
        "Hello from foo",  
        "Hello from bar"   
    };

    ptr = msgs;  /*msgs是主线程中的变量,通过全局的ptr指针,对等线程可以直接访问主线程变量中的内容msgs*/
    for (i = 0; i < N; i++)  
        Pthread_create(&tid, NULL, thread, (void *)i); 
    Pthread_exit(NULL); 
}

void *thread(void *vargp) 
{
    int myid = (int)vargp;
    static int cnt = 0;
    printf("[%d]: %s (cnt=%d)\n", myid, ptr[myid], ++cnt); 
    return NULL;
}
/* $end sharing */
