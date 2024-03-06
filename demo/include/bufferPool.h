/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef __BUFFER_POOL_H_
#define __BUFFER_POOL_H_

#include "cam_list.h"
#include "cam_interface.h"
#include "dmabufheapAllocator.h"

#undef PAGE_SIZE
#define PAGE_SIZE            (4096)
#define BUFFER_POOL_NAME_LEN (32)
#define BUFFER_POOL_MAX_SIZE (32)

#define RAW10_DUMP_SIZE(w, h) ((w / 12 + (w % 12 ? 1 : 0)) * 16 * h)
#define RAW12_DUMP_SIZE(w, h) ((w / 10 + (w % 10 ? 1 : 0)) * 16 * h)

typedef struct spmBUFFER_POOL_S {
    IMAGE_BUFFER_S bufInfo;
    BUFFER_S mem_block[5];
    LIST_HANDLE buf_list;
    IMAGE_BUFFER_S buffers[BUFFER_POOL_MAX_SIZE];
    uint32_t size;
    uint32_t buffer_size;
    char name[BUFFER_POOL_NAME_LEN];
} BUFFER_POOL;

BUFFER_POOL *create_buffer_pool(int width, int height, PIXEL_FORMAT_E format, const char *name);
void destroy_buffer_pool(BUFFER_POOL *pool);
int32_t buffer_pool_alloc(BUFFER_POOL *pool, uint32_t buffer_count);
void buffer_pool_free(BUFFER_POOL *pool);
IMAGE_BUFFER_S *buffer_pool_get_buffer(BUFFER_POOL *pool);
void buffer_pool_put_buffer(BUFFER_POOL *pool, IMAGE_BUFFER_S *buffer);
int buffer_pool_invlide_cache(BUFFER_POOL *pool, IMAGE_BUFFER_S *imageBuffer);

int frameinfo_buffer_alloc(IMAGE_BUFFER_S *frameInfoBuf);
int frameinfo_buffer_free(IMAGE_BUFFER_S *frameInfoBuf);

#endif
