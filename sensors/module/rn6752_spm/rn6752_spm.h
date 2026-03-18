/*
 * Copyright (C) 2024
 * All Rights Reserved.
 *
 * RN6752V1 video decoder module header
 */
#ifndef _RN6752_SPM_H_
#define _RN6752_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum RN6752_SPM_WORK_MODE {
    RN6752_SPM_1920x1080_8bit_LINEAR_25_4LANE = 0,
    RN6752_SPM_1280x720_8bit_LINEAR_25_4LANE,
    RN6752_SPM_1280x720_8bit_LINEAR_30_4LANE,

    RN6752_SPM_WORK_MODE_SIZE,
} RN6752_SPM_WORK_MODE_E;

/* RN6752 is a video decoder, no VTS adjust needed for AE */
#define RN6752_VTS_ADJUST (0)

#define RN6752_VMAX_1080P25_8bit_LINEAR   (1125)
#define RN6752_LINETIME_1080P25_8bit_LINEAR   35556  /* ns, 1/25/1125 */

#define RN6752_VMAX_720P25_8bit_LINEAR    (750)
#define RN6752_LINETIME_720P25_8bit_LINEAR    53333  /* ns, 1/25/750 */

#define RN6752_VMAX_720P30_8bit_LINEAR    (750)
#define RN6752_LINETIME_720P30_8bit_LINEAR    44444  /* ns, 1/30/750 */

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
