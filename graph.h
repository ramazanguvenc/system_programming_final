#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>

struct Node{
   int value,neighbor_counter, neighbor_size;
   struct Node** neighbors; //edges between this node to others.
};

struct Graph{
    struct Node** node_list;
    int size; //size of list
    int node_counter;
    int edge_counter;
};

struct List{
    int* arr;
    int length;
    int size;
    int front;
};

struct Node* createNode(int value);
struct Graph* createGraph(int size);
struct Node* add_node_to_graph(int value, struct Graph* graph);
void addEdge(struct Graph* graph, int x, int y);
void print_nodes_neighbors(struct Graph* graph, int value);
char* bfs(int src, int dest, struct Graph* graph);
struct List* createList(int size);
void push(struct List* list,int item);
int pop(struct List* list);
void set(struct List* list, int item, int index);
char* get_path(struct List * parent, int str_index);
void free_list(struct List* list);
void free_graph(struct Graph* graph);
void free_node(struct Node* node);
char* list_to_str(struct List* input);
