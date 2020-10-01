/* $begin dll */
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int x[2] = {1, 2};
int y[2] = {3, 4};
int z[2];

int main() 
{
    void *handle;
    void (*addvec)(int *, int *, int *, int); /*函数指针*/
    char *error; 

    /* Dynamically load the shared library that contains addvec() */
    /*RTLD_LAZY参数指示动态链接器推迟符号解析,直到执行来自库中的代码*/
    /*handle是指向整个共享库的句柄,想调用句柄中的函数/全局变量 => 使用dlsym函数*/
    handle = dlopen("./libvector.so", RTLD_NOW);
    if (!handle) {
	    fprintf(stderr, "%s\n", dlerror());
	    exit(1);
    }

    /* Get a pointer to the addvec() function we just loaded */
    /*获取共享库中的一个函数/全局变量*/
    addvec = dlsym(handle, "addvec");
    if ((error = dlerror()) != NULL) {
	    fprintf(stderr, "%s\n", error);
	    exit(1);
    }

    /* Now we can call addvec() just like any other function */
    addvec(x, y, z, 2);
    printf("z = [%d %d]\n", z[0], z[1]);

    /* Unload the shared library */
    if (dlclose(handle) < 0) {
	    fprintf(stderr, "%s\n", dlerror());
	    exit(1);
    }
    return 0;
}
/* $end dll */

