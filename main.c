#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <errno.h> 
#include <ctype.h>
#include <signal.h>
#include "graph.h"

struct Cache{
    char** paths;
};

struct Cache** cache;

void main(void){
    struct Graph* graph = createGraph(1000);
    int fd = open("input.txt", O_RDONLY);
    if(fd < 0 ){
        printf("open failed\n");
        return;
    }

    char buf[1];
    while((read(fd, buf, 1))){
        if(buf[0] == '#'){
            while(buf[0] != '\n')
                read(fd, buf, 1);
        }
        else{
            char tmp_buf[128];
            int index = 0;
            while(buf[0] != '\n'){
                tmp_buf[index++] = buf[0];
                read(fd, buf, 1);
            }
            int src = -1, dest = -1;
            tmp_buf[index] = '\0';
            sscanf(tmp_buf, "%d\t%d",&src, &dest);
            addEdge(graph, src, dest);
        }
    }
    cache = malloc(sizeof(struct Cache *) * graph->size);
    //if(cache[3] == NULL)printf("NULL\n");  
    char * str = bfs(3,151,graph);
    cache[3] = calloc(1,sizeof(struct Cache));
    cache[3]->paths = calloc(1, sizeof(char*));
    char* path = bfs(3,151,graph);
    if(cache[3] == NULL)printf("NULL2\n");
    cache[3]->paths[151] = malloc(strlen(str));
    cache[3]->paths[151] = str;
    if(cache[3]->paths[0] == NULL) printf("NULL3\n");
    printf("%s", cache[3]->paths[151]);
    


    
    
    free_graph(graph);
    
}