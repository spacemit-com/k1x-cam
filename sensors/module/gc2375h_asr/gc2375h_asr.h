/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#ifndef _GC2375H_ASR_H_
#define _GC2375H_ASR_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum GC2375H_ASR_WORK_MODE {
    GC2375H_ASR_1600x1200_10bit_LINEAR_30_1LANE = 0,

    GC2375H_ASR_WORK_MODE_SIZE,
} GC2375H_ASR_WORK_MODE_E;

#define GC2375H_VTS_ADJUST (4) /* make real fps less than stand fps because NVR require*/
#if 0 // mclk=24mhz
#define GC2375H_VMAX_2M30_10bit_LINEAR     (1256 + GC2375H_INCREASE_LINES)
#define GC2375H_LINETIME_2M30_10bit_LINEAR 26900  // ns
#else // mclk=23.63mhz
#define GC2375H_VMAX_2M30_10bit_LINEAR     (1330 + GC2375H_VTS_ADJUST)//1260
#define GC2375H_LINETIME_2M30_10bit_LINEAR 24982  // ns
#endif

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
