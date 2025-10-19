typedef struct queue Queue;

Queue* CreateQueue();
char IsEmpty(Queue* queue);
void Enqueue(Queue* queue, int pid);
int pop(Queue* queue);