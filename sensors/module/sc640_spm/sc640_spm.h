/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _SC640_SPM_H_
#define _SC640_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum SC640_SPM_WORK_MODE {
    SC640_SPM_640x512_8bit_LINEAR_60_2LANE = 0,
    SC640_SPM_640x512_8bit_LINEAR_30_2LANE,

    SC640_SPM_WORK_MODE_SIZE,
} SC640_SPM_WORK_MODE_E;

#define SC640_VTS_ADJUST (0)
#define SC640_VMAX_512P60_8bit_LINEAR (512)
#define SC640_LINETIME_512P60_8bit_LINEAR 65104
#define SC640_VMAX_512P30_8bit_LINEAR (512)
#define SC640_LINETIME_512P30_8bit_LINEAR 65104

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
