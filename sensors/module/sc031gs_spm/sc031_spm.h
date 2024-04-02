/*
 * Copyright (C) 2022 Spacemit Micro Limited
 * All Rights Reserved.
 */
#ifndef _SC031_SPM_H_
#define _SC031_SPM_H_
#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum SC031_SPM_WORK_MODE {
    SC031_SPM_640x480_10bit_LINEAR_30_4LANE = 0,
    SC031_SPM_WORK_MODE_SIZE,
} SC031_SPM_WORK_MODE_E;

#define SC031_VTS_ADJUST (4) /* vts - max_exposure*/
#define SC031_VMAX_13M30_10bit_LINEAR       (3246 + SC031_VTS_ADJUST)//0xc4c
#define SC031_LINETIME_13M30_10bit_LINEAR   10328  // ns
#if 0 //mclk = 24M
#define SC031_VMAX_13M30_10bit_LINEAR       (0xc48 + SC031_VTS_ADJUST)//0xc4c
#define SC031_LINETIME_13M30_10bit_LINEAR   10600  // ns
#else //mclk = 23.63M
#define SC031_VMAX_13M30_10bit_LINEAR       (3246 + SC031_VTS_ADJUST)//0xc4c
#define SC031_LINETIME_13M30_10bit_LINEAR   10328  // ns
#endif
#define SC031_VMAX_10M30_10bit_LINEAR       (0xc48 + SC031_VTS_ADJUST)
#define SC031_LINETIME_10M30_10bit_LINEAR   10600  // ns
#define SC031_VMAX_3M30_10bit_LINEAR        (0x64c + SC031_VTS_ADJUST)//0x650
#define SC031_LINETIME_3M30_10bit_LINEAR    20600  // ns
#define SC031_VMAX_2500K30_10bit_LINEAR     (0x64c + SC031_VTS_ADJUST)
#define SC031_LINETIME_2500K30_10bit_LINEAR 20600  // ns
#define SC031_VMAX_1080P30_10bit_LINEAR     (0x64c + SC031_VTS_ADJUST)
#define SC031_LINETIME_1080P30_10bit_LINEAR 20600  // ns

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
