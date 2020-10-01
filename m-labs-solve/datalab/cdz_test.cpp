#include<stdio.h>


int allOddBits(int x) {

    return 2;
}

int main()
{
    int a=170;
    int b=170<<8;
    int c=b<<8;
    int d=c<<8;
    int ret=((a|b)|c)|d;
    int std=0xAAAAAAAA;
    printf("std answer:%d \n",std);

    printf("puzzle answer:%d \n",ret);
    printf("done!\n");

    return 0;
}