/* 
 * echoserverp.c - A concurrent echo server based on processes
 */
/* $begin echoserverpmain */
#include "csapp.h"
void echo(int connfd);

void sigchld_handler(int sig) //line:conc:echoserverp:handlerstart
{
	/*SIGCHLD信号是阻塞的，且Linux信号不排队,
	这里使用循环是为了回收多个僵死的子进程 */
    while (waitpid(-1, 0, WNOHANG) > 0)
	;
    return;
} //line:conc:echoserverp:handlerend

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    Signal(SIGCHLD, sigchld_handler);    /*提供信号处理程序:回收僵死的子进程*/
    listenfd = Open_listenfd(argv[1]);
    while (1) {
	clientlen = sizeof(struct sockaddr_storage); 
	connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
	if (Fork() == 0) { 
	    Close(listenfd); /* 子进程关闭其监听描述符*/
	    echo(connfd);    /* Child services client */
	    Close(connfd);   /* Child closes connection with client */ 
	    exit(0);         /* Child exits */
	}
	Close(connfd); /*父进程需要关闭已连接描述符*/ 
    }
}
/* $end echoserverpmain */
