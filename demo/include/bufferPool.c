/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include "bufferPool.h"

#include <stdio.h>
#include <string.h>

#include "ISPGlobalDefine.h"
#include "cam_log.h"

#define ALIGN_N(x, n) (((x) + (n)-1) & (~((n)-1)))

static uint32_t get_buffer_size(IMAGE_BUFFER_S *bufInfo, uint32_t align)
{
    bool has_dwt = false;
    uint32_t buffer_size = 0;
    int32_t i = 0, divisor = 0;
    IMAGE_BUFFER_PLANE_S *planes = bufInfo->planes;
    int bpp = 10;
    int pixelPerLine = 128 / bpp;

    switch (bufInfo->format) {
        case PIXEL_FORMAT_NV12_DWT:
            has_dwt = true;
        case PIXEL_FORMAT_NV12:
            bufInfo->numPlanes = 2;
            planes[0].width = bufInfo->size.width;
            planes[0].height = bufInfo->size.height;
            planes[0].stride = align ? ALIGN_N(planes[0].width, align) : planes[0].width;
            planes[0].length = planes[0].stride * planes[0].height;
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            planes[1].width = bufInfo->size.width;
            planes[1].height = bufInfo->size.height / 2;
            planes[1].stride = align ? ALIGN_N(planes[1].width, align) : planes[1].width;
            planes[1].length = planes[1].stride * planes[1].height;
            planes[1].virAddr = 0;
            buffer_size += planes[1].length;
            break;
        case PIXEL_FORMAT_FBC_DWT:
            has_dwt = true;
        case PIXEL_FORMAT_FBC:
            bufInfo->numPlanes = 2;
            planes[0].width = ((bufInfo->size.width + 31) / 32) * ((bufInfo->size.height + 3) / 4) * 8;
            planes[0].height = 1;
            planes[0].stride = planes[0].width;
            planes[0].length = planes[0].stride * planes[0].height;
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            planes[1].width = ((bufInfo->size.width + 31) / 32) * ((bufInfo->size.height + 3) / 4) * 192;
            planes[1].height = 1;
            planes[1].stride = planes[1].width;
            planes[1].length = planes[1].stride * planes[1].height;
            planes[1].virAddr = 0;
            buffer_size += planes[1].length;
            break;
        case PIXEL_FORMAT_RAW:
            bufInfo->numPlanes = 1;
            planes[0].width = bufInfo->size.width * 2;
            planes[0].height = bufInfo->size.height;
            planes[0].stride = planes[0].width;
            planes[0].length = ALIGN_N(planes[0].stride * planes[0].height, PAGE_SIZE);
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            break;
        case PIXEL_FORMAT_RAW_10BPP:
            bufInfo->numPlanes = 1;
            bpp = 10;  // temp hard code , must get bpp
            pixelPerLine = 128 / bpp;
            if (bpp == 8) {
                planes[0].width = bufInfo->size.width;
            } else {
                planes[0].width = (bufInfo->size.width + pixelPerLine - 1) / pixelPerLine * 16;
            }
            planes[0].height = bufInfo->size.height;
            planes[0].stride = planes[0].width;
            planes[0].length =
                RAW10_DUMP_SIZE(bufInfo->size.width, bufInfo->size.height);  // temp hard code, must get bpp
            buffer_size += planes[0].length;
            break;
        case PIXEL_FORMAT_RAW_12BPP:
            bufInfo->numPlanes = 1;
            bpp = 12;  // temp hard code , must get bpp
            pixelPerLine = 128 / bpp;
            if (bpp == 8) {
                planes[0].width = bufInfo->size.width;
            } else {
                planes[0].width = (bufInfo->size.width + pixelPerLine - 1) / pixelPerLine * 16;
            }
            planes[0].height = bufInfo->size.height;
            planes[0].stride = planes[0].width;
            planes[0].length =
                RAW12_DUMP_SIZE(bufInfo->size.width, bufInfo->size.height);  // temp hard code, must get bpp
            buffer_size += planes[0].length;
            break;
        case PIXEL_FORMAT_RGB565:
            bufInfo->numPlanes = 1;
            planes[0].width = bufInfo->size.width;
            planes[0].height = bufInfo->size.height;
            planes[0].stride = bufInfo->size.width * 2;
            planes[0].length = ALIGN_N(planes[0].stride * planes[0].height, PAGE_SIZE);
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            break;
        case PIXEL_FORMAT_RGB888:
            bufInfo->numPlanes = 1;
            planes[0].width = bufInfo->size.width;
            planes[0].height = bufInfo->size.height;
            planes[0].stride = bufInfo->size.width * 3;
            planes[0].length = ALIGN_N(planes[0].stride * planes[0].height, PAGE_SIZE);
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            break;
        case PIXEL_FORMAT_Y210:
            bufInfo->numPlanes = 1;
            planes[0].width = bufInfo->size.width;
            planes[0].height = bufInfo->size.height;
            planes[0].stride = bufInfo->size.width * 4;
            planes[0].length = ALIGN_N(planes[0].stride * planes[0].height, PAGE_SIZE);
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            break;
        case PIXEL_FORMAT_P210:
            bufInfo->numPlanes = 2;
            planes[0].width = bufInfo->size.width;
            planes[0].height = bufInfo->size.height;
            planes[0].stride = bufInfo->size.width * 2;
            planes[0].length = planes[0].stride * planes[0].height;
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            planes[1].width = bufInfo->size.width;
            planes[1].height = bufInfo->size.height;
            planes[1].stride = bufInfo->size.width * 2;
            planes[1].length = planes[1].stride * planes[1].height;
            planes[1].virAddr = 0;
            buffer_size += planes[1].length;
            break;
        case PIXEL_FORMAT_P010:
            bufInfo->numPlanes = 2;
            planes[0].width = bufInfo->size.width;
            planes[0].height = bufInfo->size.height;
            planes[0].stride = bufInfo->size.width * 2;
            planes[0].length = planes[0].stride * planes[0].height;
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            planes[1].width = bufInfo->size.width;
            planes[1].height = bufInfo->size.height / 2;
            planes[1].stride = bufInfo->size.width * 2;
            planes[1].length = planes[1].stride * planes[1].height;
            planes[1].virAddr = 0;
            buffer_size += planes[1].length;
            break;
        default:
            CLOG_INFO("unsupport format %d\n", bufInfo->format);
            break;
    }
    if (has_dwt) {
        for (i = 1; i <= 4; i++) {
            divisor = 1 << i;
            if (i == 1) {
                planes = &(bufInfo->dwt1[0]);
            } else if (i == 2) {
                planes = &(bufInfo->dwt2[0]);
            } else if (i == 3) {
                planes = &(bufInfo->dwt3[0]);
            } else {
                planes = &(bufInfo->dwt4[0]);
            }
            planes[0].width = ((ALIGN_N(bufInfo->size.width, 64) / divisor * 10 + 7) / 8);
            planes[0].height = (ALIGN_N(bufInfo->size.height, 32) / divisor);
            planes[0].stride = planes[0].width;
            planes[0].length = ALIGN_N(planes[0].stride * planes[0].height, PAGE_SIZE);
            planes[0].virAddr = 0;
            buffer_size += planes[0].length;
            planes[1].width = ((ALIGN_N(bufInfo->size.width, 64) / divisor * 10 + 7) / 8);
            planes[1].height = (ALIGN_N(bufInfo->size.height, 32) / divisor) / 2;
            planes[1].stride = planes[1].width;
            planes[1].length = ALIGN_N(planes[1].stride * planes[1].height, PAGE_SIZE);
            planes[1].virAddr = 0;
            buffer_size += planes[1].length;
        }
    }

    return buffer_size;
}

