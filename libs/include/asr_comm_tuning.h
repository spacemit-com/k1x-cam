/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#ifndef __ASR_COMM_TUNING_H__
#define __ASR_COMM_TUNING_H__

#include <stdbool.h>
#include <stdint.h>

#include "CPPGlobalDefine.h"
#include "asr_isp_sensor_comm.h"

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

typedef struct asrSENSOR_MODULE_SETTING {
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

#endif /* __ASR_COMM_TUNING_H__ */
