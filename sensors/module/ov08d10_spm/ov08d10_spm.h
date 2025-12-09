/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OV08D10_SPM_H_
#define _OV08D10_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV08D10_SPM_WORK_MODE {
    OV08D10_SPM_3264x2448_10bit_LINEAR_30_2LANE = 0,
    // OV08D10_SPM_1632x1224_10bit_LINEAR_30_2LANE,
    OV08D10_SPM_1920x1080_10bit_LINEAR_30_2LANE,
    OV08D10_SPM_WORK_MODE_SIZE,
    OV08D10_SPM_1632x1224_10bit_LINEAR_30_2LANE,  // not used
    OV08D10_SPM_640x480_10bit_LINEAR_30_1LANE,  //not used
} OV08D10_SPM_WORK_MODE_E;

#define OV08D10_VTS_ADJUST                 (20) /* make real fps less than stand fps because NVR require*/
#define OV08D10_VMAX_8M30_10bit_LINEAR     (2608 + OV08D10_VTS_ADJUST)
#define OV08D10_LINETIME_8M30_10bit_LINEAR 12800  // ns
#if 0                                             // MCLK = 24M
#define OV08D10_VMAX_2M30_10bit_LINEAR        (2508 + OV08D10_VTS_ADJUST)
#define OV08D10_LINETIME_2M30_10bit_LINEAR    13278  // ns
#define OV08D10_VMAX_1080P30_10bit_LINEAR     (2324 + OV08D10_VTS_ADJUST)
#define OV08D10_LINETIME_1080P30_10bit_LINEAR 14330  // ns
#else                                                // MCLK = 23.63M
#define OV08D10_VMAX_2M30_10bit_LINEAR        (2470 + OV08D10_VTS_ADJUST)
#define OV08D10_LINETIME_2M30_10bit_LINEAR    13486  // ns
#define OV08D10_VMAX_1080P30_10bit_LINEAR     (2290 + OV08D10_VTS_ADJUST)
#define OV08D10_LINETIME_1080P30_10bit_LINEAR 14554  // ns
#define OV08D10_VMAX_VGA30_10bit_LINEAR        (1252 + OV08D10_VTS_ADJUST)
#define OV08D10_LINETIME_VGA30_10bit_LINEAR    26556  // ns

#endif

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
