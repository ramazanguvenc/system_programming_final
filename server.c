#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <errno.h> 
#include <ctype.h>
#include <semaphore.h>
#include <math.h>
#include <sys/file.h>
#include "graph.h"


#define SA struct sockaddr 
//TODO
//FIX GRAPH MEMORY LEAK.
//FIX if argv is not given.
//ADD LOG
//SEND PATH LENGTH TO CLIENT FIRST
//ADD CACHE

struct thread_param{
    pthread_mutex_t wait_connection, is_avaiable_mutex;
    int index, is_avaiable, connfd;
};

struct Cache{
    char** paths;
};

pthread_t *thread_ids;
struct thread_param *params;
struct Graph* graph;
int thread_counter=0, max_thread = 0, current_working_thread_counter =0,old_thread_counter=0,log_fd, finish_flag =0, sockfd, writer_counter=0,reader_counter=0,priority_flag = 0,r=2,deamon_fd;
sem_t sem_avaiable_thread_counter, sem_resizer;
pthread_mutex_t thread_counter_mutex, current_working_thread_counter_mutex, cache_mutex, log_mutex, finish_mutex, writer_mutex, reader_mutex, priority_mutex;
float old_system_load = 0.0; //I need this because when resizer tries to print this it already changes. So I'm saving the old version why he's going to increase it first place.
struct Cache** cache;
char* node_error;


void func(int sockfd);
void load_graph(char* filename);
void* worker_thread(void *vargp); 
void* resizer_thread(void *vargp);
int get_or_set_thread_counter(int flag);
int get_or_set_current_thread_counter(int flag); // current_working_thread_counter += flag;
void increase_worker_threads();
void write_to_log(char * msg); //writing executing parametres etc.
float get_system_load();
void signal_handler(int sig);
int get_set_finish_flag(int inc);
int get_set_reader_counter(int inc);
int get_set_writer_counter(int inc);
void free_cache();
static void deamon();
 
