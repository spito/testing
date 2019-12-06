#ifndef CUT_GC_H
#define CUT_GC_H

#ifndef CUT_MAIN
#error "cannot be standalone"
#endif

struct cut_GCgroup {
    void **data;
    size_t size;
    size_t capacity;
};

struct cut_GC {
    struct cut_GCgroup self;

    struct cut_GCgroup **groups;
    size_t groupSize;
    size_t groupCapacity;
};


#define CUT_GC_TRESHOLD 1024

CUT_PRIVATE struct cut_GC *cut_GCget();

CUT_PRIVATE void cut_GCpurge() {
    struct cut_GC *gc = cut_GCget();
    for (size_t i = 0; i < gc->self.size; ++i)
        free(gc->self.data[i]);
    free(gc->self.data);

    for (size_t i = 0; i < gc->groupSize; ++i) {
        cut_GCgroupFree(gc->groups[i]);
    }
    free(gc->groups);
    free(gc->self.data);
}

CUT_PRIVATE void cut_GCinit(struct cut_GC *gc) {
    gc->self.capacity = 64;
    gc->self.data = (void **) malloc(sizeof(void *) * gc->self.capacity);

    gc->groupCapacity = 64;
    gc->groups = (struct cut_GCgroup **) malloc(sizeof(void *) * gc->groupCapacity);

    atexit(cut_GCpurge);
}

CUT_PRIVATE struct cut_GC *cut_GCget() {
    static struct cut_GC gc = {{NULL, 0, 0}, NULL, 0, 0};
    if (!gc.self.data)
        cut_GCinit(&gc);
    return &gc;
}

CUT_PRIVATE int cut_GCcmp(const void *_a, const void *_b) {
    return *(const void **)_b - *(const void **)_a;
}

CUT_PRIVATE void cut_GCsort(struct cut_GCgroup *group) {

    qsort(group->data, group->size, sizeof(void *), cut_GCcmp);
}

CUT_PRIVATE void *cut_GCgroupAlloc(void *self, size_t size) {
    struct cut_GCgroup *group = (struct cut_GCgroup *)self;

    if (group->size == group->capacity) {
        if (CUT_GC_TRESHOLD <= group->capacity)
            group->capacity += CUT_GC_TRESHOLD;
        else
            group->capacity *= 2;
        void **data = (void **) realloc(group->data, sizeof(void *) * group->capacity);
        if (!data)
            cut_FatalExit("cannot allocate memory for GC");
        group->data = data;
    }
    void *data = malloc(size);
    if (!data)
        cut_FatalExit("cannot allocate memory for GC");
    memset(data, 0, size);
    group->data[group->size++] = data;
    cut_GCsort(group);
    return data;
}

CUT_PRIVATE void *cut_GCgroupRealloc(void *self, void *ptr, size_t size) {
    struct cut_GCgroup *group = (struct cut_GCgroup *)self;

    if (!ptr)
        return cut_GCgroupAlloc(self, size);

    void **place = bsearch(&ptr, group->data, group->size, sizeof(void *), cut_GCcmp);

    if (!place)
        cut_FatalExit("invalid pointer to realloc");

    void *data = (void *) realloc(ptr, size);
    if (!data)
        cut_FatalExit("cannot realloc memory");
    *place = data;
    cut_GCsort(group);
    return data;
}


CUT_PRIVATE void *cut_GCalloc(size_t size) {
    struct cut_GC *gc = cut_GCget();
    return cut_GCgroupAlloc(gc, size);
}

CUT_PRIVATE void *cut_GCrealloc(void *ptr, size_t size) {
    struct cut_GC *gc = cut_GCget();
    return cut_GCgroupRealloc(gc, ptr, size);
}


CUT_PRIVATE void cut_GCgroupInit(void *self) {
    struct cut_GCgroup *group = (struct cut_GCgroup *)self;
    struct cut_GC *gc = cut_GCget();
    group->capacity = 64;
    group->data = (void **) malloc(sizeof(void *) * group->capacity);
    memset(group->data, 0, sizeof(void *) * group->capacity);

    if (gc->groupSize == gc->groupCapacity) {
        if (CUT_GC_TRESHOLD <= gc->groupCapacity)
            gc->groupCapacity += CUT_GC_TRESHOLD;
        else
            gc->groupCapacity *= 2;
        void **data = (struct cut_GCgroup **) realloc(gc->groups, sizeof(void *) * gc->groupCapacity);
        if (!data)
            cut_FatalExit("cannot allocate memory for GC");
        gc->groups = data;
    }
    gc->groups[gc->groupSize++] = self;
}

CUT_PRIVATE void cut_GCgroupFree(void *self) {
    struct cut_GCgroup *group = (struct cut_GCgroup *) self;
    if (!group->data)
        return;
    for (size_t i = 0; i < group->size; ++i)
        free(group->data[i]);
    free(group->data);
    group->data = NULL;
}


#endif
