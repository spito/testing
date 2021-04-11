#ifndef CUT_FILE_OPERATIONS_H
#define CUT_FILE_OPERATIONS_H

#include "declarations.h"
#include "globals.h"
#include "messages.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct cut_FileCompare {
    cut_Reporter reporter;
    const char *expectedContent;
    size_t expectedLength;
    const char *givenContent;
    size_t givenLength;
    const char *fdString;
    const char *file;
    unsigned line;
};

CUT_PRIVATE char *cut_ReadFile(int fd, size_t *sizeOut) {
    enum { CHUNK_SIZE = 512 };
    size_t size = CHUNK_SIZE;
    size_t position = 0;
    char *content = (char *) malloc(size);

    int64_t justRead = 0;
    while (1) {
        justRead = cut_Read(fd, content + position, CHUNK_SIZE);
        if (justRead <= 0)
            break;

        position += justRead;
        size = position + CHUNK_SIZE;
        char *extension = (char *) realloc(content, size);
        if (!extension) {
            free(content);
            return NULL;
        }
        content = extension;
    }

    if (justRead == -1) {
        free(content);
        return NULL;
    }

    *sizeOut = position;
    return content;
}

CUT_PRIVATE char *cut_ReadWholeFile(FILE *file, size_t *sizeOut) {
    if (!file || !sizeOut)
        return NULL;

    fflush(file);
    int fd = cut_ReopenFile(file);
    char *result = cut_ReadFile(fd, sizeOut);

    cut_CloseFile(fd);
    return result;
}


CUT_PRIVATE void cut_FileCompareBinary(const struct cut_FileCompare *pack) {
    if (pack->expectedLength != pack->givenLength) {
        cut_FormatMessage(pack->reporter, pack->file, pack->line,
                          "Lengths of the contents differ; expected: %u, given: %u",
                          pack->expectedLength, pack->givenLength);
        return;
    }

    for (unsigned i = 0; i < pack->expectedLength; ++i) {
        if (pack->expectedContent[i] != pack->givenContent[i]) {
            cut_FormatMessage(pack->reporter, pack->file, pack->line,
                              "Content differs at position %u: expected 0x%X, given 0x%X",
                              i, pack->expectedContent[i], pack->givenContent[i]);
            return;
        }
    }
}

// TODO: implement diff algorithm
CUT_PRIVATE void cut_FileCompareText(const struct cut_FileCompare *pack) {
    cut_FileCompareBinary(pack);
}

void cut_FileCompare(cut_Reporter reporter, FILE *fd, const char *expected, int mode, const char *fdStr, const char *file, unsigned line) {
    struct cut_FileCompare pack = {};
    pack.reporter = reporter;
    pack.expectedContent = expected;
    pack.expectedLength = strlen(expected);
    pack.fdString = fdStr;
    pack.file = file;
    pack.line = line;

    (pack.givenContent = cut_ReadWholeFile(fd, &pack.givenLength)) || CUT_DIE("cannot alocate memory");

    switch (mode) {
    case CUT_MODE_BINARY:
        cut_FileCompareBinary(&pack);
        break;

    case CUT_MODE_TEXT:
        cut_FileCompareText(&pack);
        break;

    default:
        CUT_DIE("unknown mode");
    }

    free((void *)pack.givenContent);
}

#endif