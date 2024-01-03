/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#include "ov08d10_asr.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ov08d10_asr_setting.h"

static const uint8_t ov08d10_module_i2c_addr = 0x36; /* I2C Address 7-bit*/

struct regval_tab ov08d10_asr_vendor_id[] = {
    {0x00, 0x56},
    {0x01, 0x08},
    {0x02, 0x47},
};

static SENSOR_MODULE_SETTING_S ov08d10_asr_setting = {
    .ispPreviewSetting =
        {
#include "ov08d10_asr_front_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "ov08d10_asr_front_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "ov08d10_asr_front_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "ov08d10_asr_front_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "ov08d10_asr_front_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "ov08d10_asr_front_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "ov08d10_asr_front_nightshot_setting.h"
        },
};

const CapabilityElement ov08d10FrontCapCfg[] = {
    {"capSensorPhysicalSize", "3.67, 2.76"},
    {"capLensOpticalStabMode", "off"},
    {"capLensFocalLength", "1.65"},
    {"capLensMinFocusDistanc", ""},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int ov08d10_asr_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = OV08D10_ASR_WORK_MODE_SIZE;
    return 0;
}

static int ov08d10_asr_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != OV08D10_ASR_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, OV08D10_ASR_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = OV08D10_ASR_WORK_MODE_SIZE;
    for (i = 0; i < OV08D10_ASR_WORK_MODE_SIZE; i++) {
        switch (i) {
            case OV08D10_ASR_3264x2448_10bit_LINEAR_30_2LANE: {
                sensor_capability->snr_config[i].width = 3264;
                sensor_capability->snr_config[i].height = 2448;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = OV08D10_ASR_3264x2448_10bit_LINEAR_30_2LANE;
                sensor_capability->snr_config[i].setting = &ov08d10_asr_setting;
            } break;
            case OV08D10_ASR_1632x1224_10bit_LINEAR_30_2LANE: {
                sensor_capability->snr_config[i].width = 1632;
                sensor_capability->snr_config[i].height = 1224;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = OV08D10_ASR_1632x1224_10bit_LINEAR_30_2LANE;
                sensor_capability->snr_config[i].setting = &ov08d10_asr_setting;
            } break;
            case OV08D10_ASR_1920x1080_10bit_LINEAR_30_2LANE: {
                sensor_capability->snr_config[i].width = 1920;
                sensor_capability->snr_config[i].height = 1080;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = OV08D10_ASR_1920x1080_10bit_LINEAR_30_2LANE;
                sensor_capability->snr_config[i].setting = &ov08d10_asr_setting;
            } break;
            case OV08D10_ASR_640x480_10bit_LINEAR_30_1LANE: {
                sensor_capability->snr_config[i].width = 640;
                sensor_capability->snr_config[i].height = 480;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 1;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = OV08D10_ASR_640x480_10bit_LINEAR_30_1LANE;
                sensor_capability->snr_config[i].setting = &ov08d10_asr_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           OV08D10_ASR_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int ov08d10_asr_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = ov08d10_module_i2c_addr;
    // snr_info->id_table = ov08d10_asr_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(ov08d10_asr_vendor_id);

    switch (work_mode) {
        case OV08D10_ASR_3264x2448_10bit_LINEAR_30_2LANE: {
            snr_info->linetime = OV08D10_LINETIME_8M30_10bit_LINEAR;  // 0.1 us
            snr_info->vts = OV08D10_VMAX_8M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = (0x1250 / 2) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = ov08d10_asr_3264x2448_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(ov08d10_asr_3264x2448_10bit_30fps_tab);
            snr_info->mipi_clock = 1440;  // Mhz
        } break;
        case OV08D10_ASR_1632x1224_10bit_LINEAR_30_2LANE: {
            snr_info->linetime = OV08D10_LINETIME_2M30_10bit_LINEAR;  // 0.1 us
            snr_info->vts = OV08D10_VMAX_2M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = (0x96a / 2) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = ov08d10_asr_1632x1224_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(ov08d10_asr_1632x1224_10bit_30fps_tab);
            snr_info->mipi_clock = 720;  // Mhz
        } break;
        case OV08D10_ASR_1920x1080_10bit_LINEAR_30_2LANE: {
            snr_info->linetime = OV08D10_LINETIME_1080P30_10bit_LINEAR;  // 0.1 us
            snr_info->vts = OV08D10_VMAX_1080P30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = (0x8b9 / 2) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = ov08d10_asr_1920x1080_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(ov08d10_asr_1920x1080_10bit_30fps_tab);
            snr_info->mipi_clock = 882;  // Mhz
        } break;
        case OV08D10_ASR_640x480_10bit_LINEAR_30_1LANE: {
            snr_info->linetime = OV08D10_LINETIME_VGA30_10bit_LINEAR;  // 0.1 us
            snr_info->vts = OV08D10_VMAX_VGA30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = (0x16a / 2) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = ov08d10_asr_640x480_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(ov08d10_asr_640x480_10bit_30fps_tab);
            snr_info->mipi_clock = 720;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       OV08D10_ASR_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;

    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &ov08d10_asr_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &ov08d10_asr_setting.ispVideoSetting;

    return 0;
}

static int ov08d10_asr_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 2:  // front
            dev_info->deviceConfig = ov08d10FrontCapCfg;
            dev_info->deviceConfigCnt = sizeof(ov08d10FrontCapCfg) / sizeof(ov08d10FrontCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int ov08d10_asr_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = ov08d10_module_i2c_addr;
    vendor_id->id_table = ov08d10_asr_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(ov08d10_asr_vendor_id);

    return 0;
}

static int ov08d10_asr_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = ov08d10_module_i2c_addr;
    return 0;
}

MODULE_OBJ_S ov08d10_asr_Obj = {
    .name = "ov08d10_asr",
    .pfnGetsnrCapSize = ov08d10_asr_get_sensor_cap_size,
    .pfnGetsnrCapbility = ov08d10_asr_get_sensor_capbility,
    .pfnGetSnrWorkInfo = ov08d10_asr_get_sensor_work_info,
    .pfnGetDevInfo = ov08d10_asr_get_device_info,
    .pfnGetSnrVendorId = ov08d10_asr_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = ov08d10_asr_get_sensor_i2c_addr,
};
