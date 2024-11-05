/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OV13855_SPM_H_
#define _OV13855_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV13855_SPM_WORK_MODE {
    OV13855_SPM_4224x3136_10bit_LINEAR_30_4LANE = 0,

    OV13855_SPM_WORK_MODE_SIZE,
} OV13855_SPM_WORK_MODE_E;

#define OV13855_VTS_ADJUST (8) /* vts - max_exposure*/

#define OV13855_VMAX_13M30_10bit_LINEAR       (3214 + OV13855_VTS_ADJUST)
#define OV13855_LINETIME_13M30_10bit_LINEAR   10371  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
