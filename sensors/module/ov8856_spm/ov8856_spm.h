/*
 * OV8856 RK 4-lane timing metadata.
 */
#ifndef _OV8856_SPM_H_
#define _OV8856_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum OV8856_SPM_WORK_MODE {
	OV8856_SPM_3264x2448_10bit_LINEAR_30_4LANE = 0,
    OV8856_SPM_1632x1224_10bit_LINEAR_30_4LANE = 1,
    OV8856_SPM_WORK_MODE_SIZE,
} OV8856_SPM_WORK_MODE_E;

#define OV8856_VTS_ADJUST (6) /* vts - max_exposure*/
/* RK 4-lane modes: HTS 0x078c, VTS 0x09b2, 143.85672 MHz PCLK. */
#define OV8856_VMAX_2K30_10bit_LINEAR       0x09b2
#define OV8856_LINETIME_2K30_10bit_LINEAR   13430  // ns
#define OV8856_VMAX_1K30_10bit_LINEAR       0x09b2
#define OV8856_LINETIME_1K30_10bit_LINEAR   13430  // ns
#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
