/*
 * echoclient.c - An echo client
 */
/* $begin echoclientmain */
#include "csapp.h"

int main(int argc, char **argv) 
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
	fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
	exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);       /*和服务端建立连接*/
    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {  /*从客户端标准输入读取文本行到buf*/
        Rio_writen(clientfd, buf, strlen(buf));   /*将buf中的信息发送给服务器*/
        Rio_readlineb(&rio, buf, MAXLINE);        /*读取服务器返回的信息到buf*/
        Fputs(buf, stdout);                       /*将buf中服务器返回的信息显示到客户端标准输出*/
    }
    Close(clientfd); //line:netp:echoclient:close
    exit(0);
}
/* $end echoclientmain */
