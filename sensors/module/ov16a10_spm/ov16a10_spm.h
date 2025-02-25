/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OV16A10_SPM_H_
#define _OV16A10_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV16A10_SPM_WORK_MODE {
    OV16A10_SPM_3840x2160_10bit_LINEAR_30_4LANE = 0,
    OV16A10_SPM_WORK_MODE_SIZE,
} OV16A10_SPM_WORK_MODE_E;

#define OV16A10_VTS_ADJUST (8) /* vts - max_exposure*/
// MCLK = ?M
#define OV16A10_VMAX_8M30_10bit_LINEAR       (0xA04 + OV16A10_VTS_ADJUST)
#define OV16A10_LINETIME_8M30_10bit_LINEAR   13000  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
