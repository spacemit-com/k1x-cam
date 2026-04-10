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
	OV8856_SPM_3280x2464_10bit_LINEAR_15_4LANE = 0,
    OV8856_SPM_1640x1232_10bit_LINEAR_30_4LANE = 1,
    OV8856_SPM_WORK_MODE_SIZE,
} OV8856_SPM_WORK_MODE_E;

#define OV8856_VTS_ADJUST (6) /* vts - max_exposure*/
// MCLK = ?M
#define OV8856_VMAX_2K15_10bit_LINEAR       (2488 + OV8856_VTS_ADJUST)
#define OV8856_LINETIME_2K15_10bit_LINEAR   26795  // ns
#define OV8856_VMAX_1K30_10bit_LINEAR       (1256 + OV8856_VTS_ADJUST)
#define OV8856_LINETIME_1K30_10bit_LINEAR   26539  // ns
#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
