/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#include "imx135_asr.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "imx135_asr_setting.h"

static const uint8_t module_i2c_addr = 0x10; /* I2C Address 7-bit*/  // 0x34

struct regval_tab imx135_asr_vendor_id[] = {
    {0x0016, 0x01},
    {0x0017, 0x35},
};

static SENSOR_MODULE_SETTING_S imx135_asr_setting = {
    .ispPreviewSetting =
        {
#include "imx135_asr_rear_primary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "imx135_asr_rear_primary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "imx135_asr_rear_primary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "imx135_asr_rear_primary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "imx135_asr_rear_primary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "imx135_asr_rear_primary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "imx135_asr_rear_primary_nightshot_setting.h"
        },
};

const CapabilityElement imx135BackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.46"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int imx135_asr_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = IMX135_ASR_WORK_MODE_SIZE;
    return 0;
}

static int imx135_asr_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != IMX135_ASR_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, IMX135_ASR_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = IMX135_ASR_WORK_MODE_SIZE;
    for (i = 0; i < IMX135_ASR_WORK_MODE_SIZE; i++) {
        switch (i) {
            case IMX135_ASR_4208x3120_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 4208;
                sensor_capability->snr_config[i].height = 3120;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = IMX135_ASR_4208x3120_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &imx135_asr_setting;
            } break;
            case IMX135_ASR_4208x2416_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 4208;
                sensor_capability->snr_config[i].height = 2416;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = IMX135_ASR_4208x2416_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &imx135_asr_setting;
            } break;
            case IMX135_ASR_2104x1560_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 2104;
                sensor_capability->snr_config[i].height = 1560;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = IMX135_ASR_2104x1560_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &imx135_asr_setting;
            } break;
            case IMX135_ASR_2104x1184_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 2104;
                sensor_capability->snr_config[i].height = 1184;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = IMX135_ASR_2104x1184_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &imx135_asr_setting;
            } break;
            case IMX135_ASR_1920x1080_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 1920;
                sensor_capability->snr_config[i].height = 1080;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = IMX135_ASR_1920x1080_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &imx135_asr_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           IMX135_ASR_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int imx135_asr_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = module_i2c_addr;
    // snr_info->id_table = imx135_asr_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(imx135_asr_vendor_id);

    switch (work_mode) {
        case IMX135_ASR_4208x3120_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = IMX135_LINETIME_13M30_10bit_LINEAR;  // ns
            snr_info->vts = IMX135_VMAX_13M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0xc10 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = imx135_asr_4208x3120_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(imx135_asr_4208x3120_10bit_30fps_tab);
            snr_info->mipi_clock = 1080;  // Mhz
        } break;
        case IMX135_ASR_4208x2416_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = IMX135_LINETIME_10M30_10bit_LINEAR;  // ns
            snr_info->vts = IMX135_VMAX_10M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0xa10 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = imx135_asr_4208x2416_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(imx135_asr_4208x2416_10bit_30fps_tab);
            snr_info->mipi_clock = 1080;  // Mhz
        } break;
        case IMX135_ASR_2104x1560_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = IMX135_LINETIME_3M30_10bit_LINEAR;  // ns
            snr_info->vts = IMX135_VMAX_3M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0x610 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = imx135_asr_2104x1560_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(imx135_asr_2104x1560_10bit_30fps_tab);
            snr_info->mipi_clock = 555;  // Mhz
        } break;
        case IMX135_ASR_2104x1184_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = IMX135_LINETIME_2500K30_10bit_LINEAR;  // ns
            snr_info->vts = IMX135_VMAX_2500K30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0x610 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = imx135_asr_2104x1184_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(imx135_asr_2104x1184_10bit_30fps_tab);
            snr_info->mipi_clock = 555;  // Mhz
        } break;
        case IMX135_ASR_1920x1080_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = IMX135_LINETIME_1080P30_10bit_LINEAR;  // ns
            snr_info->vts = IMX135_VMAX_1080P30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0x400 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = imx135_asr_1920x1080_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(imx135_asr_1920x1080_10bit_30fps_tab);
            snr_info->mipi_clock = 560;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       IMX135_ASR_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &imx135_asr_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &imx135_asr_setting.ispVideoSetting;

    return 0;
}

static int imx135_asr_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:  // back
            dev_info->deviceConfig = imx135BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(imx135BackCapCfg) / sizeof(imx135BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int imx135_asr_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = module_i2c_addr;
    vendor_id->id_table = imx135_asr_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(imx135_asr_vendor_id);

    return 0;
}

static int imx135_asr_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S imx135_asr_Obj = {
    .name = "imx135_asr",
    .pfnGetsnrCapSize = imx135_asr_get_sensor_cap_size,
    .pfnGetsnrCapbility = imx135_asr_get_sensor_capbility,
    .pfnGetSnrWorkInfo = imx135_asr_get_sensor_work_info,
    .pfnGetDevInfo = imx135_asr_get_device_info,
    .pfnGetSnrVendorId = imx135_asr_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = imx135_asr_get_sensor_i2c_addr,
};
