/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 *
 * SC640 thermal module register settings.
 * The module is controlled by command packets over I2C and outputs YUV over
 * 2-lane MIPI. The stream on/off packet is handled in sensor/sc640_sensor.c.
 */
#ifndef _SC640_SPM_SETTING_H_
#define _SC640_SPM_SETTING_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

struct regval_tab sc640_spm_640x512_8bit_30fps_tab[] = {
    // {0xFFFF, 0x0000},
};

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
