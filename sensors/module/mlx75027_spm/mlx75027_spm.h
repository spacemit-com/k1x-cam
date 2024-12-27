/*
 * Copyright (C) 2024 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _MLX75027_SPM_H_
#define _MLX75027_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum MLX75027_SPM_WORK_MODE {
    MLX75027_SPM_640x480_12bit_LINEAR_30_2LANE = 0,

    MLX75027_SPM_WORK_MODE_SIZE,
} MLX75027_SPM_WORK_MODE_E;

#define MLX75027_VTS_ADJUST (0) /* vts - max_exposure */

#define MLX75027_VMAX_480P30_12bit_LINEAR     (640 + MLX75027_VTS_ADJUST) //
#define MLX75027_LINETIME_480P30_12bit_LINEAR 16842  // ns, 640 / 38M = 1.6842e-5

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
