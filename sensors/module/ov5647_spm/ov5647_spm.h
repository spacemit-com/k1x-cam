/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OV5647_SPM_H_
#define _OV5647_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV5647_SPM_WORK_MODE {
    OV5647_SPM_2592x1936_10bit_LINEAR_30_4LANE = 0,
    OV5647_SPM_WORK_MODE_SIZE,
} OV5647_SPM_WORK_MODE_E;

#define OV5647_VTS_ADJUST (4) /* vts - max_exposure*/
// MCLK = ?M
#define OV5647_VMAX_2K30_10bit_LINEAR       (1974 + OV5647_VTS_ADJUST)
#define OV5647_LINETIME_2K30_10bit_LINEAR   33772  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