BUFFER_POOL *create_buffer_pool(int width, int height, PIXEL_FORMAT_E format, const char *name)
{
    BUFFER_POOL *pool = malloc(sizeof(BUFFER_POOL));
    if (!pool) {
        CLOG_ERROR("%s malloc pool for %s failed", __func__, name);
        return NULL;
    }
    pool->bufInfo.size.width = width;
    pool->bufInfo.size.height = height;
    pool->bufInfo.format = format;
    pool->bufInfo.type = BUF_ALLOC_TYPE_DMABUF_HEAP;
    pool->buf_list = List_Create(false);
    strncpy(pool->name, name, BUFFER_POOL_NAME_LEN - 1);
    return pool;
}

void destroy_buffer_pool(BUFFER_POOL *pool)
{
    if (pool) {
        List_Destroy(pool->buf_list);
        free(pool);
    }
}

int buffer_pool_invlide_cache(BUFFER_POOL *pool, IMAGE_BUFFER_S *imageBuffer)
{
    uint32_t i;
    int allocIndex = -1;

    for (i = 0; i < pool->size; i++) {
        if (imageBuffer->m.fd == pool->mem_block[i].m.fd) {
            allocIndex = i;
            break;
        }
    }

    if (pool && allocIndex >= 0) {
        dmabufheapCacheOps(&pool->mem_block[allocIndex], CACHE_OPS_DIR_INVALIDATE);
    }

    return 0;
}

