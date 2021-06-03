/**
 * Basic Queue data structure implementation
 */
struct Queue
{
    int head, tail, length;
    unsigned capacity;
    int *array;
};

struct Queue *initialize_queue(unsigned capacity)
{
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->head = 0;
    queue->length = 0;
    queue->tail = capacity - 1;
    queue->array = (int *)malloc(queue->capacity * sizeof(int));
    return queue;
};

int isFull(struct Queue *queue)
{
    return queue->length == queue->capacity;
}

int isEmpty(struct Queue *queue)
{
    return queue->length == 0;
}

int enqueue(struct Queue *queue, int el)
{
    if (isFull(queue))
    {
        printf("Queue is full, unable to enqueue.\n");
    }
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->array[queue->tail] = el;
    queue->length += 1;
}

int dequeue(struct Queue *queue)
{
    if (isEmpty(queue))
        return -1;
    int el = queue->array[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->length -= 1;
    return el;
}

int head(struct Queue *queue)
{
    if (isEmpty(queue))
        return -1;
    return queue->array[queue->head];
}

int tail(struct Queue *queue)
{
    if (isEmpty(queue))
        return -1;
    return queue->array[queue->tail];
}