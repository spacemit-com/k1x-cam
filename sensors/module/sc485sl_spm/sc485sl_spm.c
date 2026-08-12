/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 *
 * SC485SL module.  The two mode tables are transcribed from SmartSens
 * SC485SL 27 MHz, four-lane reference INI files.
 */
#include <errno.h>
#include <string.h>

#include "sc485sl_spm.h"
#include "sc485sl_spm_setting.h"

static SENSOR_MODULE_SETTING_S sc485sl_spm_setting = {
    .ispPreviewSetting =
        {
#include "sc485sl_spm_rear_primary_isp_setting.h"
        },
};

static const uint8_t module_i2c_addr = 0x30;

static struct regval_tab sc485sl_spm_vendor_id[] = {
    {0x3107, 0xbd},
    {0x3108, 0x82},
};

static int sc485sl_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);
    *capArraySize = SC485SL_SPM_WORK_MODE_SIZE;
    return 0;
}

static int sc485sl_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != SC485SL_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("snr_config_num(%d) is not equal with work_mode size(%d)",
                   sensor_capability->snr_config_num, SC485SL_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = SC485SL_SPM_WORK_MODE_SIZE;
    for (i = 0; i < SC485SL_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case SC485SL_SPM_2688X1520_10BIT_90FPS_4LANE: {
                sensor_capability->snr_config[i].width = 2688;
                sensor_capability->snr_config[i].height = 1520;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 90;
                sensor_capability->snr_config[i].minFps = 30;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = SC485SL_SPM_2688X1520_10BIT_90FPS_4LANE;
                sensor_capability->snr_config[i].setting = &sc485sl_spm_setting;
            } break;
            case SC485SL_SPM_2688X1520_12BIT_90FPS_4LANE: {
                sensor_capability->snr_config[i].width = 2688;
                sensor_capability->snr_config[i].height = 1520;
                sensor_capability->snr_config[i].bitDepth = 12;
                sensor_capability->snr_config[i].maxFps = 90;
                sensor_capability->snr_config[i].minFps = 30;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = SC485SL_SPM_2688X1520_12BIT_90FPS_4LANE;
                sensor_capability->snr_config[i].setting = &sc485sl_spm_setting;
            } break;
            default: {
                CLOG_ERROR("invalid work mode (%d) for max workmode (%d)", i,
                           SC485SL_SPM_WORK_MODE_SIZE);
            } break;
        }
    }
    return 0;
}

static int sc485sl_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    switch (work_mode) {
        case SC485SL_SPM_2688X1520_10BIT_90FPS_4LANE: {
            snr_info->linetime = SC485SL_LINETIME_2688X1520_90FPS;  // ns
            snr_info->vts = SC485SL_VTS_2688X1520_90FPS;
            snr_info->f32maxFps = 90;
            snr_info->exp_time[0] = (snr_info->vts - 8) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = sc485sl_spm_2688x1520_10bit_90fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(sc485sl_spm_2688x1520_10bit_90fps_tab);
            snr_info->mipi_clock = 1080;  // Mhz
        } break;
        case SC485SL_SPM_2688X1520_12BIT_90FPS_4LANE: {
            snr_info->linetime = SC485SL_LINETIME_2688X1520_90FPS;  // ns
            snr_info->vts = SC485SL_VTS_2688X1520_90FPS;
            snr_info->f32maxFps = 90;
            snr_info->exp_time[0] = (snr_info->vts - 8) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = sc485sl_spm_2688x1520_12bit_90fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(sc485sl_spm_2688x1520_12bit_90fps_tab);
            snr_info->mipi_clock = 1296;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       SC485SL_SPM_WORK_MODE_SIZE);
        } break;
    }

    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &sc485sl_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &sc485sl_spm_setting.ispVideoSetting;
    return 0;
}

static int sc485sl_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    static const CapabilityElement config[] = {
        {"capSensorPhysicalSize", "4.7, 3.5"},
        {"capLensFocalLength", "3.46"},
        {"capLensOpticalStabMode", "off"},
        {"capLensFocusDistanceCalibration", "uncalibrated"},
    };
    if (devId != 0)
        return -EINVAL;
    dev_info->deviceConfig = config;
    dev_info->deviceConfigCnt = ARRAY_SIZE(config);
    return 0;
}

static int sc485sl_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);
    vendor_id->id_table = sc485sl_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(sc485sl_spm_vendor_id);
    return 0;
}

static int sc485sl_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);
    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S sc485sl_spm_Obj = {
    .name = "sc485sl_spm",
    .pfnGetsnrCapSize = sc485sl_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = sc485sl_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = sc485sl_spm_get_sensor_work_info,
    .pfnGetDevInfo = sc485sl_spm_get_device_info,
    .pfnGetSnrVendorId = sc485sl_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = sc485sl_spm_get_sensor_i2c_addr,
};
