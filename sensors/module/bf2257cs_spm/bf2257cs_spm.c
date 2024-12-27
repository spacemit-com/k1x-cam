/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "bf2257cs_spm.h"
#include "bf2257cs_spm_setting.h"

static const uint8_t bf2257cs_module_i2c_addr = 0x3e; //0x6e

struct regval_tab bf2257cs_spm_vendor_id[] = {
    {0xfc, 0x22},
    {0xfd, 0x57},
};

static SENSOR_MODULE_SETTING_S bf2257cs_spm_setting = {
    .ispPreviewSetting =
        {
#include "bf2257cs_spm_rear_secondary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "bf2257cs_spm_rear_secondary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "bf2257cs_spm_rear_secondary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "bf2257cs_spm_rear_secondary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "bf2257cs_spm_rear_secondary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "bf2257cs_spm_rear_secondary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "bf2257cs_spm_rear_secondary_nightshot_setting.h"
        },
};

const CapabilityElement bf2257csBackCapCfg[] = {
    {"capSensorPhysicalSize", "2.8, 2.1"},
    {"capLensFocalLength", "2.4"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", ""},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.6"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int bf2257cs_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = BF2257CS_SPM_WORK_MODE_SIZE;
    return 0;
}

static int bf2257cs_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != BF2257CS_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, BF2257CS_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = BF2257CS_SPM_WORK_MODE_SIZE;
    for (i = 0; i < BF2257CS_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case BF2257CS_SPM_1600x1200_10bit_LINEAR_30_1LANE: {
                sensor_capability->snr_config[i].width = 1600;
                sensor_capability->snr_config[i].height = 1200;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 1;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = BF2257CS_SPM_1600x1200_10bit_LINEAR_30_1LANE;
                sensor_capability->snr_config[i].setting = &bf2257cs_spm_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           BF2257CS_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int bf2257cs_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = bf2257cs_module_i2c_addr;
    // snr_info->id_table = bf2257cs_spm_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(bf2257cs_spm_vendor_id);

    switch (work_mode) {
        case BF2257CS_SPM_1600x1200_10bit_LINEAR_30_1LANE: {
            snr_info->linetime = BF2257CS_LINETIME_2M30_10bit_LINEAR;  // ns
            snr_info->vts = BF2257CS_VMAX_2M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0x4E2 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = bf2257cs_spm_1600x1200_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(bf2257cs_spm_1600x1200_10bit_30fps_tab);
            snr_info->mipi_clock = 672;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       BF2257CS_SPM_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &bf2257cs_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &bf2257cs_spm_setting.ispVideoSetting;

    return 0;
}

static int bf2257cs_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 1:  // back aux
            dev_info->deviceConfig = bf2257csBackCapCfg;
            dev_info->deviceConfigCnt = sizeof(bf2257csBackCapCfg) / sizeof(bf2257csBackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int bf2257cs_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = bf2257cs_module_i2c_addr;
    vendor_id->id_table = bf2257cs_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(bf2257cs_spm_vendor_id);

    return 0;
}

static int bf2257cs_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = bf2257cs_module_i2c_addr;
    return 0;
}

MODULE_OBJ_S bf2257cs_spm_Obj = {
    .name = "bf2257cs_spm",
    .pfnGetsnrCapSize = bf2257cs_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = bf2257cs_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = bf2257cs_spm_get_sensor_work_info,
    .pfnGetDevInfo = bf2257cs_spm_get_device_info,
    .pfnGetSnrVendorId = bf2257cs_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = bf2257cs_spm_get_sensor_i2c_addr,
};
