#ifndef CUT_QUEUE_H
#define CUT_QUEUE_H

#include "declarations.h"

CUT_NS_BEGIN

CUT_PRIVATE void cut_InitQueue(struct cut_Queue *queue) {
    queue->size = 0;
    queue->first = NULL;
    queue->last = NULL;
    queue->trash = NULL;
}

CUT_PRIVATE struct cut_QueueItem *cut_QueuePushRefCountedTest(struct cut_Queue *queue, int testId, int *refCount) {
    struct cut_QueueItem *item = (struct cut_QueueItem *) malloc(sizeof(struct cut_QueueItem));
    if (!item)
        cut_FatalExit("cannot allocate memory for queue item");

    item->testId = testId;
    item->refCount = refCount;
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

CUT_PRIVATE struct cut_QueueItem *cut_QueuePushTest(struct cut_Queue *queue, int testId) {
    struct cut_QueueItem *item = cut_QueuePushRefCountedTest(queue, testId, NULL);
    item->refCount = (int *) malloc(sizeof(int));
    if (!item->refCount)
        cut_FatalExit("cannot allocate memory for refCount");
    *item->refCount = 1;
    return item;
}

CUT_PRIVATE void cut_ClearQueueItem(struct cut_QueueItem *toFree) {
    --*toFree->refCount;
    if (!*toFree->refCount)
        free(toFree->refCount);

    cut_ClearQueue(&toFree->depending);
    free(toFree);
}


CUT_PRIVATE void cut_ClearQueueItems(struct cut_QueueItem *current) {
    while (current) {
        struct cut_QueueItem *toFree = current;
        current = current->next;
        cut_ClearQueueItem(toFree);
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


CUT_PRIVATE void cut_QueueMeltTest(struct cut_Queue *queue, struct cut_QueueItem *toMelt) {
    if (!toMelt->depending.size)
        return;

    struct cut_QueueItem **cursor;

    if (!queue->size) {
        queue->first = toMelt->depending.first;
        cursor = &queue->first;
    }
    else {
        queue->last->next = toMelt->depending.first;
        cursor = &queue->last->next;
    }

    while (*cursor) {
        if (1 < *(*cursor)->refCount) {
            struct cut_QueueItem *toFree = *cursor;
            *cursor = (*cursor)->next;
            cut_ClearQueueItem(toFree);
        }
        else {
            ++queue->size;
            queue->last = *cursor;
            cursor = &(*cursor)->next;
        }
    }
    queue->last->next = NULL;
    toMelt->depending.first = NULL;
    toMelt->depending.last = NULL;
    toMelt->depending.size = 0;
}

CUT_PRIVATE int cut_QueueRePushTest(struct cut_Queue *queue, struct cut_QueueItem *toRePush) {
    struct cut_QueueItem **cursor = &queue->trash;
    int found = 0;
    for (; *cursor; cursor = &(*cursor)->next) {
        if (*cursor == toRePush) {
            found = 1;
            *cursor = (*cursor)->next;
            break;
        }
    }
    if (!found)
        return 0;

    if (!queue->first)
        queue->first = toRePush;
    else
        queue->last->next = toRePush;
    queue->last = toRePush;
    toRePush->next = NULL;
    ++queue->size;
    return 1;
}

CUT_PRIVATE struct cut_QueueItem *cut_QueueAddTest(struct cut_Queue *queue, struct cut_QueueItem *toAdd) {
    struct cut_QueueItem *item = cut_QueuePushRefCountedTest(queue, toAdd->testId, toAdd->refCount);
    ++*item->refCount;
    item->depending.first = toAdd->depending.first;
    item->depending.last = toAdd->depending.last;
    item->depending.trash = toAdd->depending.trash;
    item->depending.size = toAdd->depending.size;
    memset(&toAdd->depending, 0, sizeof(struct cut_Queue));
    return item;
}

CUT_NS_END

#endif
