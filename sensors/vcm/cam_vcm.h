/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _CAM_VCM_H_
#define _CAM_VCM_H_

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef struct VCM_CONTEXT {
    const char* name;
    int32_t devId;
    uint32_t magic;
    uint8_t i2c_addr;
    uint8_t twsi_no;
    uint32_t current_position;
} VCM_CONTEXT_S;

#define VCM_CHECK_HANDLE_IS_ERR(ptr)                                                       \
    do {                                                                                   \
        if ((ptr)->magic != VCM_MAGIC) {                                                   \
            CLOG_ERROR("%s: vcm context magic 0x%08x is err", __FUNCTION__, (ptr)->magic); \
            return -ENXIO;                                                                 \
        }                                                                                  \
    } while (0)

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_VCM_H_ */
