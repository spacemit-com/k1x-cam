/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _ASR_COMM_SENSORS_H_
#define _ASR_COMM_SENSORS_H_

#include <stdbool.h>
#include <stdint.h>

#include "asr_comm_cam.h"
#include "asr_comm_tuning.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum {
    SENSOR_LINEAR_MODE,
    SENSOR_HDR_MODE,
} SENSOR_IMAGE_MODE_E;

#define MAX_EXP_CHAN_PER_SENSOR 2
#define LSC_PROFILE_DIMENSION 3
#define OTP_LSC_PROFILE_LEN 576

typedef struct asrSENSOR_INIT_ATTR_S {
    int32_t initSceneLuma[MAX_EXP_CHAN_PER_SENSOR];
    int32_t initSceneLux[MAX_EXP_CHAN_PER_SENSOR];  // Q8 format
    int32_t initCorrelationCT[MAX_EXP_CHAN_PER_SENSOR];
    int32_t initTint[MAX_EXP_CHAN_PER_SENSOR];
    int32_t SampleRgain[MAX_EXP_CHAN_PER_SENSOR];
    int32_t SampleBgain[MAX_EXP_CHAN_PER_SENSOR];
} SENSOR_INIT_ATTR_S;

typedef struct asrSENSOR_CONFIG {
    int32_t width;
    int32_t height;
    int32_t bitDepth;
    float maxFps;
    float minFps;
    SENSOR_IMAGE_MODE_E image_mode;
    int32_t lane_num;
    ISP_BAYER_PATTERN_E pattern;
    int32_t supportPDAF;
    int32_t work_mode;
    const SENSOR_MODULE_SETTING_S *setting;
} SENSOR_CONFIG_S;

typedef struct asrSENSOR_CAPABILITY {
    SENSOR_CONFIG_S *snr_config;
    int32_t snr_config_num;
} SENSOR_CAPABILITY_S;

typedef struct asrSENSOR_OTP_DATA {
    int data_valid;
    int lsc_profile[OTP_LSC_PROFILE_LEN];
    int lsc_merged_profile[LSC_PROFILE_DIMENSION][OTP_LSC_PROFILE_LEN];
    int awb_goldenRG;
    int awb_goldenBG;
    int awb_currentRG;
    int awb_currentBG;
} SENSOR_OTP_DATA_S;

typedef struct asrSENSORS_MODULE_CAPABILITY {
    SENSOR_CAPABILITY_S sensor_capability;
} SENSORS_MODULE_CAPABILITY_S;

typedef struct asrSENSORS_MODULE_INFO {
    int32_t snr_config_num;
} SENSORS_MODULE_INFO_S;

#ifdef __cplusplus
}
#endif /* extern "C" */
#endif
