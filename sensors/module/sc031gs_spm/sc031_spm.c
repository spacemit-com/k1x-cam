/*
 * Copyright (C) 2023 SPM Micro Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sc031_spm.h"
#include "sc031_spm_setting.h"

static const uint8_t module_i2c_addr = 0x30; /* I2C Address 7-bit*/

struct regval_tab sc031_spm_vendor_id[] = {
    {0x3108, 0x31},
};

static SENSOR_MODULE_SETTING_S sc031_spm_setting = {
    .ispPreviewSetting =
        {
#include "sc031_spm_rear_primary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "sc031_spm_rear_primary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "sc031_spm_rear_primary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "sc031_spm_rear_primary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "sc031_spm_rear_primary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "sc031_spm_rear_primary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "sc031_spm_rear_primary_nightshot_setting.h"
        },
};

const CapabilityElement sc031BackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.46"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int sc031_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = 1;
    return 0;
}

static int sc031_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);

    if (capArraySize != SC031_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("snr_config_num(%d) is not equal with work_mode size(%d)",
                   sensor_capability->snr_config_num, SC031_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = SC031_SPM_WORK_MODE_SIZE;
    for (i = 0; i < SC031_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case SC031_SPM_640x480_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 640;
                sensor_capability->snr_config[i].height = 480;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 25;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 1;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = SC031_SPM_640x480_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &sc031_spm_setting;
            } break;
            default: {
                CLOG_ERROR("invalid work mode (%d) for max workmode (%d)", i, SC031_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int sc031_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = module_i2c_addr;
    // snr_info->id_table = sc031_spm_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(sc031_spm_vendor_id);

    switch (work_mode) {
        case SC031_SPM_640x480_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = 12194;  //ns, Tline = hts/pclk
            snr_info->vts = 2732;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0xaac * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = sc031_spm_640x480_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(sc031_spm_640x480_10bit_30fps_tab);
            snr_info->mipi_clock = 720;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       SC031_SPM_WORK_MODE_SIZE);
        } break;
    }
    
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &sc031_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &sc031_spm_setting.ispVideoSetting;

    return 0;
}

static int sc031_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:  // back
            dev_info->deviceConfig = sc031BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(sc031BackCapCfg) / sizeof(sc031BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int sc031_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = module_i2c_addr;
    vendor_id->id_table = sc031_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(sc031_spm_vendor_id);

    return 0;
}

static int sc031_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;

    return 0;
}

MODULE_OBJ_S sc031_spm_Obj = {
    .name = "sc031_spm",
    .pfnGetsnrCapSize = sc031_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = sc031_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = sc031_spm_get_sensor_work_info,
    .pfnGetDevInfo = sc031_spm_get_device_info,
    .pfnGetSnrVendorId = sc031_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = sc031_spm_get_sensor_i2c_addr,
};
