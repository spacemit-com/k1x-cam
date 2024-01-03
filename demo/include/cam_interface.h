/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef __CAM_INTERFACE_H__
#define __CAM_INTERFACE_H__

#include "cam_module_interface.h"

typedef struct {
    int width;
    int height;
    PIXEL_FORMAT_E format;
} IMAGE_INFO_S;

typedef enum BUF_ALLOC_TYPE {
    BUF_ALLOC_TYPE_HEAP = 0,
    BUF_ALLOC_TYPE_ION,
    BUF_ALLOC_TYPE_DMABUF_HEAP,
} BUF_ALLOC_TYPE_E;

#endif /* __CAM_INTERFACE_H__ */
