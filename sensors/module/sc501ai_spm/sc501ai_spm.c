/*
 * Copyright (C) 2023 SPM Micro Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sc501ai_spm.h"
#include "sc501ai_spm_setting.h"

static const uint8_t module_i2c_addr = 0x30; /* I2C Address 7-bit*/

struct regval_tab sc501ai_spm_vendor_id[] = {
    {0x3107, 0xce},
    {0x3108, 0x1f},
};

static SENSOR_MODULE_SETTING_S sc501ai_spm_setting = {
    .ispPreviewSetting =
        {
#include "sc501ai_spm_rear_primary_isp_setting.h"
        },
    .cppPreviewSetting =
        {
#include "sc501ai_spm_rear_primary_cpp_preview_setting.h"
        },
};

const CapabilityElement sc501aiBackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.46"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int sc501ai_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = 1;
    return 0;
}

static int sc501ai_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);

    if (capArraySize != SC501AI_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("snr_config_num(%d) is not equal with work_mode size(%d)",
                   sensor_capability->snr_config_num, SC501AI_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = SC501AI_SPM_WORK_MODE_SIZE;
    for (i = 0; i < SC501AI_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case SC501AI_SPM_2688x1616_10bit_LINEAR_30_2LANE: {
                sensor_capability->snr_config[i].width = 2688;
                sensor_capability->snr_config[i].height = 1616;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 25;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 2;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = SC501AI_SPM_2688x1616_10bit_LINEAR_30_2LANE;
                sensor_capability->snr_config[i].setting = &sc501ai_spm_setting;
            } break;
            default: {
                CLOG_ERROR("invalid work mode (%d) for max workmode (%d)", i, SC501AI_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int sc501ai_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = module_i2c_addr;
    // snr_info->id_table = sc501ai_spm_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(sc501ai_spm_vendor_id);

    switch (work_mode) {
        case SC501AI_SPM_2688x1616_10bit_LINEAR_30_2LANE: {
            snr_info->linetime = 20202;  //ns, Tline = hts/pclk
            snr_info->vts = 1650;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = (1650 - 10) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = sc501ai_spm_2688x1520_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(sc501ai_spm_2688x1520_10bit_30fps_tab);
            snr_info->mipi_clock = 792;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       SC501AI_SPM_WORK_MODE_SIZE);
        } break;
    }
    
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &sc501ai_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &sc501ai_spm_setting.ispVideoSetting;

    return 0;
}

static int sc501ai_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:  // back
            dev_info->deviceConfig = sc501aiBackCapCfg;
            dev_info->deviceConfigCnt = sizeof(sc501aiBackCapCfg) / sizeof(sc501aiBackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int sc501ai_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = module_i2c_addr;
    vendor_id->id_table = sc501ai_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(sc501ai_spm_vendor_id);

    return 0;
}

static int sc501ai_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    *i2c_addr = module_i2c_addr;

    return 0;
}

MODULE_OBJ_S sc501ai_spm_Obj = {
    .name = "sc501ai_spm",
    .pfnGetsnrCapSize = sc501ai_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = sc501ai_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = sc501ai_spm_get_sensor_work_info,
    .pfnGetDevInfo = sc501ai_spm_get_device_info,
    .pfnGetSnrVendorId = sc501ai_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = sc501ai_spm_get_sensor_i2c_addr,
};
