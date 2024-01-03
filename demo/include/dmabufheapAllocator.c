/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */

#include "dmabufheapAllocator.h"

#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "BufferAllocatorWrapper.h"
#include "cam_list.h"
#include "cam_log.h"

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static char kDmabufCameraSystemHeapName[] = "system";
static char kDmabufCameraSystemUncachedHeapName[] = "system-uncached";
//static char kDmabufDmaHeapName[] = "reserved";
//static char kDmabufDmaUncachedHeapName[] = "reserved-uncached";
static char kDmabufDmaHeapName[] = "linux,cma";
static char kDmabufDmaUncachedHeapName[] = "linux,cma-uncached";
static LIST_HANDLE g_bufferList;
static BufferAllocator *gAllocator;
static uint32_t gBufferSizeHightWaterMark;
static uint32_t gBufferTotalSize;

typedef struct DMABUF_HEAP_BUFFER {
    int fd;
    int size;
    void *viraddr;
    char heap_name[24];
} DMABUF_HEAP_BUFFER_S;

static int initAllocator()
{
    int ret = 0;

    pthread_mutex_lock(&g_mutex);
    if (gAllocator != NULL) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    gAllocator = CreateDmabufHeapBufferAllocator();

    g_bufferList = List_Create(0);
    pthread_mutex_unlock(&g_mutex);

    return ret;
}

static int allocBuffer(const char *heap_name, int size, int *buffer_fd, void **viraddr)
{
    int ret;
    if (initAllocator()) {
        return -1;
    }

    ret = DmabufHeapAlloc(gAllocator, heap_name, size, 0, 0);
    if (ret < 0) {
        CLOG_ERROR("allco buffer fail");
        return ret;
    }

    void *addr = mmap(0, size, PROT_WRITE | PROT_READ, MAP_SHARED, ret, 0);
    if (addr == MAP_FAILED) {
        return -1;
    }

    pthread_mutex_lock(&g_mutex);
    DMABUF_HEAP_BUFFER_S *buffer = malloc(sizeof(*buffer));
    buffer->fd = ret;
    buffer->viraddr = addr;
    buffer->size = size;
    if (strlen(heap_name) < sizeof(buffer->heap_name) + 1)
        strncpy(buffer->heap_name, heap_name, sizeof(buffer->heap_name));
    else
	return -1;
    gBufferTotalSize += size;
    gBufferSizeHightWaterMark =
        gBufferSizeHightWaterMark > gBufferTotalSize ? gBufferSizeHightWaterMark : gBufferTotalSize;
    List_Push(g_bufferList, buffer);
    pthread_mutex_unlock(&g_mutex);

    *buffer_fd = ret;
    *viraddr = addr;
    return 0;
}

static int allocDmabufHeapSystemBuffer(int size, int cached, void **viraddr, int *fd)
{
    const char *heap_name = cached ? kDmabufCameraSystemHeapName : kDmabufCameraSystemUncachedHeapName;

    // ATRACE_BEGIN(__FUNCTION__);
    int ret = allocBuffer(heap_name, size, fd, viraddr);
    // ATRACE_END();

    return ret;
}

static int allocDmabufHeapDmaBuffer(int size, int cached, void **viraddr, int *fd)
{
    const char *heap_name = cached ? kDmabufDmaHeapName : kDmabufDmaUncachedHeapName;

    // ATRACE_BEGIN(__FUNCTION__);
    int ret = allocBuffer(heap_name, size, fd, viraddr);
    // ATRACE_END();

    return ret;
}

static bool findDmabufheapBuffer(const void *item, const void *condition)
{
    const DMABUF_HEAP_BUFFER_S *buffer = (const DMABUF_HEAP_BUFFER_S *)item;
    const int *fd = (const int *)condition;

    return (buffer->fd > 0 && buffer->fd == *fd);
}

static int freeDmabufHeapBuffer(int buffer_fd)
{
    pthread_mutex_lock(&g_mutex);
    DMABUF_HEAP_BUFFER_S *buffer =
        (DMABUF_HEAP_BUFFER_S *)List_FindItemIf(g_bufferList, findDmabufheapBuffer, &buffer_fd);
    if (buffer == 0) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }
    gBufferTotalSize -= buffer->size;
    List_EraseByItem(g_bufferList, buffer);
    pthread_mutex_unlock(&g_mutex);

    int ret = munmap(buffer->viraddr, (size_t)buffer->size);
    ret |= close(buffer->fd);
    free(buffer);

    return ret;
}

int32_t dmabufheapAlloc(BUFFER_S *buffer, uint32_t size, int continuous)
{
    int ret;
    void *virAddr;
    int fd;

    memset(buffer, 0, sizeof(*buffer));
    if (continuous) {
        ret = allocDmabufHeapDmaBuffer(size, 1, &virAddr, &fd);
    } else {
        ret = allocDmabufHeapSystemBuffer(size, 1, &virAddr, &fd);
    }
    if (ret != 0) {
        return -1;
    }
    buffer->m.fd = fd;
    buffer->addr = virAddr;

    return 0;
}

int32_t dmabufheapFree(const BUFFER_S *buffer)
{
    return freeDmabufHeapBuffer(buffer->m.fd);
}

static int syncDmabufheapBuffer(int fd, CACHE_OPS_DIR_E dir)
{
    int ret = 0;

    switch (dir) {
        case CACHE_OPS_DIR_CLEAN:
            ret = DmabufHeapCpuSyncEnd(gAllocator, fd, kSyncWrite);
            break;
        case CACHE_OPS_DIR_INVALIDATE:
            ret = DmabufHeapCpuSyncStart(gAllocator, fd, kSyncRead);
            break;
        case CACHE_OPS_DIR_CLEAN_INVALIDATE:
            ret = DmabufHeapCpuSyncEnd(gAllocator, fd, kSyncReadWrite);
            break;
        default:
            return -2;
    }

    return ret;
}

int32_t dmabufheapCacheOps(const BUFFER_S *buffer, CACHE_OPS_DIR_E dir)
{
    return syncDmabufheapBuffer(buffer->m.fd, dir);
}