int32_t buffer_pool_alloc(BUFFER_POOL *pool, uint32_t buffer_count)
{
    uint32_t block_size = 0, buffer_size = 0, i = 0, j = 0;
    uint32_t offset = 0;
    int32_t ret = 0;
    bool has_dwt = false;
    IMAGE_BUFFER_S *bufInfo = &(pool->bufInfo);
    IMAGE_BUFFER_S *buffer = NULL;
    IMAGE_BUFFER_PLANE_S *planes = NULL;

    if (buffer_count > 5) {
        CLOG_ERROR("pool(%s) buffer count(%u) could not be larger than %u\n", pool->name, buffer_count, 5);
        return -1;
    }

    if (bufInfo->format == PIXEL_FORMAT_NV12_DWT || bufInfo->format == PIXEL_FORMAT_FBC_DWT) {
        has_dwt = true;
    }

    buffer_size = get_buffer_size(&(pool->bufInfo), 0);
    CLOG_INFO("pool(%s) buffer_size=%u buffer_count=%u block_size=%u\n", pool->name, buffer_size, buffer_count,
              block_size);
    CLOG_INFO("alloc buffer continues = 1\n");
    for (i = 0; i < buffer_count; i++) {
        offset = 0;
        ret = dmabufheapAlloc(&pool->mem_block[i], buffer_size, 1);
        if (ret < 0) {
            CLOG_ERROR("alloc buffer for pool(%s) failed\n", pool->name);
            return ret;
        }

        buffer = &(pool->buffers[i]);
        memcpy(buffer, bufInfo, sizeof(*buffer));
        buffer->m.fd = pool->mem_block[i].m.fd;
        planes = &(buffer->planes[0]);
        for (j = 0; j < bufInfo->numPlanes; j++) {
            planes[j].offset = offset;
            planes[j].virAddr = (void *)((unsigned long)pool->mem_block[i].addr + offset);
            offset += planes[j].length;
            planes[j].fd = buffer->m.fd;
        }
        if (has_dwt) {
            planes = &(buffer->dwt1[0]);
            for (j = 0; j < 2; j++) {
                planes[j].offset = offset;
                planes[j].virAddr = (void *)((unsigned long)pool->mem_block[i].addr + offset);
                offset += planes[j].length;
                planes[j].fd = buffer->m.fd;
            }
            planes = &(buffer->dwt2[0]);
            for (j = 0; j < 2; j++) {
                planes[j].offset = offset;
                planes[j].virAddr = (void *)((unsigned long)pool->mem_block[i].addr + offset);
                offset += planes[j].length;
                planes[j].fd = buffer->m.fd;
            }
            planes = &(buffer->dwt3[0]);
            for (j = 0; j < 2; j++) {
                planes[j].offset = offset;
                planes[j].virAddr = (void *)((unsigned long)pool->mem_block[i].addr + offset);
                offset += planes[j].length;
                planes[j].fd = buffer->m.fd;
            }
            planes = &(buffer->dwt4[0]);
            for (j = 0; j < 2; j++) {
                planes[j].offset = offset;
                planes[j].virAddr = (void *)((unsigned long)pool->mem_block[i].addr + offset);
                offset += planes[j].length;
                planes[j].fd = buffer->m.fd;
            }
        }
        List_Push(pool->buf_list, buffer);
    }
    pool->size = buffer_count;
    pool->buffer_size = buffer_size;
    return 0;
}

void buffer_pool_free(BUFFER_POOL *pool)
{
    uint32_t i;

    List_Clear(pool->buf_list);
    for (i = 0; i < pool->size; i++) dmabufheapFree(&pool->mem_block[i]);
}

IMAGE_BUFFER_S *buffer_pool_get_buffer(BUFFER_POOL *pool)
{
    IMAGE_BUFFER_S *buffer = NULL;

    if (!pool->buf_list)
        return NULL;
    if (List_IsEmpty(pool->buf_list))
        return NULL;
    buffer = (IMAGE_BUFFER_S *)List_Pop(pool->buf_list);
    return buffer;
}

void buffer_pool_put_buffer(BUFFER_POOL *pool, IMAGE_BUFFER_S *buffer)
{
    uint32_t i = 0;
    for (i = 0; i < pool->size; i++) {
        if (pool->buffers[i].planes[0].virAddr == buffer->planes[0].virAddr) {
            List_Push(pool->buf_list, &pool->buffers[i]);
            break;
        }
    }
}

int frameinfo_buffer_alloc(IMAGE_BUFFER_S *frameInfoBuf)
{
    int ret = 0;

    memset(frameInfoBuf, 0, sizeof(IMAGE_BUFFER_S));
    frameInfoBuf->numPlanes = 1;
    //same with: frameInfoBuf->planes[0].length = sizeof(FRAME_INFO_S) + ASR_ISP_GetFwFrameInfoSize();
    frameInfoBuf->planes[0].length = sizeof(FRAME_INFO_S) + sizeof(_isp_fw_frameinfo_t);
    frameInfoBuf->planes[0].virAddr = malloc(frameInfoBuf->planes[0].length);
    frameInfoBuf->type = BUF_ALLOC_TYPE_HEAP;
    if (NULL == frameInfoBuf->planes[0].virAddr) {
        CLOG_ERROR("malloc for frameinfo buffer failed!\n");
        return -1;
    } else {
        memset(frameInfoBuf->planes[0].virAddr, 0, frameInfoBuf->planes[0].length);
        CLOG_INFO("malloc (%zu+%zu) for frameinfo buffer!\n", sizeof(FRAME_INFO_S), sizeof(_isp_fw_frameinfo_t));
    }

    return ret;
}

int frameinfo_buffer_free(IMAGE_BUFFER_S *frameInfoBuf)
{
    int ret = 0;

    if (frameInfoBuf->planes[0].virAddr) {
        free(frameInfoBuf->planes[0].virAddr);
    }
    memset(frameInfoBuf, 0, sizeof(IMAGE_BUFFER_S));

    return ret;
}
