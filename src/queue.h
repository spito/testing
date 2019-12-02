#ifndef CUT_QUEUE_H
#define CUT_QUEUE_H

#ifndef CUT_MAIN
#error "cannot be standalone"
#endif



CUT_PRIVATE void cut_InitQueue(struct cut_Queue *queue) {
    queue->size = 0;
    queue->first = NULL;
    queue->last = NULL;
    queue->trash = NULL;
}

CUT_PRIVATE struct cut_QueueItem *cut_QueuePushTest(struct cut_Queue *queue, int testId) {
    struct cut_QueueItem *item = (struct cut_QueueItem *) malloc(sizeof(struct cut_QueueItem));
    if (!item)
        cut_FatalExit("cannot allocate memory for queue item");

    item->testId = testId;
    item->next = NULL;
    cut_InitQueue(&item->depending);
    if (!queue->first)
        queue->first = item;
    else
        queue->last->next = item;
    queue->last = item;
    ++queue->size;
    return item;
}

CUT_PRIVATE void cut_ClearQueueItems(struct cut_QueueItem *current) {
    while (current) {
        struct cut_QueueItem *toFree = current;
        current = current->next;

        cut_ClearQueue(&toFree->depending);
        free(toFree);
    };
}


CUT_PRIVATE void cut_ClearQueue(struct cut_Queue *queue) {
    cut_ClearQueueItems(queue->first);
    cut_ClearQueueItems(queue->trash);
    queue->first = NULL;
    queue->last = NULL;
    queue->trash = NULL;
    queue->size = 0;
}


CUT_PRIVATE struct cut_QueueItem *cut_QueuePopTest(struct cut_Queue *queue) {
    if (!queue->size)
        return NULL;
    
    struct cut_QueueItem *item = queue->first;
    queue->first = item->next;
    if (!queue->first)
        queue->last = NULL;
    --queue->size;

    item->next = queue->trash;
    queue->trash = item;

    return item;
}


CUT_PRIVATE void cut_MeltQueueItem(struct cut_Queue *queue, struct cut_QueueItem *toMelt) {
    if (!toMelt->depending.size)
        return;

    if (!queue->size)
        queue->first = toMelt->depending.first;
    else
        queue->last->next = toMelt->depending.first;
    queue->last = toMelt->depending.last;
    queue->size += toMelt->depending.size;
}

#endif
