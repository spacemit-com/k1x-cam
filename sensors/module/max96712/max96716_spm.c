/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include "max96716_spm.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "max96716_spm_setting.h"

static const uint8_t module_i2c_addr = 0x28; /* I2C Address 7-bit*/  // 0x48

struct regval_tab max96716_spm_vendor_id[] = {
    // {0x000d, 0xbe},
    // {0x000e, 0x03},
    {0x000d, 0xbe},
    {0x000e, 0x84},
};

static SENSOR_MODULE_SETTING_S max96716_spm_setting = {
    .ispPreviewSetting =
        {
#include "max96716_spm_rear_primary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "max96716_spm_rear_primary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "max96716_spm_rear_primary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "max96716_spm_rear_primary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "max96716_spm_rear_primary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "max96716_spm_rear_primary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "max96716_spm_rear_primary_nightshot_setting.h"
        },
};

const CapabilityElement max96716BackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.46"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int max96716_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = MAX96716_SPM_WORK_MODE_SIZE;
    return 0;
}

static int max96716_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != MAX96716_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, MAX96716_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = MAX96716_SPM_WORK_MODE_SIZE;
    for (i = 0; i < MAX96716_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case MAX96716_SPM_3864x2192_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 640;
                sensor_capability->snr_config[i].height = 480;
                sensor_capability->snr_config[i].bitDepth = 12;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_GBRG;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = MAX96716_SPM_3864x2192_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &max96716_spm_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           MAX96716_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int max96716_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = module_i2c_addr;
    // snr_info->id_table = max96716_spm_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(max96716_spm_vendor_id);

    switch (work_mode) {
        case MAX96716_SPM_3864x2192_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = 16842;  // ns
            snr_info->vts = 640;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = (400) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = max96716_spm_640x480_12bit_60fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(max96716_spm_640x480_12bit_60fps_tab);
            snr_info->mipi_clock = 960;  // Mhz
            // snr_info->mclk = 37125000;  // hz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       MAX96716_SPM_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &max96716_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &max96716_spm_setting.ispVideoSetting;

    return 0;
}

static int max96716_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:  // back
            dev_info->deviceConfig = max96716BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(max96716BackCapCfg) / sizeof(max96716BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int max96716_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = module_i2c_addr;
    vendor_id->id_table = max96716_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(max96716_spm_vendor_id);

    return 0;
}

static int max96716_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S max96716_spm_Obj = {
    .name = "max96716_max96717_imx556_spm",
    .pfnGetsnrCapSize = max96716_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = max96716_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = max96716_spm_get_sensor_work_info,
    .pfnGetDevInfo = max96716_spm_get_device_info,
    .pfnGetSnrVendorId = max96716_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = max96716_spm_get_sensor_i2c_addr,
};
