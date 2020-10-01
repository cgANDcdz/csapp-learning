#include <stdio.h>
#include"csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/******************************* 注意 **********************************


***********************************************************************/

void solve(){
    //
}

/*************  读取浏览器请求 *************/
void read_req(int connfd){
    rio_t rio;
    char line[MAXLINE];
    Rio_readinitb(&rio,connfd); 
    while(Rio_readlineb(&rio,line,MAXLINE)){
        printf("%s",line);
    }
    
}

void write_res(int connfd){

}


/* You won't lose style points for including this long line in your code */
//static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


int main(int argc, char **argv)
{

    int listenfd,connfd;
    SA clientaddr;                            //客户端的通用socket地址
    socklen_t clientlen;
    char clientname[MAXLINE], clientport[MAXLINE];

    if(argc!=2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    listenfd=Open_listenfd(argv[1]);

    while(1){
        clientlen=sizeof(clientaddr);                                              //clientlen必须初始化
        connfd=Accept(listenfd, &clientaddr, &clientlen);                          //accept系统调用自动挂起等待请求到达
        Getnameinfo(&clientaddr, clientlen, clientname, MAXLINE, clientport, MAXLINE, 0);  //获取客户端主机名与端口
        printf("Accepted connection from (%s, %s)\n", clientname, clientport);
        
        read_req(connfd);

        Close(connfd);   
    }

    return 0;
}
