/* 
 * echoservert_pre.c - A prethreaded concurrent echo server
 */
/* $begin echoservertpremain */
#include "csapp.h"
#include "sbuf.h"
#define NTHREADS  4
#define SBUFSIZE  16

void echo_cnt(int connfd);
void *thread(void *vargp);

sbuf_t sbuf; /* Shared buffer of connected descriptors */

int main(int argc, char **argv) 
{
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid; 

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);      /*初始化共享的连接描述符缓冲区*/
    /*预线程化 => 线程个数是固定的,且服务完一个请求后并不会释放线程,而是留着继续服务下一个请求*/
    for (i = 0; i < NTHREADS; i++)  /* Create worker threads */ 
	    Pthread_create(&tid, NULL, thread, NULL);              

    while (1) { 
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd); /* Insert connfd in buffer => 已连接描述符加入缓冲区 */ 
    }
}

void *thread(void *vargp) 
{  
    Pthread_detach(pthread_self()); 
    /*线程创建后不会撤销,能不停地提供服务*/
    while (1) { 
        int connfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */
        echo_cnt(connfd);                /* Service client */
        Close(connfd);
    }
}
/* $end echoservertpremain */
