/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _HIK_IR_SPM_H_
#define _HIK_IR_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum HIK_IR_SPM_WORK_MODE {
    HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE = 0,

    HIK_IR_SPM_WORK_MODE_SIZE,
} HIK_IR_SPM_WORK_MODE_E;

#define HIK_IR_VMAX_512P50_8bit_LINEAR (512)
#define HIK_IR_LINETIME_512P50_8bit_LINEAR 39062

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
