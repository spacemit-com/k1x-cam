/*
 * Copyright (C) 2025 Spacemit Micro Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sc533hai_spm.h"
#include "sc533hai_spm_setting.h"

static const uint8_t module_i2c_addr = 0x30;  // 7-bit i2c address
struct regval_tab sc533hai_spm_vendor_id[] = {
    {0x3107, 0xCE},
    {0x3108, 0x7C},
};

static SENSOR_MODULE_SETTING_S sc533hai_spm_setting = {
    .ispPreviewSetting =
        {
#include "sc533hai_spm_rear_primary_isp_setting.h"
        },
    .cppPreviewSetting =
        {
#include "sc533hai_spm_rear_primary_cpp_preview_setting.h"
        },
};


const CapabilityElement sc533haiBackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.46"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};


static int sc533hai_spm_get_sensor_cap_size(int32_t* capArraySize) {
    SENSORS_CHECK_PARA_POINTER(capArraySize);
    *capArraySize = SC533HAI_SPM_WORK_MODE_SIZE;
    return 0;
}

static int sc533hai_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability) {
    int i;
    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);

    // 校验模式数量
    if (capArraySize != SC533HAI_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("capArraySize(%d) != work_mode_size(%d)", capArraySize, SC533HAI_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = SC533HAI_SPM_WORK_MODE_SIZE;
    for (i = 0; i < SC533HAI_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case SC533HAI_SPM_2688x1616_10bit_30fps_2LANE: {
                sensor_capability->snr_config[i].width = 2688;
                sensor_capability->snr_config[i].height = 1616;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = SC533HAI_SPM_2688x1616_10bit_30fps_2LANE;
                sensor_capability->snr_config[i].setting = &sc533hai_spm_setting;
            } break;

            case SC533HAI_SPM_1920x1080_10bit_60fps_2LANE: {
                sensor_capability->snr_config[i].width = 1920;
                sensor_capability->snr_config[i].height = 1080;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 60;
                sensor_capability->snr_config[i].minFps = 30;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = SC533HAI_SPM_1920x1080_10bit_60fps_2LANE;
                sensor_capability->snr_config[i].setting = &sc533hai_spm_setting;
            } break;

            default:
                CLOG_ERROR("invalid work_mode(%d), max=%d", i, SC533HAI_SPM_WORK_MODE_SIZE);
                break;
        }
    }
    return 0;
}


static int sc533hai_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info) {
    SENSORS_CHECK_PARA_POINTER(snr_info);

    snr_info->image_mode = SENSOR_LINEAR_MODE;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &sc533hai_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &sc533hai_spm_setting.ispVideoSetting;

    switch (work_mode) {
        case SC533HAI_SPM_2688x1616_10bit_30fps_2LANE: {
            snr_info->linetime = 19048;
            snr_info->vts = 1750;
            snr_info->f32maxFps = 30;
            // exp_time:(VTS - VTS_ADJUST) * linetime / 1000 → (1750-8)*19048/1000 ≈ 33177us
            snr_info->exp_time[0] = (1750 - 8) * 19048 / 1000; 
            snr_info->again[0] = 1 * 0x100;
            snr_info->dgain[0] = 1 * 0x1000;
            snr_info->setting_table = sc533hai_spm_2688x1616_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(sc533hai_spm_2688x1616_10bit_30fps_tab);
            snr_info->mipi_clock = 864;
        } break;

        case SC533HAI_SPM_1920x1080_10bit_60fps_2LANE: {
            snr_info->linetime = 11111;
            snr_info->vts = 1500;
            snr_info->f32maxFps = 60;
            // exp_time:(VTS - VTS_ADJUST) * linetime / 1000 → (1500-8)*11111/1000 ≈ 16549us
            snr_info->exp_time[0] = (1500 - 8) * 11111 / 1000;
            snr_info->again[0] = 1 * 0x100;
            snr_info->dgain[0] = 1 * 0x1000;
            snr_info->setting_table = sc533hai_spm_1920x1080_10bit_60fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(sc533hai_spm_1920x1080_10bit_60fps_tab);
            snr_info->mipi_clock = 945;
        } break;

        default:
            CLOG_ERROR("invalid work_mode(%d), max=%d", work_mode, SC533HAI_SPM_WORK_MODE_SIZE);
            return -EINVAL;
    }

    snr_info->work_mode = work_mode;
    return 0;
}

static int sc533hai_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info) {
    switch (devId) {
        case 0:
            dev_info->deviceConfig = sc533haiBackCapCfg;
            dev_info->deviceConfigCnt = sizeof(sc533haiBackCapCfg) / sizeof(sc533haiBackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("invalid devId(%d), only support devId=0", devId);
            break;
    }
    return 0;
}

static int sc533hai_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id) {
    SENSORS_CHECK_PARA_POINTER(vendor_id);
    vendor_id->id_table = sc533hai_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(sc533hai_spm_vendor_id);
    return 0;
}

static int sc533hai_spm_get_sensor_i2c_addr(uint8_t* i2c_addr) {
    SENSORS_CHECK_PARA_POINTER(i2c_addr);
    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S sc533hai_spm_Obj = {
    .name = "sc533hai_spm",
    .pfnGetsnrCapSize = sc533hai_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = sc533hai_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = sc533hai_spm_get_sensor_work_info,
    .pfnGetDevInfo = sc533hai_spm_get_device_info,
    .pfnGetSnrVendorId = sc533hai_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = sc533hai_spm_get_sensor_i2c_addr,
};