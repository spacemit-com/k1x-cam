/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include "s5k5e3yx_spm.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "s5k5e3yx_spm_setting.h"

static const uint8_t module_i2c_addr = 0x10; /* I2C Address 7-bit*/  // TST=1: 0x2d

struct regval_tab s5k5e3yx_spm_vendor_id[] = {
    {0x0000, 0x5E},
    {0x0001, 0x30},
};

static SENSOR_MODULE_SETTING_S s5k5e3yx_spm_setting = {
    .ispPreviewSetting =
        {
#include "s5k5e3yx_spm_front_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "s5k5e3yx_spm_front_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "s5k5e3yx_spm_front_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "s5k5e3yx_spm_front_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "s5k5e3yx_spm_front_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "s5k5e3yx_spm_front_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "s5k5e3yx_spm_front_nightshot_setting.h"
        },
};

const CapabilityElement s5k5e3yxBackCapCfg[] = {
    {"capSensorPhysicalSize", "2.9, 2.18"},
    {"capLensFocalLength", "2.31"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", ""},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int s5k5e3yx_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = S5K5E3YX_SPM_WORK_MODE_SIZE;
    return 0;
}

static int s5k5e3yx_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != S5K5E3YX_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, S5K5E3YX_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = S5K5E3YX_SPM_WORK_MODE_SIZE;
    for (i = 0; i < S5K5E3YX_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case S5K5E3YX_SPM_2560x1920_10bit_LINEAR_30_2LANE: {
                sensor_capability->snr_config[i].width = 2560;
                sensor_capability->snr_config[i].height = 1920;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_GBRG;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = S5K5E3YX_SPM_2560x1920_10bit_LINEAR_30_2LANE;
                sensor_capability->snr_config[i].setting = &s5k5e3yx_spm_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           S5K5E3YX_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int s5k5e3yx_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = module_i2c_addr;
    // snr_info->id_table = s5k5e3yx_spm_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(s5k5e3yx_spm_vendor_id);

    switch (work_mode) {
        case S5K5E3YX_SPM_2560x1920_10bit_LINEAR_30_2LANE: {
            snr_info->linetime = S5K5E3YX_LINETIME_5M30_10bit_LINEAR;  // ns
            snr_info->vts = S5K5E3YX_VMAX_5M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = (S5K5E3YX_VMAX_5M30_10bit_LINEAR - S5K5E3YX_VTS_ADJUST) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = s5k5e3yx_spm_2560x1920_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(s5k5e3yx_spm_2560x1920_10bit_30fps_tab);
            snr_info->mipi_clock = 800;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       S5K5E3YX_SPM_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &s5k5e3yx_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &s5k5e3yx_spm_setting.ispVideoSetting;

    return 0;
}

static int s5k5e3yx_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 2:  // front
            dev_info->deviceConfig = s5k5e3yxBackCapCfg;
            dev_info->deviceConfigCnt = sizeof(s5k5e3yxBackCapCfg) / sizeof(s5k5e3yxBackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int s5k5e3yx_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = module_i2c_addr;
    vendor_id->id_table = s5k5e3yx_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(s5k5e3yx_spm_vendor_id);

    return 0;
}

static int s5k5e3yx_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S s5k5e3yx_spm_Obj = {
    .name = "s5k5e3yx_spm",
    .pfnGetsnrCapSize = s5k5e3yx_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = s5k5e3yx_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = s5k5e3yx_spm_get_sensor_work_info,
    .pfnGetDevInfo = s5k5e3yx_spm_get_device_info,
    .pfnGetSnrVendorId = s5k5e3yx_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = s5k5e3yx_spm_get_sensor_i2c_addr,
};
