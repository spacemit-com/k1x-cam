/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _GC5035_SPM_H_
#define _GC5035_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum GC5035_SPM_WORK_MODE {
    GC5035_SPM_2592x1944_10bit_LINEAR_30_2LANE = 0,

    GC5035_SPM_WORK_MODE_SIZE,
} GC5035_SPM_WORK_MODE_E;

#define GC5035_VTS_ADJUST (4) /* make real fps less than stand fps because NVR require*/
#if 0                         // mclk=24mhz
#define GC5035_VMAX_8M30_10bit_LINEAR     (0x7d8 + GC5035_INCREASE_LINES)
#define GC5035_LINETIME_8M30_10bit_LINEAR 16700  // ns
#else                                            // mclk=23.63mhz
#define GC5035_VMAX_8M30_10bit_LINEAR     (0x86e + GC5035_VTS_ADJUST)
#define GC5035_LINETIME_8M30_10bit_LINEAR 15400  // ns
#endif

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
