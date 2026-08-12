/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _SC485SL_SPM_H_
#define _SC485SL_SPM_H_

#include "cam_sensors_module.h"

typedef enum {
    SC485SL_SPM_2688X1520_10BIT_90FPS_4LANE = 0,
    SC485SL_SPM_2688X1520_12BIT_90FPS_4LANE,
    SC485SL_SPM_WORK_MODE_SIZE,
} SC485SL_SPM_WORK_MODE_E;

#define SC485SL_VTS_2688X1520_90FPS 1600U
#define SC485SL_LINETIME_2688X1520_90FPS 6944U

#endif
