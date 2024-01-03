/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#ifndef _OV13B10_ASR_H_
#define _OV13B10_ASR_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV13B10_ASR_WORK_MODE {
    OV13B10_ASR_4208x3120_10bit_LINEAR_30_4LANE = 0,
    OV13B10_ASR_4208x2416_10bit_LINEAR_30_4LANE,
    OV13B10_ASR_2104x1560_10bit_LINEAR_30_4LANE,
    OV13B10_ASR_1920x1080_10bit_LINEAR_30_2LANE,
    OV13B10_ASR_WORK_MODE_SIZE,
    OV13B10_ASR_1600x1200_10bit_LINEAR_30_4LANE,  //not used
} OV13B10_ASR_WORK_MODE_E;

#define OV13B10_VTS_ADJUST (8) /* vts - max_exposure*/
// MCLK = 24M
//#define OV13B10_VMAX_13M30_10bit_LINEAR  (0xc7c + OV13B10_VTS_ADJUST)
//#define OV13B10_LINETIME_13M30_10bit_LINEAR 10500  // ns
// MCLK = 23.63M
#define OV13B10_VMAX_13M30_10bit_LINEAR       (0xd8c + OV13B10_VTS_ADJUST)
#define OV13B10_LINETIME_13M30_10bit_LINEAR   9613  // ns
#define OV13B10_VMAX_10M30_10bit_LINEAR       (0xa78 + OV13B10_VTS_ADJUST)
#define OV13B10_LINETIME_10M30_10bit_LINEAR   12442  // ns
#define OV13B10_VMAX_3M30_10bit_LINEAR        (0xbbc + OV13B10_VTS_ADJUST)
#define OV13B10_LINETIME_3M30_10bit_LINEAR    11100  // ns
#define OV13B10_VMAX_1080P30_10bit_LINEAR     (0x4b0 + OV13B10_VTS_ADJUST)
#define OV13B10_LINETIME_1080P30_10bit_LINEAR 27785  // ns
#define OV13B10_VMAX_2MP30_10bit_LINEAR     (0xbbc + OV13B10_VTS_ADJUST)
#define OV13B10_LINETIME_2MP30_10bit_LINEAR 11100  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
