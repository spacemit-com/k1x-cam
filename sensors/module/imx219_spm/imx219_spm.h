/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _IMX219_SPM_H_
#define _IMX219_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum IMX219_SPM_WORK_MODE {
    IMX219_SPM_1920x1080_10bit_LINEAR_30_2LANE = 0,

    IMX219_SPM_WORK_MODE_SIZE,
} IMX219_SPM_WORK_MODE_E;

#define IMX219_VTS_ADJUST (4) /* vts - max_exposure*/

#define IMX219_VMAX_1080P30_10bit_LINEAR     (0x06E6 + IMX219_VTS_ADJUST)
#define IMX219_LINETIME_1080P30_10bit_LINEAR 18875  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
