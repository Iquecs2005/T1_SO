#include <stdlib.h>

#include "Queue.h"

typedef struct node Node;
struct node
{
    int pid;
    Node* next;  
};

struct queue
{
    Node* start;
    Node* end;
};

Queue* CreateQueue()
{
    Queue* newQueue = (Queue*)malloc(sizeof(Queue));
    if (newQueue == NULL)
        return NULL;
    newQueue->end = NULL;
    newQueue->start = NULL;

    return newQueue;
};

char IsEmpty(Queue* queue)
{
    if (queue->start == NULL)
        return 1;
    return 0;
}

void Enqueue(Queue* queue, int pid)
{
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL)
        return;
    newNode->pid = pid;
    newNode->next = NULL;

    if (IsEmpty(queue))
    {
        queue->start = newNode;
    }
    else
    {
        queue->end->next = newNode;
    }
    queue->end = newNode;

    return;
}

int pop(Queue* queue)
{
    if (IsEmpty(queue))
        return -1;
    
    int pid = queue->start->pid;
    queue->start = queue->start->next;

    return pid;
}