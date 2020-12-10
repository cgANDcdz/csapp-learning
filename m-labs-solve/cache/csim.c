#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<string.h>
#include "cachelab.h"

/***********************************************************************************
 *                                      partA
 * 1.关于LRU算法:这次采用时间戳+遍历的方式找到最近最久未使用
 *             => 更高效的方式是:hash+双向链表,参考leetcode 146
 * 2.根据文档要求,认为访存请求是对齐的,即一次访存总是在一个block内部,不存在要访问的一个数据
 *      横跨两个block的情况(从而可忽略b/B);也就是访问的只是块内的一部分数据,对于模拟器而言
 *      不需要考虑块内偏移,判断命中只需要set和tag!!!
 * 3.由于测试数据只有32位,这里也默认地址是32位(否则需更改部分代码,比如tag的数据类型)
 * ********************************************************************************/

#define TAG(addr,s,b) addr>>(s+b)
#define SET(addr,s,b) ({                     \
            unsigned int tag=TAG(addr,s,b);  \
            tag=tag<<s;                      \
            unsigned int tag_set=addr>>b;    \
            tag_set^tag;                     \
        })
#define TIME_RECENTLY 0
#define true 1
#define false 0
typedef int bool;



typedef struct{
    bool valid;
    int tag;
    //char * data;    /* 按文档要求,认为每次访问恰好对应一个block,所以可忽略数据部分 */
    int timestamp;    /* 用于LRU,每次被访问时设置为0; 其他行全部+1 */
}Line;

typedef struct{
    int nLine;
    Line *lines;
}Set;

typedef struct{
    int nSet;
    Set *sets;
}Cache;



/******************** 全局变量  ***********************/
int s=0;
int E=0;
int b=0;
bool v_flag=false;
char tracefile[20];

Cache* cache;

int hits=0;
int misses=0;
int evictions=0;





/****************** 辅助函数声明 *********************/
void parse_commline(int argc, char* argv[]);
void printHelp();

void init_cache();
void release_cache();

void load(int addr,int size);
void store(int aadr,int size);
void modify(int addr,int size);

bool check_hit(Set* set,int addr); // 判断set中是否命中addr处的数据;进行一次访存,所有时间戳+1,这个+1放在check_hit中统计!
bool check_full(Set* set,int addr); // 判断缓存是否已满,若不满,选择一个空行存放addr对应数据即可
void lru(Set* set,int addr);       // 根据lru策略,将set中的某行替换出去,将addr处的数据替换进来         






/********************* main ************************/
int main(int argc, char* argv[])
{
    parse_commline(argc,argv);
    init_cache();
   
    // 读取测试文件
    FILE *fp=fopen(tracefile,"r");
    if(fp==NULL){
        printf("error:open file failed!");
        return -1;
    }
    char type[1];
    int addr;
    int size;
    while(true){
        if(fscanf(fp,"%s %x,%d",type,&addr,&size)==EOF) break;

        if(v_flag)  printf("%s %x,%d",type,addr,size);
        
        // 不同访存类型
        if(strcmp(type,"L")==0){
            load(addr,size);
        }
        else if(strcmp(type,"S")==0){
            store(addr,size);
        }
        else if(strcmp(type,"M")==0){
            modify(addr,size);
        }
        else{
            // I
        }
        if(v_flag) printf("\n");
        
    }
    fclose(fp);

    release_cache();
    printSummary(hits, misses, evictions);
    return 0;
}




/*******************************************************************/
void parse_commline(int argc, char* argv[]){
    int c;
    extern char *optarg;
    while((c=getopt(argc,argv,"hvs:E:b:t:"))!=-1){
        switch (c)
        {
        case 'h':
            printHelp();
            exit(0);
        case 'v':
            v_flag=true;
            break;
        case 's':
            s=atoi(optarg);
            break;
        case 'E':
            E=atoi(optarg);
            break;
        case 'b':
            b=atoi(optarg);
            break;
        case 't':
            strcpy(tracefile,optarg);
            break;
        default:
            printHelp();
            exit(0);
        }
    }
}

void printHelp(){
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\n");
    printf("Examples:");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void init_cache(){
    cache=malloc(sizeof(Cache));                        // 初始化cache
    int S=1;
    for(int i=0;i<s;i++) S*=2;
    cache->nSet=S;
    cache->sets=malloc(sizeof(Set)*cache->nSet);
    for(int i=0;i<cache->nSet;i++){                    // 初始化cache中每个组
        Set *set=&(cache->sets[i]);
        set->nLine=E;
        set->lines=malloc(sizeof(Line)*set->nLine);
        for(int j=0;j<set->nLine;j++){                 // 初始化每个组中的所有行
            Line *line=&(set->lines[j]);
            line->valid=false;
            line->tag=0;
            line->timestamp=TIME_RECENTLY;
        }
    }
}

void load(int addr,int size){
    int setIdx=SET(addr,s,b);
    Set *set=&(cache->sets[setIdx]);
    if(check_hit(set,addr)){                    // 命中
        if(v_flag){
            printf(" hit");
        }
        hits++;
    }
    else{                                      // 不命中
        if(!check_full(set,addr)){   //不命中,但是缓存未满(在里边选择一个空行存放数据)    
            if(v_flag){
                printf(" miss");
            }
            misses++;
        }
        else{                       //不命中,缓存已满=> 采用lru替换策略
            lru(set,addr);
            if(v_flag){
                printf(" miss eviction");
            }
            misses++;
            evictions++;
        }
    }
}

// 与load其实是一样
void store(int addr,int size){
    load(addr,size);
}

// 先load,再store
void modify(int addr,int size){
    load(addr,size);
    store(addr,size);
}

//判断是否命中;每次访存,时间戳+1放在check_hit中实现
bool check_hit(Set* set,int addr){  
    int tag=TAG(addr,s,b);
    bool hit=false;
    for(int i=0;i<set->nLine;i++){
        Line* line=&(set->lines[i]);
        line->timestamp++;
        if(line->valid && line->tag==tag){                        //命中
                line->timestamp=0;
                hit=true;
        }    
    }
    return hit;
}

// 当不命中时,判断缓存是否满,不满则将addr处的数据放到一个空行中
bool check_full(Set* set,int addr){         
    bool full=true;
    int tag=TAG(addr,s,b);
    for(int i=0;i<set->nLine;i++){
        Line* line=&(set->lines[i]);
        if(!line->valid){                     // 选择该空行存放addr对应数据
            full=false;
            line->valid=true;
            line->timestamp=TIME_RECENTLY;
            line->tag=tag;
            break;
        }
    }
    return full;
}


// 缓存全部满;采用lru,将最久的块替换出去;将addr处的数据替换进来
void lru(Set* set,int addr){            
    int tag=TAG(addr,s,b);
    int maxTime=-1;
    int maxIdx=-1;
    for(int i=0;i<set->nLine;i++){
        Line* line=&(set->lines[i]);
        if(line->timestamp > maxTime){
            maxTime=line->timestamp;
            maxIdx=i;
        }
    }
    Line* line=&(set->lines[maxIdx]);
    line->timestamp=0;
    line->valid=true;
    line->tag=tag;
}


void release_cache(){
    for(int i=0;i<cache->nSet;i++){
        Set* set=&(cache->sets[i]);
        free(set->lines);
    }
    free(cache->sets);
    free(cache);
}

