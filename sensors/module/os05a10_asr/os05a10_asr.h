/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#ifndef _OS05A10_ASR_H_
#define _OS05A10_ASR_H_

#include "cam_sensors_module.h"
#include "os05a10_asr_setting.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OS05A10_ASR_WORK_MODE {
    OS05A10_ASR_1920x1080_10bit_LINEAR_30_1LANE = 0,
    OS05A10_ASR_1920x1080_10bit_HDR_30_1LANE,

    OS05A10_ASR_WORK_MODE_SIZE,
} OS05A10_ASR_WORK_MODE_E;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
