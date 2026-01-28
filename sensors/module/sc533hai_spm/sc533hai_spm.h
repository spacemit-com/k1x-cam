/*
 * Copyright (C) 2022 Spacemit Micro Limited
 * All Rights Reserved.
 */
#ifndef _SC533HAI_SPM_H_
#define _SC533HAI_SPM_H_
#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum SC501AI_SPM_WORK_MODE {
    SC533HAI_SPM_2688x1616_10bit_30fps_2LANE = 0,
    SC533HAI_SPM_1920x1080_10bit_60fps_2LANE,
    SC533HAI_SPM_WORK_MODE_SIZE
} SC501AI_SPM_WORK_MODE_E;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