int main(int argc, char **argv){ 

    deamon();
    int  connfd, len, c, i_flag=0,o_flag=0, port=0; 
    struct sockaddr_in servaddr, cli; 
    char filename[PATH_MAX], log_filename[PATH_MAX];
    signal(SIGINT, signal_handler);

    while((c = getopt(argc, argv, "i:p:s:x:o:r:")) != -1){
        switch (c)
        {
        case 'i':
            i_flag = 1;
            strcpy(filename, optarg);
            break;
        
        case 'p':
            port = atoi(optarg);
            break;
        case 's':
            thread_counter = atoi(optarg);
            break;
        case 'x':
            max_thread = atoi(optarg);
            break;
        case 'o':
            o_flag = 1;
            strcpy(log_filename, optarg);
            break;
        case 'r':
            r = atoi(optarg);
            break;
        
        case '?':
            if(optopt == 'i')
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
    if(i_flag ==0 || o_flag ==0 || max_thread < 2 || thread_counter < 1 || port < 1){
        fprintf(stderr, "Argumants given wrong. Please check again!\n");
        exit(1);
    }
    log_fd = creat(log_filename, 0666);
    if(log_fd < 0){
        printf("Error while creating file: %s\n", log_filename);
        exit(1);
    }

    node_error = malloc(128);
    strcpy(node_error, "Given node numbers are greater than your input. Please be careful!\n");
    char log_buf[128];
    write(log_fd, "Executing with parametres:\n", strlen("Executing with parametres:\n"));
    write(log_fd, "-i\t", 3);
    write(log_fd, filename, strlen(filename));
    sprintf(log_buf,"\n-p\t%d\n",port);
    write(log_fd, log_buf, strlen(log_buf));
    write(log_fd, "-o\t", 3);
    write(log_fd, log_filename, strlen(log_filename));
    sprintf(log_buf,"\n-s\t%d\n-x\t%d\nLoading graph...\n", thread_counter, max_thread);
    write(log_fd, log_buf, strlen(log_buf));
    clock_t begin = clock();
    load_graph(filename);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    sprintf(log_buf,"%lu Graph loaded in %.2lf seconds with %d nodes and %d edges\n", time(NULL),time_spent, graph->node_counter+1, graph->edge_counter);
    write(log_fd, log_buf, strlen(log_buf));
    cache = malloc(sizeof(struct Cache*) * graph->size);

    if(sem_init(&sem_avaiable_thread_counter, 0, 0) == -1 ||
       sem_init(&sem_resizer, 0, 0) == -1 ||
       pthread_mutex_init(&thread_counter_mutex, NULL) != 0 ||
       pthread_mutex_init(&current_working_thread_counter_mutex, NULL) != 0 ||
       pthread_mutex_init(&cache_mutex, NULL) != 0 ||
       pthread_mutex_init(&log_mutex, NULL) != 0 ||
       pthread_mutex_init(&finish_mutex, NULL) != 0 ||
       pthread_mutex_init(&priority_mutex, NULL) != 0 ||
       pthread_mutex_init(&reader_mutex, NULL) != 0 ||
       pthread_mutex_init(&writer_mutex, NULL) != 0){
        sprintf(log_buf, "sem_init error\n");
        write(log_fd, log_buf, strlen(log_buf));
        return 1;       
    }
    
    //creating threads
    thread_ids = malloc(sizeof(pthread_t) * max_thread);
    params = malloc(sizeof(struct thread_param) * max_thread);
    pthread_t worker_id;
    pthread_create(&worker_id, NULL, resizer_thread, NULL);

    for(int i =0; i < thread_counter; i++){
        if (pthread_mutex_init(&params[i].wait_connection, NULL) != 0 ||
            pthread_mutex_init(&params[i].is_avaiable_mutex, NULL) != 0){ 
            sprintf(log_buf,"\n%lu mutex init has failed\n", time(NULL));
            write(log_fd, log_buf, strlen(log_buf)); 
            exit(1); 
        }
        params[i].index = i;
        params[i].is_avaiable = 1;
        params[i].connfd = -666;
        pthread_mutex_lock(&(params[i].wait_connection));
        pthread_create(&(thread_ids[i]), NULL, worker_thread, (void*) &params[i]);
    }
    sprintf(log_buf,"%lu A pool of %d threads has been created\n", time(NULL),thread_counter);
    write(log_fd, log_buf, strlen(log_buf));

    //rest of max threads also has to initialize here because of some mutex rule you have to
    //unlock your mutex in which thread you lock them or something.

    for(int i=thread_counter;i<max_thread;i++){
        if (pthread_mutex_init(&params[i].wait_connection, NULL) != 0 ||
            pthread_mutex_init(&params[i].is_avaiable_mutex, NULL) != 0){ 
            sprintf(log_buf,"\n%lu mutex init has failed\n", time(NULL)); 
            write(log_fd, log_buf, strlen(log_buf)); 
            exit(1); 
        }
        params[i].index = i;
        params[i].is_avaiable = 1;
        params[i].connfd = -666;
        pthread_mutex_lock(&(params[i].wait_connection));
        //no create.
    }
    
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        sprintf(log_buf,"%lu socket creation failed\n", time(NULL));
        write(log_fd, log_buf, strlen(log_buf)); 
        exit(1); 
    } 
    else{
        sprintf(log_buf,"%lu Socket successfully created\n", time(NULL));
        write(log_fd, log_buf, strlen(log_buf));  
    }
        
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        sprintf(log_buf,"%lu socket bind failed\n", time(NULL));
        write(log_fd, log_buf, strlen(log_buf)); 
        exit(1); 
    } 
    else{
        sprintf(log_buf,"%lu Socket successfully binded\n", time(NULL));
        write(log_fd, log_buf, strlen(log_buf)); 
    }
        
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        sprintf(log_buf,"%lu Listen failed\n", time(NULL)); 
        write(log_fd, log_buf, strlen(log_buf)); 
        exit(1); 
    } 
    else{
        sprintf(log_buf,"%lu Server listening\n", time(NULL)); 
        write(log_fd, log_buf, strlen(log_buf)); 
    }
        
    len = sizeof(cli); 
    // Accept the data packet from client and verification 
    while(get_set_finish_flag(0) == 0) {
        connfd = accept(sockfd, (SA*)&cli, &len); 
        if (connfd < 0) { 
            sprintf(log_buf,"%lu server acccept failed\n", time(NULL));
            write(log_fd, log_buf, strlen(log_buf));  
        }
        else{
            if(get_system_load() == 100.00){
                sprintf(log_buf, "%lu No thread is avaiable! Waiting for one\n", time(NULL));
                write_to_log(log_buf);
            }
            sem_wait(&sem_avaiable_thread_counter);
            pthread_mutex_lock(&thread_counter_mutex);
            int tmp_thread_counter = thread_counter;
            pthread_mutex_unlock(&thread_counter_mutex);
            for(int i =0; i < tmp_thread_counter; i++){
                pthread_mutex_lock(&(params[i].is_avaiable_mutex));
                if(params[i].is_avaiable == 1){
                    params[i].is_avaiable = 0;
                    pthread_mutex_unlock(&(params[i].is_avaiable_mutex));  
                    params[i].connfd = connfd;
                    sprintf(log_buf, "%lu A connection has been delegated to thread id #%d, system load %.1f%%\n",time(NULL),i, get_system_load());
                    write(log_fd, log_buf, strlen(log_buf));
                    pthread_mutex_unlock(&(params[i].wait_connection));
                    break;
                    
                }
                else{
                    pthread_mutex_unlock(&(params[i].is_avaiable_mutex));
                }
            }
            
        }
        
    }
   
    sprintf(log_buf, "%lu Termination signal recieved, waiting for ongoing threads to complete\n", time(NULL));
    write_to_log(log_buf);
    for(int i =0;i < get_or_set_thread_counter(0); i++)pthread_mutex_unlock(&(params[i].wait_connection));
    for(int i =0;i < get_or_set_thread_counter(0); i++){
        pthread_join(thread_ids[i], NULL);
        pthread_mutex_destroy(&(params[i].is_avaiable_mutex));
        pthread_mutex_destroy(&(params[i].wait_connection));
    }
    for(int i = get_or_set_thread_counter(0);i < max_thread;i++){
        pthread_mutex_destroy(&(params[i].is_avaiable_mutex));
        pthread_mutex_destroy(&(params[i].wait_connection));       
    }
    
    sem_post(&sem_resizer);
    pthread_join(worker_id, NULL);
    sprintf(log_buf, "%lu All threads have terminated, server shutting down\n", time(NULL));
    free_cache(cache);
    free_graph(graph);
    free(thread_ids);
    free(params);
    free(node_error);
    close(sockfd); 
    close(log_fd);
    pthread_mutex_destroy(&thread_counter_mutex);
    pthread_mutex_destroy(&current_working_thread_counter_mutex);
    pthread_mutex_destroy(&cache_mutex);
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&finish_mutex);
    //int fd = open("whatsup",O_CREAT | O_EXCL);
    int result = flock(deamon_fd, LOCK_UN);
    close(deamon_fd);
    remove("__161044037__");
}

