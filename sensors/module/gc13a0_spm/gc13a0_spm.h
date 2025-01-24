/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _GC13A0_SPM_H_
#define _GC13A0_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum GC13A0_SPM_WORK_MODE {
    GC13A0_SPM_4208x3120_10bit_LINEAR_30_4LANE = 0,

    GC13A0_SPM_WORK_MODE_SIZE,
} GC13A0_SPM_WORK_MODE_E;

#define GC13A0_VTS_ADJUST (16)

#define GC13A0_VMAX_13M30_10bit_LINEAR     (3232 + GC13A0_VTS_ADJUST)//1260
#define GC13A0_LINETIME_13M30_10bit_LINEAR 10310  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
