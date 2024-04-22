/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OV8856_SPM_H_
#define _OV8856_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV8856_SPM_WORK_MODE {
    OV8856_SPM_2560x1440_10bit_LINEAR_30_4LANE = 0,
    OV8856_SPM_WORK_MODE_SIZE,
} OV8856_SPM_WORK_MODE_E;

#define OV8856_VTS_ADJUST (6) /* vts - max_exposure*/
// MCLK = ?M
#define OV8856_VMAX_2K30_10bit_LINEAR       (2482 + OV8856_VTS_ADJUST)
#define OV8856_LINETIME_2K30_10bit_LINEAR   13430  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