void* worker_thread(void *vargp){
    struct thread_param* this;
    this = (struct thread_param*) vargp;
    int index = this->index;
    char msg[128];
    int s=-1, d=-1, database_flag = 0;
    while(get_set_finish_flag(0) == 0){
            sem_post(&sem_avaiable_thread_counter);
            sprintf(msg, "%lu Thread #%d, waiting for connection\n", time(NULL), index);
            write_to_log(msg);
            pthread_mutex_lock(&(this->wait_connection));
            if(get_set_finish_flag(0) == 1){
                if(this->connfd != -666)close(this->connfd);
                pthread_mutex_unlock(&(this->wait_connection));
                sprintf(msg, "%lu Thread #%d is exiting\n", time(NULL), index);
                write_to_log(msg);
                return NULL;
            }
            get_or_set_current_thread_counter(1);
            sem_post(&sem_resizer);
            read(this->connfd, msg, 128);
            sscanf(msg, "%d %d", &s, &d);
            char* answer;
            sprintf(msg, "%lu Searching database for a path from node %d to node %d\n", time(NULL),s, d);
            write_to_log(msg);
            pthread_mutex_lock(&cache_mutex);
            if(s > graph->node_counter || d > graph->node_counter){
                answer = node_error;
            }
            else if(cache[s] == NULL){
                if(priority_flag == 0){
                    //priority is for readers wait until they're done.
                    pthread_mutex_unlock(&cache_mutex);
                    while(get_set_reader_counter(0) > 0){
                        pthread_mutex_lock(&reader_mutex);
                    }
                    pthread_mutex_lock(&cache_mutex);
                }
                get_set_writer_counter(1);
                sprintf(msg,"%lu No path in database calculating %d->%d\n",time(NULL),s, d);
                write_to_log(msg);
                answer = bfs(s, d, graph);
                cache[s] = malloc(sizeof(struct Cache));
                cache[s]->paths = calloc(graph->size,sizeof(char*));
                free(cache[s]->paths[d]);
                cache[s]->paths[d] = answer;
                get_set_writer_counter(-1);
                pthread_mutex_unlock(&writer_mutex);
                //sprintf(msg, "saved to cache[%d][%d]\n", s, d);
                //write(1,msg,strlen(msg));
            }
            else if(cache[s]->paths[d] == NULL){
                if(priority_flag == 0){
                    //priority is for readers wait until they're done.
                    pthread_mutex_unlock(&cache_mutex);
                    while(get_set_reader_counter(0) > 0){
                        pthread_mutex_lock(&reader_mutex);
                    }
                    pthread_mutex_lock(&cache_mutex);
                }
                get_set_writer_counter(1);
                sprintf(msg,"%lu No path in database calculating %d->%d\n",time(NULL),s, d);
                write_to_log(msg);
                answer = bfs(s, d, graph);
                cache[s]->paths[d] = answer;
                get_set_writer_counter(-1);
                pthread_mutex_unlock(&writer_mutex);

            }
            else{
                if(priority_flag == 1){
                    //priority is for writers wait until they're done.
                    pthread_mutex_unlock(&cache_mutex);
                    while(get_set_writer_counter(0) > 0){
                        pthread_mutex_lock(&writer_mutex);
                    }
                    pthread_mutex_lock(&cache_mutex);
                }
                get_set_reader_counter(1);
                sprintf(msg, "%lu path found in database: ",time(NULL));
                database_flag = 1;
                pthread_mutex_lock(&log_mutex);
                write(log_fd, msg, strlen(msg));
                answer = cache[s]->paths[d];
                write(log_fd, answer, strlen(answer));
                pthread_mutex_unlock(&log_mutex);
                get_set_reader_counter(-1);
                pthread_mutex_unlock(&reader_mutex);
                //sprintf(msg, "used cache[%d][%d]\n", s, d);
                //write(1,msg,strlen(msg));
            }
            pthread_mutex_unlock(&cache_mutex);
            char str_len[10];
            sprintf(str_len,"%ld",strlen(answer));
            write(this->connfd, str_len, 10);
            read(this->connfd, strlen, 1); //this probably doesn't do anything but i did it just in case, it doesn't really slow down the program that much.
            write(this->connfd,answer,strlen(answer));
            if(strcmp(answer, "Couldn't find the path\n") == 0){
                sprintf(msg, "%lu path not possible from node %d to %d\n", time(NULL),s, d);
                write_to_log(msg);
            }
            else if(database_flag == 0){
                char msg_tmp[128 + strlen(answer)];
                sprintf(msg_tmp, "%lu path calculated: %s", time(NULL), answer);
                write_to_log(msg_tmp);
            }
            else{
                database_flag = 0;
            }
            pthread_mutex_lock(&(this->is_avaiable_mutex));
            this->is_avaiable = 1;
            pthread_mutex_unlock(&(this->is_avaiable_mutex));
            close(this->connfd);
            get_or_set_current_thread_counter(-1);
    }

    if(this->connfd != 666)close(this->connfd);
    pthread_mutex_unlock(&(this->wait_connection));
    sprintf(msg, "%lu Thread #%d is exiting\n", time(NULL), index);
    write_to_log(msg);
    
    return NULL;
    
}

