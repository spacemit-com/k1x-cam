/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OV2735_SPM_H_
#define _OV2735_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV2735_SPM_WORK_MODE {
    OV2735_SPM_1920x1080_10bit_LINEAR_30_1LANE = 0,

    OV2735_SPM_WORK_MODE_SIZE,
} OV2735_SPM_WORK_MODE_E;

#define OV2735_VTS_ADJUST (4) /* vts - max_exposure*/

#define OV2735_VMAX_13M30_10bit_LINEAR       (1664 + OV2735_VTS_ADJUST)
#define OV2735_LINETIME_13M30_10bit_LINEAR   20032  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
