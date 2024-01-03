/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _CAM_FLASH_H_
#define _CAM_FLASH_H_

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef struct FLASH_CONTEXT {
    const char* name;
    int32_t devId;
    uint8_t twsi_no;
} FLASH_CONTEXT_S;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_FLASH_H_ */