void* resizer_thread(void *vargp){
    while(1){
        sem_wait(&sem_resizer);
        if(get_set_finish_flag(0) == 1)return NULL;
        int tmp_current_thread_counter = get_or_set_current_thread_counter(0);
        int tmp_thread_counter = get_or_set_thread_counter(0);
        float quarter = ceil((float)tmp_thread_counter/4.0);
        old_system_load = get_system_load();
        if(old_system_load >= 75){
            old_thread_counter = get_or_set_thread_counter(0);
            get_or_set_thread_counter(quarter);
            increase_worker_threads();
        }
        if(get_or_set_thread_counter(0) == max_thread) return NULL;
    }
}

int get_or_set_thread_counter(int flag){
    int result;
    pthread_mutex_lock(&thread_counter_mutex);
    thread_counter = thread_counter + flag;
    if(thread_counter > max_thread) thread_counter = max_thread;
    result = thread_counter;
    pthread_mutex_unlock(&thread_counter_mutex);
    return result;
}

int get_or_set_current_thread_counter(int flag){
    int result;
    pthread_mutex_lock(&current_working_thread_counter_mutex);
    current_working_thread_counter += flag;
    result = current_working_thread_counter;
    pthread_mutex_unlock(&current_working_thread_counter_mutex);
    return result;
}

