/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "gc5035_asr.h"
#include "gc5035_asr_setting.h"

static const uint8_t gc5035_module_i2c_addr = 0x37; /* I2C Address 7-bit*/

struct regval_tab gc5035_asr_vendor_id[] = {
    {0xf0, 0x50},
    {0xf1, 0x35},
};

static SENSOR_MODULE_SETTING_S gc5035_asr_setting = {
    .ispPreviewSetting =
        {
#include "gc5035_asr_rear_secondary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "gc5035_asr_rear_secondary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "gc5035_asr_rear_secondary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "gc5035_asr_rear_secondary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "gc5035_asr_rear_secondary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "gc5035_asr_rear_secondary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "gc5035_asr_rear_secondary_nightshot_setting.h"
        },
};

/********************************************************************/
static int gc5035_asr_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = GC5035_ASR_WORK_MODE_SIZE;
    return 0;
}

static int gc5035_asr_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != GC5035_ASR_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, GC5035_ASR_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = GC5035_ASR_WORK_MODE_SIZE;
    for (i = 0; i < GC5035_ASR_WORK_MODE_SIZE; i++) {
        switch (i) {
            case GC5035_ASR_2592x1944_10bit_LINEAR_30_2LANE: {
                sensor_capability->snr_config[i].width = 2592;
                sensor_capability->snr_config[i].height = 1944;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = GC5035_ASR_2592x1944_10bit_LINEAR_30_2LANE;
                sensor_capability->snr_config[i].setting = &gc5035_asr_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           GC5035_ASR_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int gc5035_asr_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = gc5035_module_i2c_addr;
    // snr_info->id_table = gc5035_asr_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(gc5035_asr_vendor_id);

    switch (work_mode) {
        case GC5035_ASR_2592x1944_10bit_LINEAR_30_2LANE: {
            snr_info->linetime = GC5035_LINETIME_8M30_10bit_LINEAR;  // ns
            snr_info->vts = GC5035_VMAX_8M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 0x7c8 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = gc5035_asr_2592x1944_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(gc5035_asr_2592x1944_10bit_30fps_tab);
            snr_info->mipi_clock = 945;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       GC5035_ASR_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &gc5035_asr_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &gc5035_asr_setting.ispVideoSetting;

    return 0;
}

static int gc5035_asr_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = gc5035_module_i2c_addr;
    vendor_id->id_table = gc5035_asr_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(gc5035_asr_vendor_id);

    return 0;
}

static int gc5035_asr_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = gc5035_module_i2c_addr;
    return 0;
}

MODULE_OBJ_S gc5035_asr_Obj = {
    .name = "gc5035_asr",
    .pfnGetsnrCapSize = gc5035_asr_get_sensor_cap_size,
    .pfnGetsnrCapbility = gc5035_asr_get_sensor_capbility,
    .pfnGetSnrWorkInfo = gc5035_asr_get_sensor_work_info,
    .pfnGetDevInfo = NULL,
    .pfnGetSnrVendorId = gc5035_asr_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = gc5035_asr_get_sensor_i2c_addr,
};
