#ifndef CUT_FRAGMENTS_H
#define CUT_FRAGMENTS_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#define CUT_MAX_SLICE_COUNT 255
#define CUT_MAX_SERIALIZED_LENGTH (256*256-1)

struct cut_FragmentSlice {
    char *data;
    uint16_t length;
    struct cut_FragmentSlice *next;
};

struct cut_Fragment {
    uint32_t serializedLength;
    uint8_t id;
    uint8_t sliceCount;
    char *serialized;
    struct cut_FragmentSlice *slices;
    struct cut_FragmentSlice *lastSlice;
};

struct cut_FragmentHeader {
    uint32_t length;
    uint8_t id;
    uint8_t sliceCount;
};

typedef union {
    struct {
        uint32_t length;
        uint32_t processed;
    } structured;
    uint64_t raw;
} cut_FragmentReceiveStatus;
#define CUT_FRAGMENT_RECEIVE_STATUS {0}

void cut_FragmentInit(struct cut_Fragment *fragments, int id) {
    fragments->id = id;
    fragments->sliceCount = 0;
    fragments->serializedLength = 0;
    fragments->serialized = NULL;
    fragments->slices = NULL;
    fragments->lastSlice = NULL;
}

void cut_FragmentClean(struct cut_Fragment *fragments) {
    if (!fragments)
        return;
    if (fragments->serialized)
        free(fragments->serialized);
    while (fragments->slices) {
        struct cut_FragmentSlice *current = fragments->slices;
        fragments->slices = fragments->slices->next;
        free(current->data);
        free(current);
    }
}

void *cut_FragmentReserve(struct cut_Fragment *fragments, size_t length, int *sliceId) {
    if (!fragments)
        return NULL;
    if (fragments->sliceCount == CUT_MAX_SLICE_COUNT)
        return NULL;
    struct cut_FragmentSlice *slice = malloc(sizeof(struct cut_FragmentSlice));
    if (!slice)
        return NULL;
    slice->data = malloc(length);
    if (!slice->data) {
        free(slice);
        return NULL;
    }
    slice->length = length;
    slice->next = NULL;
    if (fragments->lastSlice)
        fragments->lastSlice->next = slice;
    else
        fragments->slices = slice;
    fragments->lastSlice = slice;
    if (sliceId)
        *sliceId = fragments->sliceCount;
    ++fragments->sliceCount;
    return slice->data;
}

void *cut_FragmentAddString(struct cut_Fragment *fragments, const char *str) {
    if (!fragments)
        return NULL;
    size_t length = strlen(str) + 1;
    void *data = cut_FragmentReserve(fragments, length, NULL);
    if (!data)
        return NULL;
    memcpy(data, str, length);
    return data;
}

char *cut_FragmentGet(struct cut_Fragment *fragments, int sliceId, size_t *length) {
    if (!fragments)
        return NULL;
    if (sliceId >= fragments->sliceCount)
        return NULL;
    struct cut_FragmentSlice *current = fragments->slices;
    for (int id = 0; id < sliceId; ++id) {
        current = current->next;
    }
    if (length)
        *length = current->length;
    return current->data;
}

int cut_FragmentSerialize(struct cut_Fragment *fragments) {
    if (!fragments)
        return 0;
    if (fragments->serialized)
        free(fragments->serialized);
    uint32_t length = sizeof(struct cut_FragmentHeader);
    length += fragments->sliceCount * sizeof(uint16_t);
    uint32_t contentOffset = length;
    for (struct cut_FragmentSlice *current = fragments->slices; current; current = current->next) {
        length += current->length;
    }
    if (length > CUT_MAX_SERIALIZED_LENGTH)
        return 0;
    fragments->serialized = malloc(length);
    if (!fragments->serialized)
        return 0;
    fragments->serializedLength = length;
    struct cut_FragmentHeader *header = (struct cut_FragmentHeader *)fragments->serialized;
    header->length = length;
    header->id = fragments->id;
    header->sliceCount = fragments->sliceCount;
    uint16_t *sliceLength = (uint16_t *)(header + 1);
    for (struct cut_FragmentSlice *current = fragments->slices; current; current = current->next) {
        *sliceLength = current->length;
        ++sliceLength;
        memcpy(fragments->serialized + contentOffset, current->data, current->length);
        contentOffset += current->length;
    }
    return 1;
}

int cut_FragmentDeserialize(struct cut_Fragment *fragments) {
    if (!fragments)
        return 0;
    if (!fragments->serialized)
        return 0;
    struct cut_FragmentHeader *header = (struct cut_FragmentHeader *)fragments->serialized;
    fragments->serializedLength = header->length;
    fragments->id = header->id;
    fragments->sliceCount = 0;
    uint16_t *sliceLength = (uint16_t *)(header + 1);
    uint32_t contentOffset = sizeof(struct cut_FragmentHeader)
                           + header->sliceCount * sizeof(uint16_t);
    for (uint16_t slice = 0; slice < header->sliceCount; ++slice, ++sliceLength) {
        void *data = cut_FragmentReserve(fragments, *sliceLength, NULL);
        if (!data)
            return 0;
        memcpy(data, fragments->serialized + contentOffset, *sliceLength);
        contentOffset += *sliceLength;
    }
    return 1;
}

ssize_t cut_FragmentReceiveContinue(cut_FragmentReceiveStatus *status, void *data, ssize_t length) {
    if (!status)
        return 0;
    if (!data) {
        status->raw = 0;
        return sizeof(struct cut_FragmentHeader);
    }
    if (length <= 0)
        return length;
    if (!status->structured.length) {
        if (length != sizeof(struct cut_FragmentHeader))
            return 0;
        status->structured.length = ((struct cut_FragmentHeader*)data)->length;
    }
    status->structured.processed += length;
    return status->structured.length - status->structured.processed;
}

size_t cut_FragmentReceiveProcessed(cut_FragmentReceiveStatus *status) {
    if (!status)
        return 0;
    return status->structured.processed;
}

#endif
