/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _BF2257CS_SPM_H_
#define _BF2257CS_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum BF2257CS_SPM_WORK_MODE {
    BF2257CS_SPM_1600x1200_10bit_LINEAR_30_1LANE = 0,

    BF2257CS_SPM_WORK_MODE_SIZE,
} BF2257CS_SPM_WORK_MODE_E;

#define BF2257CS_VTS_ADJUST (0)

#define BF2257CS_VMAX_2M30_10bit_LINEAR     (1250 + BF2257CS_VTS_ADJUST)
#define BF2257CS_LINETIME_2M30_10bit_LINEAR 26667  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
