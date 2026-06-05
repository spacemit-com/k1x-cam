/*
 * sc285sl_spm.h
 * SPM module header for SC285SL (structure mirrors sc533hai_spm.h)
 */
#ifndef _SC285SL_SPM_H_
#define _SC285SL_SPM_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum SC285SL_SPM_WORK_MODE {
	SC285SL_SPM_1920x1080_10bit_90fps_4LANE = 0,
	SC285SL_SPM_WORK_MODE_SIZE
} SC285SL_SPM_WORK_MODE_E;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif


