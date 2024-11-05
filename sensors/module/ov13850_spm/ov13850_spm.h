/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OV13850_SPM_H_
#define _OV13850_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV13850_SPM_WORK_MODE {
    OV13850_SPM_2112x1568_10bit_LINEAR_30_2LANE = 0,

    OV13850_SPM_WORK_MODE_SIZE,
} OV13850_SPM_WORK_MODE_E;

#define OV13850_VTS_ADJUST (4) /* vts - max_exposure*/

#define OV13850_VMAX_13M30_10bit_LINEAR       (1664 + OV13850_VTS_ADJUST)
#define OV13850_LINETIME_13M30_10bit_LINEAR   20032  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