int get_set_finish_flag(int inc){
    int result;
    pthread_mutex_lock(&finish_mutex);
    finish_flag = finish_flag + inc;
    result = finish_flag;
    pthread_mutex_unlock(&finish_mutex);
    return result;
}


void load_graph(char* filename){
    graph = createGraph(1000);
    int fd = open(filename, O_RDONLY);
    if(fd < 0 ){
        printf("open failed\n");
        exit(0);
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
    close(fd);
}

void increase_worker_threads(){
    int tmp_thread_counter = get_or_set_thread_counter(0);
    for(int i =old_thread_counter; i < tmp_thread_counter; i++){
        pthread_create(&(thread_ids[i]), NULL, worker_thread, (void*) &params[i]);
    }
    char tmp[129];
    sprintf(tmp, "%lu System load %.1f%%, pool extended to %d\n", time(NULL),old_system_load, thread_counter);
    write_to_log(tmp);
}

float get_system_load(){
    return ( (float) get_or_set_current_thread_counter(0) / (float) get_or_set_thread_counter(0) ) * 100.00;
}

void write_to_log(char* msg){
    pthread_mutex_lock(&log_mutex);
    write(log_fd, msg, strlen(msg));
    pthread_mutex_unlock(&log_mutex);
}

int get_set_reader_counter(int inc){
    int result;
    pthread_mutex_lock(&reader_mutex);
    reader_counter += inc;
    result = reader_counter;
    pthread_mutex_unlock(&reader_mutex);
    return result;
}

int get_set_writer_counter(int inc){
    int result;
    pthread_mutex_lock(&writer_mutex);
    writer_counter += inc;
    result = writer_counter;
    pthread_mutex_unlock(&writer_mutex);
    return result;
}

static void deamon(){
    pid_t pid;
    deamon_fd =  open("__161044037__",O_CREAT | O_EXCL);
    int result = flock(deamon_fd, LOCK_EX);
    if(deamon_fd < 0 || result < 0){
        printf("You tried to run it again!\n");
        exit(EXIT_FAILURE);
    }

    /* Fork off the parent process */

    pid = fork();
    
    /* An error occurred */
    if (pid < 0){
        printf("error occured while creating deamon\n");
    }
    
     /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);
    
    /* Catch, ignore and handle signals */
    /*TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    /* Fork off for the second time*/
    pid = fork();
    
    /* An error occurred */
    if (pid < 0){
        printf("error occured while creating deamon\n");
        exit(EXIT_FAILURE);
    }
    
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* Set new file permissions */
    umask(0);
    
    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }
}





void signal_handler(int sig){
    if(sig == SIGINT){
        get_set_finish_flag(1);
        close(sockfd);
        sem_post(&sem_avaiable_thread_counter);
    }
}

void free_cache(){
    if(cache == NULL)return;
    for(int i =0;i < graph->size;i++){
        if(cache[i] != NULL){
            if(cache[i]->paths != NULL){
                for(int j = 0; j < graph->size;j++){
                    if(cache[i]->paths[j] != NULL)
                        free(cache[i]->paths[j]);
                }
                free(cache[i]->paths);
            }
            free(cache[i]);
        }
    }
    free(cache);
}