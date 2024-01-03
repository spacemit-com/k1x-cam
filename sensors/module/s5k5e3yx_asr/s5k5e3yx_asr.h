/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#ifndef _S5K5E3YX_ASR_H_
#define _S5K5E3YX_ASR_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum S5K5E3YX_ASR_WORK_MODE {
    S5K5E3YX_ASR_2560x1920_10bit_LINEAR_30_2LANE = 0,

    S5K5E3YX_ASR_WORK_MODE_SIZE,
} S5K5E3YX_ASR_WORK_MODE_E;

#define S5K5E3YX_VTS_ADJUST (8) /* vts - max_exposure*/
// MCLK = 24M
//#define S5K5E3YX_VMAX_5M30_10bit_LINEAR  (0x7e1 + S5K5E3YX_VTS_ADJUST)
//#define S5K5E3YX_LINETIME_5M30_10bit_LINEAR 16400  // ns
// MCLK = 23.63M
#define S5K5E3YX_VMAX_5M30_10bit_LINEAR       (0x7e2 + S5K5E3YX_VTS_ADJUST)
#define S5K5E3YX_LINETIME_5M30_10bit_LINEAR   16400  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
