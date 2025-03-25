#include <stdlib.h>
#include <threads.h>

#include "queue.h"

typedef struct  Qnode{
    void *data;
    struct Qnode *next;
} Qnode;

typedef struct Qlist{
    Qnode *head;
    Qnode *last;
} Qlist;
/**********Helper functions**********/

//pop first node in queue and return it's data. free Qnode.
static void* pop(Qlist *queue){
    Qnode *curr_head = queue->head;
    void* ans = curr_head->data;
    if(curr_head->next == NULL){
        queue->last = NULL;
    }
    queue->head = curr_head->next;
    free(curr_head);
    return ans;
}

static void* get_from_index(Qlist *queue, int index){
    Qnode *prev ,*curr;
    void* ans;
    if(index == 0){
        return pop(queue);
    }
    curr = queue->head;
    prev = curr;
    while(index > 0){
        prev = curr;
        curr = curr->next;
        index--;
    }
    ans = curr->data;
    prev->next = curr->next;
    if(queue->last == curr){
        queue->last = prev;
    }
    free(curr);
    return ans;
}


//free Qlist and all it's Qnodes.
static void free_Qlist(Qlist *list){
    Qnode *head = list->head;
    Qnode *tmp;
    while(head != NULL){
        tmp = head;
        head = head->next;
        free(tmp);
    }
    free(list);
}
/*************************************/

static Qlist *data_Q; //Queue data structure
static Qlist *threads_Q; //Will keep sleeping threads in FIFO order
static size_t queue_size;
static size_t waiting_counter;
static size_t visited_counter;
static mtx_t lock;

void initQueue(void){
    mtx_init(&lock, mtx_plain); //"No need to check for errors"
    mtx_lock(&lock);
    data_Q = (Qlist *)malloc(sizeof(Qlist)); //"You can assume malloc doesn't fail"
    threads_Q = (Qlist *)malloc(sizeof(Qlist));
    data_Q->head = NULL;
    data_Q->last = NULL;
    threads_Q->head = NULL;
    threads_Q->last = NULL;
    queue_size = 0;
    waiting_counter = 0;
    visited_counter = 0;
    mtx_unlock(&lock);
}

void destroyQueue(void){
    mtx_lock(&lock);
    free_Qlist(data_Q);
    free_Qlist(threads_Q);
    mtx_unlock(&lock);
    mtx_destroy(&lock);
}

void enqueue(void* data){
    Qnode *tmp = (Qnode *)malloc(sizeof(Qnode));
    tmp->data = data;
    tmp->next = NULL;
    mtx_lock(&lock);
    if(data_Q->head == NULL){ //No elements in the queue
        data_Q->head = tmp;
    }else{
        data_Q->last->next = tmp;
    }
    data_Q->last = tmp;
    queue_size++;

    if(threads_Q->head != NULL){ //There is sleeping thread waiting for available item for it to process
        cnd_signal((cnd_t *)threads_Q->head->data); //Wake up the oldest sleeping thread
    }

    mtx_unlock(&lock);
}

void* dequeue(void){
    cnd_t cv;
    Qnode *thread_node;
    void *item;
    bool waited = false; //flag that indicates thread was waiting
    mtx_lock(&lock);
    if(queue_size == 0 || threads_Q->head != NULL){ //No available item to process
        cnd_init(&cv);
        //Build thread node to insert to the thread's queue
        thread_node = (Qnode *)malloc(sizeof(Qnode));
        thread_node->data = &cv;
        thread_node->next = NULL;

        //Insert thread_node to thread's queue
        if(threads_Q->head == NULL){
            threads_Q->head = thread_node;
        }else{
            threads_Q->last->next = thread_node;
        }
        threads_Q->last = thread_node;
        
        waiting_counter++;
        while(queue_size == 0 || threads_Q->head->data != &cv){ //Go to sleep until it's your turn.
            cnd_wait(&cv, &lock);
        }
        cnd_destroy(&cv);
        pop(threads_Q);
        waited = true;
    }
    item = pop(data_Q);
    queue_size--;
    visited_counter++;
    if(waited){waiting_counter--;} //If thread wasn't waiting we shouldn't decrease 
    if(queue_size > 0 && threads_Q->head != NULL){ //There is another item in the queue-
                                                   //wake up the next sleeping thread.
                                                   //Must be done in case of more then 1 thread is sleeping
                                                   //and enqueue happen in row (They will wake up the same thread every time),
                                                   //or TryDequeue happen?
        cnd_signal((cnd_t *)threads_Q->head->data);
    }
    mtx_unlock(&lock);
    return item;
}

bool tryDequeue(void** pointer){ 
    /*void *item;
    mtx_lock(&lock);
    if(data_Q->head != NULL){ //There is item in the queue -success!
        item = pop(data_Q);
        *pointer = item; //return the item via the argument
        queue_size--;
        visited_counter++;
        mtx_unlock(&lock);
        return true;
    }
    //Queue is empty
    mtx_unlock(&lock);
    return false;*/
    //
    void *item;
    int index;
    mtx_lock(&lock);
    if(data_Q->head == NULL || waiting_counter >= queue_size ){ //No available items in the queue
        mtx_unlock(&lock);
        return false;
    }
    //There is available item and queue_size > waiting_counter
    index = waiting_counter;
    item = get_from_index(data_Q, index);
    *pointer = item; //return the item via the argument
    queue_size--;
    visited_counter++;
    mtx_unlock(&lock);
    return true;
}

size_t size(void){
    size_t ans;
    mtx_lock(&lock);
    ans = queue_size;
    mtx_unlock(&lock);
    return ans;
}

size_t waiting(void){
    return waiting_counter;
}

size_t visited(void){
    return visited_counter;
}
