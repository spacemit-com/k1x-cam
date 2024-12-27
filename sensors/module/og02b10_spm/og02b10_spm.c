/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include "og02b10_spm.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "og02b10_spm_setting.h"

static const uint8_t module_i2c_addr = 0x60; /* I2C Address 7-bit, SID LOW */

struct regval_tab og02b10_spm_vendor_id[] = {
    {0x300c, 0xa0},
};

static SENSOR_MODULE_SETTING_S og02b10_spm_setting = {
    .ispPreviewSetting =
        {
#include "og02b10_spm_rear_primary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "og02b10_spm_rear_primary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "og02b10_spm_rear_primary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "og02b10_spm_rear_primary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "og02b10_spm_rear_primary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "og02b10_spm_rear_primary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "og02b10_spm_rear_primary_nightshot_setting.h"
        },
};

const CapabilityElement og02b10BackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.64"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int og02b10_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = OG02B10_SPM_WORK_MODE_SIZE;
    return 0;
}

static int og02b10_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != OG02B10_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, OG02B10_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = OG02B10_SPM_WORK_MODE_SIZE;
    for (i = 0; i < OG02B10_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case OG02B10_SPM_1600x1300_10bit_LINEAR_60_2LANE: {
                sensor_capability->snr_config[i].width = 1600;
                sensor_capability->snr_config[i].height = 1300;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 60;
                sensor_capability->snr_config[i].minFps = 5;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = OG02B10_SPM_1600x1300_10bit_LINEAR_60_2LANE;
                sensor_capability->snr_config[i].setting = &og02b10_spm_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           OG02B10_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int og02b10_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    switch (work_mode) {
        case OG02B10_SPM_1600x1300_10bit_LINEAR_60_2LANE: {
            snr_info->linetime = OG02B10_LINETIME_1300P60_10bit_LINEAR;  // ns
            snr_info->vts = OG02B10_VMAX_1300P60_10bit_LINEAR;
            snr_info->f32maxFps = 60;
            snr_info->exp_time[0] = 1416 * snr_info->linetime / 1000; // 1416 is vts
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = og02b10_spm_1600x1300_10bit_60fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(og02b10_spm_1600x1300_10bit_60fps_tab);
            snr_info->mipi_clock = 400;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       OG02B10_SPM_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &og02b10_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &og02b10_spm_setting.ispVideoSetting;

    return 0;
}

static int og02b10_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:  // back
            dev_info->deviceConfig = og02b10BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(og02b10BackCapCfg) / sizeof(og02b10BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int og02b10_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    vendor_id->id_table = og02b10_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(og02b10_spm_vendor_id);

    return 0;
}

static int og02b10_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;
    printf("i2c_addr = 0x%x\n", *i2c_addr);
    return 0;
}

MODULE_OBJ_S og02b10_spm_Obj = {
    .name = "og02b10_spm",
    .pfnGetsnrCapSize = og02b10_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = og02b10_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = og02b10_spm_get_sensor_work_info,
    .pfnGetDevInfo = og02b10_spm_get_device_info,
    .pfnGetSnrVendorId = og02b10_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = og02b10_spm_get_sensor_i2c_addr,
};
