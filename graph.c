#include "graph.h"


struct Node* createNode(int value){
    struct Node* new_node = malloc(sizeof(struct Node));
    new_node->value = value;
    new_node->neighbor_counter = 0;
    new_node->neighbor_size = 5;
    new_node->neighbors = malloc(sizeof(struct Node*) * new_node->neighbor_size);
    return new_node;
}



struct Node* add_node_to_graph(int value, struct Graph* graph){
   
    if(value + 5 > graph->size){ //resize
        int old_graph_size = graph->size;
        graph->size = value + 150;
        graph->node_list = realloc(graph->node_list, sizeof(struct Node*) * graph->size);
        for(int i =old_graph_size; i < graph->size;i++)
            graph->node_list[i] = createNode(i);
    }

    return graph->node_list[value];
}
    

struct Graph* createGraph(int size){
    struct Graph* newGraph = malloc(sizeof(struct Graph));
    newGraph->size = size;
    newGraph->node_counter = 0;
    newGraph->edge_counter = 0;
    newGraph->node_list = malloc(sizeof(struct Node*) * size);
    for(int i =0; i < size; i++){
        newGraph->node_list[i] = createNode(i);
    }
    return newGraph;
}

//x -> y
void addEdge(struct Graph* graph, int x, int y){
    if(x < 0) return;
    if(y < 0) return;
    if(x > graph->node_counter) graph->node_counter = x;
    if(y > graph->node_counter) graph->node_counter = y;
    struct Node* nodeX = add_node_to_graph(x, graph);
    struct Node* nodeY = add_node_to_graph(y, graph);

    if(nodeX->neighbor_counter + 1 == nodeX->neighbor_size){
        nodeX->neighbor_size = nodeX->neighbor_size * 2;
        nodeX->neighbors = realloc(nodeX->neighbors, sizeof(struct Node*) * nodeX->neighbor_size);
    }

    nodeX->neighbors[nodeX->neighbor_counter] = nodeY;
    nodeX->neighbor_counter++;
    graph->edge_counter++;
    
}



void print_nodes_neighbors(struct Graph* graph, int value){
    struct Node* node = graph->node_list[value];
    
    if(node == NULL)
        return;
    
    for(int i =0; i < node->neighbor_counter;i++)
        printf("%d->%d\t", node->value, node->neighbors[i]->value);
    printf("\n");
}



char* bfs(int src, int dest, struct Graph* graph){
    //printf("graph size = %d\n", graph->size);
    struct List* queue = createList(graph->size);
    struct List* parent = createList(graph->size);
    struct List* visited = createList(graph->size);
    push(queue, src);
    set(visited, 1, src);
    set(parent, -1, src);
    while(queue->length > 0){
        int v = pop(queue);
        if(v == dest){
            char* result = get_path(parent, v);
            free_list(queue);
            free_list(parent);
            free_list(visited);
            return result;
        }
        for(int i =0; i < graph->node_list[v]->neighbor_counter;i++){
            int neighbor = graph->node_list[v]->neighbors[i]->value;
            
            if(visited->arr[neighbor] == 0){
                //printf("neighbor = %d\n", neighbor);
                set(visited, 1, neighbor);
                set(parent, v, neighbor);
                push(queue, neighbor);
            }
        }

    }
    //printf("Couldn't find a path\n");
    free_list(queue);
    free_list(parent);
    free_list(visited);
    char* result = malloc(strlen("Couldn't find the path\n") + 5);
    strcpy(result, "Couldn't find the path\n");
    return result;
}

char* get_path(struct List * parent, int str_index){
    struct List* path = createList(parent->size);
    struct List* tmp_path = createList(parent->size);
    while(str_index != -1){
        push(path, str_index);
        push(tmp_path, str_index);
        str_index = parent->arr[str_index];
    }
    for(int i =0; i < path->length; i++){
        path->arr[path->length - i - 1] = tmp_path->arr[i];
    }
   

    free_list(tmp_path);
    char* result = list_to_str(path);
    free_list(path);
    return result;
}


struct List* createList(int size){
    struct List* new_list = malloc(sizeof(struct List));
    new_list->size = size;
    new_list->length = 0;
    new_list->arr = calloc(size, sizeof(int));
    new_list->front = 0;
    return new_list;
}

void push(struct List* list, int item){
    if(list->size == list->length + 1){
        list->size = list->size * 2;
        list->arr = realloc(list->arr, sizeof(int) * list->size);
    }
    list->arr[list->length + list->front] = item;
    list->length++;
}

int pop(struct List* list){
    int result = list->arr[list->front];
    list->front++;
    list->size--;
    list->length--;
    //printf("popping = %d\n", result);
    return result;
}

void set(struct List* list, int item, int index){
    if(list->size == list->length + 1){
        list->size = list->size * 2;
        list->arr = realloc(list->arr, sizeof(int) * list->size);
    }
    if(list->size <= index){
        list->size = list->size * 2 + index;
        list->arr = realloc(list->arr, sizeof(int) * list->size); 
    }
    list->length++;
    list->arr[index] = item;
}

void free_list(struct List* list){
    free(list->arr);
    free(list);
}

void free_node(struct Node* node){
    if(node == NULL) return;
    if(node->neighbors == NULL) return;
    free(node->neighbors);
    free(node);
}

void free_graph(struct Graph* graph){
    if(graph == NULL) return;
    if(graph->node_list == NULL);
    for(int i =0;i <graph->size; i++)
        free_node(graph->node_list[i]);
    free(graph->node_list);
    free(graph);
}

char* list_to_str(struct List* input){
    char* result;
    char tmp[8];
    result = malloc(input->length * 8);
    strcpy(result, "");
    for(int i =0; i < input->length -1; i++){
        
        sprintf(tmp,"%d-> ",input->arr[i]);
        strcat(result, tmp);
    }
    sprintf(tmp, "%d\n", input->arr[input->length-1]);
    strcat(result, tmp);    
    return result;
}