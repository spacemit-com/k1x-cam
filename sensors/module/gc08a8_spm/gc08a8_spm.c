/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "gc08a8_spm.h"
#include "gc08a8_spm_setting.h"

//0x10, 0x11, 0x12
static const uint8_t module_i2c_addr = 0x31; /* I2C Address 7-bit*/

struct regval_tab gc08a8_spm_vendor_id[] = {
    {0x03f0, 0x08},
    {0x03f1, 0xa8},
};

static SENSOR_MODULE_SETTING_S gc08a8_spm_setting = {
    .ispPreviewSetting =
        {
#include "gc08a8_spm_rear_secondary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "gc08a8_spm_rear_secondary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "gc08a8_spm_rear_secondary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "gc08a8_spm_rear_secondary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "gc08a8_spm_rear_secondary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "gc08a8_spm_rear_secondary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "gc08a8_spm_rear_secondary_nightshot_setting.h"
        },
};

const CapabilityElement gc08a8BackCapCfg[] = {
    {"capSensorPhysicalSize", "2.8, 2.1"},
    {"capLensFocalLength", "2.4"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", ""},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.6"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int gc08a8_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = GC08A8_SPM_WORK_MODE_SIZE;
    return 0;
}

static int gc08a8_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != GC08A8_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, GC08A8_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = GC08A8_SPM_WORK_MODE_SIZE;
    for (i = 0; i < GC08A8_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case GC08A8_SPM_3264x2448_10bit_LINEAR_30_2LANE: {
                sensor_capability->snr_config[i].width = 3264;
                sensor_capability->snr_config[i].height = 2448;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = GC08A8_SPM_3264x2448_10bit_LINEAR_30_2LANE;
                sensor_capability->snr_config[i].setting = &gc08a8_spm_setting;
            } break;
            case GC08A8_SPM_3264x2448_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 3264;
                sensor_capability->snr_config[i].height = 2448;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 28;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = GC08A8_SPM_3264x2448_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &gc08a8_spm_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           GC08A8_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int gc08a8_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = gc08a8_module_i2c_addr;
    // snr_info->id_table = gc08a8_spm_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(gc08a8_spm_vendor_id);

    switch (work_mode) {
        case GC08A8_SPM_3264x2448_10bit_LINEAR_30_2LANE: {
            snr_info->linetime = GC08A8_LINETIME_8M30_10bit_LINEAR;  // ns
            snr_info->vts = GC08A8_VMAX_8M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 2548 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = gc08a8_spm_3264x2448_10bit_30fps_2lane_tab;
            snr_info->setting_table_size = ARRAY_SIZE(gc08a8_spm_3264x2448_10bit_30fps_2lane_tab);
            snr_info->mipi_clock = 1400;  // Mhz
        } break;
        case GC08A8_SPM_3264x2448_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = GC08A8_LINETIME_8M30_10bit_LINEAR;  // ns
            snr_info->vts = GC08A8_VMAX_8M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 2548 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = gc08a8_spm_3264x2448_10bit_30fps_4lane_tab;
            snr_info->setting_table_size = ARRAY_SIZE(gc08a8_spm_3264x2448_10bit_30fps_4lane_tab);
            snr_info->mipi_clock = 700;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       GC08A8_SPM_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &gc08a8_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &gc08a8_spm_setting.ispVideoSetting;

    return 0;
}

static int gc08a8_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 1:  // back aux
            dev_info->deviceConfig = gc08a8BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(gc08a8BackCapCfg) / sizeof(gc08a8BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int gc08a8_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = gc08a8_module_i2c_addr;
    vendor_id->id_table = gc08a8_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(gc08a8_spm_vendor_id);

    return 0;
}

static int gc08a8_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;

    return 0;
}

MODULE_OBJ_S gc08a8_spm_Obj = {
    .name = "gc08a8_spm",
    .pfnGetsnrCapSize = gc08a8_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = gc08a8_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = gc08a8_spm_get_sensor_work_info,
    .pfnGetDevInfo = gc08a8_spm_get_device_info,
    .pfnGetSnrVendorId = gc08a8_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = gc08a8_spm_get_sensor_i2c_addr,
};
