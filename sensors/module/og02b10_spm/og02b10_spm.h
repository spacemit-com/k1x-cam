/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OG02B10_SPM_H_
#define _OG02B10_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OG02B10_SPM_WORK_MODE {
    OG02B10_SPM_1600x1300_10bit_LINEAR_60_2LANE = 0,
    OG02B10_SPM_WORK_MODE_SIZE,
} OG02B10_SPM_WORK_MODE_E;

#define OG02B10_VTS_ADJUST (12) /* vts - max_exposure*/
// MCLK = 24M
#define OG02B10_VMAX_1300P60_10bit_LINEAR       (1416 + OG02B10_VTS_ADJUST)
// #define OG02B10_LINETIME_1300P60_10bit_LINEAR 11700 // ns, 1600 / 24M = 6.6666e-5
 #define OG02B10_LINETIME_1300P60_10bit_LINEAR 11700 // ns, 1600 / 24M = 6.6666e-5

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
