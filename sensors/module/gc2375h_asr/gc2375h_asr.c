/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "gc2375h_asr.h"
#include "gc2375h_asr_setting.h"

static const uint8_t gc2375h_module_i2c_addr = 0x17; /* I2C Address 7-bit*/

struct regval_tab gc2375h_asr_vendor_id[] = {
    {0xf0, 0x23},
    {0xf1, 0x75},
};

static SENSOR_MODULE_SETTING_S gc2375h_asr_setting = {
    .ispPreviewSetting =
        {
#include "gc2375h_asr_rear_secondary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "gc2375h_asr_rear_secondary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "gc2375h_asr_rear_secondary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "gc2375h_asr_rear_secondary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "gc2375h_asr_rear_secondary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "gc2375h_asr_rear_secondary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "gc2375h_asr_rear_secondary_nightshot_setting.h"
        },
};

const CapabilityElement gc2375hBackCapCfg[] = {
    {"capSensorPhysicalSize", "2.8, 2.1"},
    {"capLensFocalLength", "2.4"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", ""},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.6"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int gc2375h_asr_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = GC2375H_ASR_WORK_MODE_SIZE;
    return 0;
}

static int gc2375h_asr_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != GC2375H_ASR_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, GC2375H_ASR_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = GC2375H_ASR_WORK_MODE_SIZE;
    for (i = 0; i < GC2375H_ASR_WORK_MODE_SIZE; i++) {
        switch (i) {
            case GC2375H_ASR_1600x1200_10bit_LINEAR_30_1LANE: {
                sensor_capability->snr_config[i].width = 1600;
                sensor_capability->snr_config[i].height = 1200;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 1;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = GC2375H_ASR_1600x1200_10bit_LINEAR_30_1LANE;
                sensor_capability->snr_config[i].setting = &gc2375h_asr_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           GC2375H_ASR_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int gc2375h_asr_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = gc2375h_module_i2c_addr;
    // snr_info->id_table = gc2375h_asr_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(gc2375h_asr_vendor_id);

    switch (work_mode) {
        case GC2375H_ASR_1600x1200_10bit_LINEAR_30_1LANE: {
            snr_info->linetime = GC2375H_LINETIME_2M30_10bit_LINEAR;  // ns
            snr_info->vts = GC2375H_VMAX_2M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0x460 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = gc2375h_asr_1600x1200_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(gc2375h_asr_1600x1200_10bit_30fps_tab);
            snr_info->mipi_clock = 744;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       GC2375H_ASR_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &gc2375h_asr_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &gc2375h_asr_setting.ispVideoSetting;

    return 0;
}

static int gc2375h_asr_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 1:  // back aux
            dev_info->deviceConfig = gc2375hBackCapCfg;
            dev_info->deviceConfigCnt = sizeof(gc2375hBackCapCfg) / sizeof(gc2375hBackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int gc2375h_asr_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = gc2375h_module_i2c_addr;
    vendor_id->id_table = gc2375h_asr_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(gc2375h_asr_vendor_id);

    return 0;
}

static int gc2375h_asr_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = gc2375h_module_i2c_addr;
    return 0;
}

MODULE_OBJ_S gc2375h_asr_Obj = {
    .name = "gc2375h_asr",
    .pfnGetsnrCapSize = gc2375h_asr_get_sensor_cap_size,
    .pfnGetsnrCapbility = gc2375h_asr_get_sensor_capbility,
    .pfnGetSnrWorkInfo = gc2375h_asr_get_sensor_work_info,
    .pfnGetDevInfo = gc2375h_asr_get_device_info,
    .pfnGetSnrVendorId = gc2375h_asr_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = gc2375h_asr_get_sensor_i2c_addr,
};
