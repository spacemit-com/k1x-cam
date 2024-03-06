/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef __SPM_COMM_TUNING_H__
#define __SPM_COMM_TUNING_H__

#include <stdbool.h>
#include <stdint.h>

#include "CPPGlobalDefine.h"
#include "spm_isp_sensor_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SETTING_ID_ISP_PREVIEW,
    SETTING_ID_ISP_VIDEO,
    SETTING_ID_CPP_PREVIEW,
    SETTING_ID_CPP_VIDEO,
    SETTING_ID_CPP_SNAPSHOT,
    SETTING_ID_CPP_NIGHTSHOT,
    SETTING_ID_NIGHTSHOT,
};

typedef struct {
    _nightmode_param_t nightmode;
} nightmode_setting_t;

typedef struct spmSENSOR_MODULE_SETTING {
    ISP_DEFAULT_SETTING_S ispPreviewSetting;
    ISP_DEFAULT_SETTING_S ispVideoSetting;
    cpp_tuning_params_t cppPreviewSetting;
    cpp_tuning_params_t cppVideoSetting;
    cpp_tuning_params_t cppSnapshotSetting;
    cpp_tuning_params_t cppNightshotSetting;
    nightmode_setting_t nightshotSetting;
} SENSOR_MODULE_SETTING_S;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* __SPM_COMM_TUNING_H__ */
