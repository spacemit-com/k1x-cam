/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef __DMABUF_HEAP_ALLOCATOR_H__
#define __DMABUF_HEAP_ALLOCATOR_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum CACHE_OPS_DIR {
    CACHE_OPS_DIR_CLEAN,
    CACHE_OPS_DIR_INVALIDATE,
    CACHE_OPS_DIR_CLEAN_INVALIDATE,
} CACHE_OPS_DIR_E;

typedef struct BUFFER {
    union {
        uint64_t phyAddr;
        int32_t blockId;
        int32_t fd;
    } m;
    void *addr;
} BUFFER_S;

int32_t dmabufheapAlloc(BUFFER_S *buffer, uint32_t size, int continuous);
int32_t dmabufheapFree(const BUFFER_S *buffer);
int32_t dmabufheapCacheOps(const BUFFER_S *buffer, CACHE_OPS_DIR_E dir);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* __DMABUF_HEAP_ALLOCATOR_H__ */
