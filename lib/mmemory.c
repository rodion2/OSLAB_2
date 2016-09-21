#include "mmemory.h"
#include "stdlib.h"

#define TRUE            1
#define FALSE            0
#define ALLOCSIZE        128
#define NULL_SYMBOL        '\0'
#define SWAPING_NAME    "swap.dat"

const int SUCCESSFUL_CODE = 0;
const int INCORRECT_PARAMETERS_ERROR = -1;
const int NOT_ENOUGH_MEMORY_ERROR = -2;
const int OUT_OF_RANGE_ERROR = -2;
const int UNKNOWN_ERROR = 1;

struct segment {
    struct segment *pNext;
    unsigned int szBlock;
    unsigned int offsetBlock;
};

struct pageInfo {
    unsigned long offsetPage;
    char isUse;
};

typedef struct page {
    struct segment *pFirstFree;
    struct segment *pFirstUse;
    unsigned int maxSizeFreeBlock;
    struct pageInfo pInfo;
    unsigned int activityFactor;
} page;


struct page *pageTable;


int numberOfPages = 0;


int sizePage = 0;


VA allocbuf;

int _malloc(VA *ptr, size_t szBlock) {
    ptr = (char**)malloc(szBlock);
    if (ptr == NULL) return 
    return -1;
}

int _free(VA ptr){
    return -1;
}

int _read(VA ptr, void *pBuffer, size_t szBuffer){
    return -1;
}

int _write(VA ptr, void *pBuffer, size_t szBuffer){
    return -1;
}

int _init(int n, size_t szPage){
    return -1;
}