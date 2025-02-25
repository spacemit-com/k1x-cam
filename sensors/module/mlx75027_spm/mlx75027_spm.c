/*
 * Copyright (C) 2024 Spacemit Limited
 * All Rights Reserved.
 */
#include "mlx75027_spm.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "mlx75027_spm_setting.h"

static const uint8_t module_i2c_addr = 0x57; /* I2C Address 7-bit, SID LOW */

struct regval_tab mlx75027_spm_vendor_id[] = {
    // {0x0308, 0xdb},
    // {0x0824, 0x4e},
    // {0x0000, 0x99}, // fake id.
    {0x2120, 0x00}, // fake id.
    {0x2121, 0x01}, // fake id.
    {0x2122, 0xD4}, // fake id.
    {0x2123, 0xC0}, // fake id.
};

static SENSOR_MODULE_SETTING_S mlx75027_spm_setting = {
    .ispPreviewSetting =
        {
#include "mlx75027_spm_rear_primary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "mlx75027_spm_rear_primary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "mlx75027_spm_rear_primary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "mlx75027_spm_rear_primary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "mlx75027_spm_rear_primary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "mlx75027_spm_rear_primary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "mlx75027_spm_rear_primary_nightshot_setting.h"
        },
};

const CapabilityElement mlx75027BackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.64"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int mlx75027_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = MLX75027_SPM_WORK_MODE_SIZE;
    return 0;
}

static int mlx75027_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != MLX75027_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, MLX75027_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = MLX75027_SPM_WORK_MODE_SIZE;
    for (i = 0; i < MLX75027_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case MLX75027_SPM_640x480_12bit_LINEAR_30_2LANE: {
                sensor_capability->snr_config[i].width = 640;
                sensor_capability->snr_config[i].height = 482;
                sensor_capability->snr_config[i].bitDepth = 12;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 25;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = MLX75027_SPM_640x480_12bit_LINEAR_30_2LANE;
                sensor_capability->snr_config[i].setting = &mlx75027_spm_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           MLX75027_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int mlx75027_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    switch (work_mode) {
        case MLX75027_SPM_640x480_12bit_LINEAR_30_2LANE: {
            snr_info->linetime = MLX75027_LINETIME_480P30_12bit_LINEAR;  // ns
            snr_info->vts = MLX75027_VMAX_480P30_12bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 480 * snr_info->linetime / 1000; // 480 is vts
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = mlx75027_spm_640x480_12bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(mlx75027_spm_640x480_12bit_30fps_tab);
            snr_info->mipi_clock = 960;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       MLX75027_SPM_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &mlx75027_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &mlx75027_spm_setting.ispVideoSetting;

    return 0;
}

static int mlx75027_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:  // back
            dev_info->deviceConfig = mlx75027BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(mlx75027BackCapCfg) / sizeof(mlx75027BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int mlx75027_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    vendor_id->id_table = mlx75027_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(mlx75027_spm_vendor_id);

    return 0;
}

static int mlx75027_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S mlx75027_spm_Obj = {
    .name = "mlx75027_spm",
    .pfnGetsnrCapSize = mlx75027_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = mlx75027_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = mlx75027_spm_get_sensor_work_info,
    .pfnGetDevInfo = mlx75027_spm_get_device_info,
    .pfnGetSnrVendorId = mlx75027_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = mlx75027_spm_get_sensor_i2c_addr,
};
