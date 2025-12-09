/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _GC08A8_SPM_H_
#define _GC08A8_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum GC08A8_SPM_WORK_MODE {
    GC08A8_SPM_3264x2448_10bit_LINEAR_30_2LANE = 0,
    GC08A8_SPM_3264x2448_10bit_LINEAR_30_4LANE,

    GC08A8_SPM_WORK_MODE_SIZE,
} GC08A8_SPM_WORK_MODE_E;

#define GC08A8_VTS_ADJUST (16) /* The minimum exposure of the sensor */

#define GC08A8_VMAX_8M30_10bit_LINEAR     (2548 + GC08A8_VTS_ADJUST)//1260
#define GC08A8_LINETIME_8M30_10bit_LINEAR 13000  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
