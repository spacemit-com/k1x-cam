/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _IMX135_SPM_H_
#define _IMX135_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum IMX135_SPM_WORK_MODE {
    IMX135_SPM_4208x3120_10bit_LINEAR_30_4LANE = 0,
    IMX135_SPM_2104x1560_10bit_LINEAR_30_4LANE,
    IMX135_SPM_2104x1184_10bit_LINEAR_30_4LANE,

    IMX135_SPM_WORK_MODE_SIZE,
    IMX135_SPM_4208x2416_10bit_LINEAR_30_4LANE,
    IMX135_SPM_1920x1080_10bit_LINEAR_30_4LANE,
} IMX135_SPM_WORK_MODE_E;

#define IMX135_VTS_ADJUST (4) /* vts - max_exposure*/

#if 0 //mclk = 24M
#define IMX135_VMAX_13M30_10bit_LINEAR       (0xc48 + IMX135_VTS_ADJUST)//0xc4c
#define IMX135_LINETIME_13M30_10bit_LINEAR   10600  // ns
#else //mclk = 23.63M
#define IMX135_VMAX_13M30_10bit_LINEAR       (3246 + IMX135_VTS_ADJUST)//0xc4c
#define IMX135_LINETIME_13M30_10bit_LINEAR   10328  // ns
#endif
#define IMX135_VMAX_10M30_10bit_LINEAR       (0xc48 + IMX135_VTS_ADJUST)
#define IMX135_LINETIME_10M30_10bit_LINEAR   10600  // ns
#define IMX135_VMAX_3M30_10bit_LINEAR        (0x64c + IMX135_VTS_ADJUST)//0x650
#define IMX135_LINETIME_3M30_10bit_LINEAR    20600  // ns
#define IMX135_VMAX_2500K30_10bit_LINEAR     (0x64c + IMX135_VTS_ADJUST)
#define IMX135_LINETIME_2500K30_10bit_LINEAR 20600  // ns
#define IMX135_VMAX_1080P30_10bit_LINEAR     (0x64c + IMX135_VTS_ADJUST)
#define IMX135_LINETIME_1080P30_10bit_LINEAR 20600  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
