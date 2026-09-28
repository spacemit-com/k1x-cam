/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _SC485SL_SPM_H_
#define _SC485SL_SPM_H_

#include "cam_sensors_module.h"

typedef enum {
    SC485SL_SPM_1344X760_12BIT_180FPS_4LANE = 0,
    SC485SL_SPM_1920X1080_12BIT_120FPS_4LANE,
    SC485SL_SPM_1280X720_12BIT_180FPS_4LANE,
    SC485SL_SPM_2688X1520_10BIT_90FPS_4LANE,
    SC485SL_SPM_2688X1520_12BIT_90FPS_4LANE,
    SC485SL_SPM_WORK_MODE_SIZE,
} SC485SL_SPM_WORK_MODE_E;

/* 客户要求 180fps 常态输出：VTS=800（fps=360M/(2500×800)=180），曝光上限=帧长
 * 5.5ms（sensor 驱动模式 0 分支钳位），暗光靠增益兜底。ISP SDK 的 AE 固定按
 * 30fps 规划曝光（GetExpoTimeByFps(30)），若放开 VTS 拉伸会因帧率模型不一致
 * 产生违约瞬态导致 AE 失稳，故本模式必须维持钳位（2026-09-28 实测）。 */
#define SC485SL_VTS_1344X760_180FPS 800
#define SC485SL_VTS_1920X1080_120FPS 1200
#define SC485SL_VTS_1280X720_180FPS 800
#define SC485SL_VTS_2688X1520_90FPS 1600
#define SC485SL_LINETIME 6944

#endif
