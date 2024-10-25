/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _IMX415_SPM_H_
#define _IMX415_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum IMX415_SPM_WORK_MODE {
    IMX415_SPM_3864x2192_10bit_LINEAR_30_4LANE = 0,

    IMX415_SPM_3864x2192_12bit_LINEAR_30_4LANE,
    IMX415_SPM_WORK_MODE_SIZE,
} IMX415_SPM_WORK_MODE_E;

#define IMX415_VTS_ADJUST (0) /* vts - max_exposure*/

#define IMX415_VMAX_8M30F_12bit_LINEAR     (0x08CA + IMX415_VTS_ADJUST)
#define IMX415_LINETIME_8M30F_12bit_LINEAR 14815  // ns

#define IMX415_VMAX_8M30F_10bit_LINEAR     (0x0A8C + IMX415_VTS_ADJUST)
#define IMX415_LINETIME_8M30F_10bit_LINEAR 12346  // ns
#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
