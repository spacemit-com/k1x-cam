/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 */
#include "sc640_spm.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "sc640_spm_setting.h"

static const uint8_t module_i2c_addr = 0x3c;

/*
 * The public product manual does not provide a readable vendor-id register map.
 * Use an empty probe table for now and rely on runtime stream validation.
 */
struct regval_tab sc640_spm_vendor_id[] = {
    {0x0000, 0x0000},
};

static SENSOR_MODULE_SETTING_S sc640_spm_setting = {
//     .ispPreviewSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_isp_setting.h"
//         },
//     .ispVideoSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_isp_setting_video.h"
//         },
//     .cppPreviewSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_cpp_preview_setting.h"
//         },
//     .cppVideoSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_cpp_video_setting.h"
//         },
//     .cppSnapshotSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_cpp_snapshot_setting.h"
//         },
//     .cppNightshotSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_cpp_nightshot_setting.h"
//         },
//     .nightshotSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_nightshot_setting.h"
//         },
};

const CapabilityElement sc640BackCapCfg[] = {
    {"capSensorPhysicalSize", "5.12, 4.10"},
    {"capLensFocalLength", "6.0"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "150"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "0.8"},
    {"capLensFilterDensities", "0"},
};

static int sc640_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = SC640_SPM_WORK_MODE_SIZE;
    return 0;
}

static int sc640_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != SC640_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, SC640_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = SC640_SPM_WORK_MODE_SIZE;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].width = 640;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].height = 512;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].bitDepth = 8;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].maxFps = 30;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].minFps = 5;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].image_mode = SENSOR_LINEAR_MODE;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].lane_num = 2;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].pattern = ISP_BAYER_PATTERN_MONO;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].supportPDAF = 0;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].work_mode =
        SC640_SPM_640x512_8bit_LINEAR_30_2LANE;
    sensor_capability->snr_config[SC640_SPM_640x512_8bit_LINEAR_30_2LANE].setting = &sc640_spm_setting;

    return 0;
}

static int sc640_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    switch (work_mode) {
        case SC640_SPM_640x512_8bit_LINEAR_30_2LANE:
            snr_info->linetime = SC640_LINETIME_512P30_8bit_LINEAR;
            snr_info->vts = SC640_VMAX_512P30_8bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = snr_info->vts * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;
            snr_info->dgain[0] = 1 * 0x1000;
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = sc640_spm_640x512_8bit_30fps_tab;
            snr_info->setting_table_size = 0;
            snr_info->mipi_clock = 297;
            break;
        default:
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       SC640_SPM_WORK_MODE_SIZE);
            return -EINVAL;
    }

    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &sc640_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &sc640_spm_setting.ispVideoSetting;
    return 0;
}

static int sc640_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:
            dev_info->deviceConfig = sc640BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(sc640BackCapCfg) / sizeof(sc640BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int sc640_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    vendor_id->id_table = sc640_spm_vendor_id;
    vendor_id->id_table_size = 0;
    return 0;
}

static int sc640_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S sc640_spm_Obj = {
    .name = "sc640_spm",
    .pfnGetsnrCapSize = sc640_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = sc640_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = sc640_spm_get_sensor_work_info,
    .pfnGetDevInfo = sc640_spm_get_device_info,
    .pfnGetSnrVendorId = sc640_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = sc640_spm_get_sensor_i2c_addr,
};
