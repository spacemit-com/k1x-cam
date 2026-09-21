/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _OV8858_SPM_H_
#define _OV8858_SPM_H_

#include "../../cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV8858_SPM_WORK_MODE {
    OV8858_SPM_R2A_3264x2448_10bit_LINEAR_15_2LANE = 0,
    OV8858_SPM_R2A_1632x1224_10bit_LINEAR_30_2LANE,
    OV8858_SPM_R2A_3264x2448_10bit_LINEAR_30_4LANE,
    OV8858_SPM_WORK_MODE_SIZE,
} OV8858_SPM_WORK_MODE_E;

#define OV8858_VTS_ADJUST (4)

/*
 * Line time = HTS / pclk, taken from the module vendor's calibration
 * (DeviceInfo_OV8858R2A.csv shipped in the OV8858-R2A MT6735/MT6580 setting packages):
 *   capture 4-lane 3264x2448: HTS 1940 (0x0794), pclk 148.8 MHz -> 13039 ns
 *   preview 2-lane 1632x1224: HTS 1928 (0x0788), pclk  74.4 MHz -> 25915 ns  (= u4PreviewExposureLineUnit)
 *   2-lane  3264x2448:        HTS 1940 (0x0794), pclk  74.4 MHz -> 26075 ns
 * The previous values (13398 / 26780 / 26795 ns) were 2.76% high, which made the
 * AE convert exposure time to lines with the wrong scale and miss the 50 Hz
 * anti-flicker grid.
 */
#define OV8858_LINETIME_R2A_3264x2448_2LANE 26075  /* ns */
#define OV8858_LINETIME_R2A_1632x1224_2LANE 25915  /* ns */
#define OV8858_LINETIME_R2A_3264x2448_4LANE 13039  /* ns */

/*
 * VMAX is the frame-length register value. The AE already reserves the four-line
 * margin itself (expotime_update() uses vts - OV8858_VTS_ADJUST), so adding
 * OV8858_VTS_ADJUST here double-counted it and pushed the frame length 8 lines
 * past the vendor value (2564 vs 2556).
 */
#define OV8858_VMAX_R2A_3264x2448_2LANE 0x09aa
#define OV8858_VMAX_R2A_1632x1224_2LANE 0x04dc
#define OV8858_VMAX_R2A_3264x2448_4LANE 0x0a00

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
