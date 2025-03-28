/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _MAX96716_SPM_H_
#define _MAX96716_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum MAX96716_SPM_WORK_MODE {
    MAX96716_SPM_3864x2192_10bit_LINEAR_30_4LANE = 0,
    MAX96716_SPM_WORK_MODE_SIZE,
} MAX96716_SPM_WORK_MODE_E;

#define MAX96716_VTS_ADJUST (0) /* vts - max_exposure*/

#define MAX96716_VMAX_8M30F_10bit_LINEAR     (0x08CA + MAX96716_VTS_ADJUST)
#define MAX96716_LINETIME_8M30F_10bit_LINEAR 14815  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
