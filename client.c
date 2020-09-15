#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h> 

 
#define SA struct sockaddr 


void send_and_recieve(int sockfd, int source, int dest) 
{ 
    
    char buf[80]; 
    int len; 
    char str_len[10];
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    sprintf(buf, "%d %d", source, dest);
    write(sockfd, buf, sizeof(buf));
    read(sockfd, str_len, 10);
    sscanf(str_len,"%d",&len);
    write(sockfd, "1", 1);
    char path[len + 1];
    bzero(path, sizeof(path)); 
    read(sockfd, path, len);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    printf("%lu path : %sArrived in %f seconds\n", time(NULL),path, (end.tv_nsec - begin.tv_nsec) / 1000000000.0 +
    (end.tv_sec  - begin.tv_sec));
    
     
    
    
    /* stuff to do! */
} 
  
int main(int argc, char **argv) 
{ 
    int sockfd, connfd, port,c, s, d; 
    struct sockaddr_in servaddr, cli; 
    char ip[50];
    while((c = getopt(argc, argv, "a:p:s:d:")) != -1){
        switch(c){
            case 'a':
                strcpy(ip, optarg);
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'd':
                d = atoi(optarg);
                break;
            case '?':
                if(optopt == 'a' || optopt == 'p' || optopt == 's' || optopt == 'd')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if(isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c.`\n", optopt);
                else
                    fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
                return 1;
                break;
            default:
                abort();
                return 1;
        }
    }
  
    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(ip); 
    servaddr.sin_port = htons(port); 
    printf("%lu Client (%u) connecting to %s\n", time(NULL), getpid(), ip);
    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("%lu Client (%u) connected and requesting a path from node %d to %d\n",time(NULL),getpid(),s,d); 
  
    
    send_and_recieve(sockfd, s, d); 
  
    // close the socket 
    close(sockfd); 
}