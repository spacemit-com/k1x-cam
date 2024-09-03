/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "gc13a0_spm.h"
#include "gc13a0_spm_setting.h"

//0x10, 0x11, 0x12
//static const uint8_t gc13a0_module_i2c_addr = 0x39; /* I2C Address 7-bit*/
static const uint8_t gc13a0_module_i2c_addr = 0x39; /* I2C Address 7-bit*/

struct regval_tab gc13a0_spm_vendor_id[] = {
    {0x03f0, 0x13},
    {0x03f1, 0xa0},
};

static SENSOR_MODULE_SETTING_S gc13a0_spm_setting = {
    .ispPreviewSetting =
        {
#include "gc13a0_spm_rear_secondary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "gc13a0_spm_rear_secondary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "gc13a0_spm_rear_secondary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "gc13a0_spm_rear_secondary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "gc13a0_spm_rear_secondary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "gc13a0_spm_rear_secondary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "gc13a0_spm_rear_secondary_nightshot_setting.h"
        },
};

const CapabilityElement gc13a0BackCapCfg[] = {
    {"capSensorPhysicalSize", "2.8, 2.1"},
    {"capLensFocalLength", "2.4"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", ""},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.6"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int gc13a0_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = GC13A0_SPM_WORK_MODE_SIZE;
    return 0;
}

static int gc13a0_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != GC13A0_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, GC13A0_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = GC13A0_SPM_WORK_MODE_SIZE;
    for (i = 0; i < GC13A0_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case GC13A0_SPM_4208x3120_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 4208;
                sensor_capability->snr_config[i].height = 3120;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 15;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_RGGB;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = GC13A0_SPM_4208x3120_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &gc13a0_spm_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           GC13A0_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int gc13a0_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = gc13a0_module_i2c_addr;
    // snr_info->id_table = gc13a0_spm_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(gc13a0_spm_vendor_id);

    switch (work_mode) {
        case GC13A0_SPM_4208x3120_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = GC13A0_SPM_4208x3120_10bit_LINEAR_30_4LANE;  // ns
            snr_info->vts = GC13A0_VMAX_13M30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 3232 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = gc13a0_spm_4208x3120_10bit_30fps_4lane_tab;
            snr_info->setting_table_size = ARRAY_SIZE(gc13a0_spm_4208x3120_10bit_30fps_4lane_tab);
            snr_info->mipi_clock = 1420;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       GC13A0_SPM_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &gc13a0_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &gc13a0_spm_setting.ispVideoSetting;

    return 0;
}

static int gc13a0_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 1:  // back aux
            dev_info->deviceConfig = gc13a0BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(gc13a0BackCapCfg) / sizeof(gc13a0BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int gc13a0_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = gc13a0_module_i2c_addr;
    vendor_id->id_table = gc13a0_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(gc13a0_spm_vendor_id);

    return 0;
}

static int gc13a0_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);
	printf("I2C_ADDRESS = %x\n", gc13a0_module_i2c_addr);

    *i2c_addr = gc13a0_module_i2c_addr;
    return 0;
}

MODULE_OBJ_S gc13a0_spm_Obj = {
    .name = "gc13a0_spm",
    .pfnGetsnrCapSize = gc13a0_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = gc13a0_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = gc13a0_spm_get_sensor_work_info,
    .pfnGetDevInfo = gc13a0_spm_get_device_info,
    .pfnGetSnrVendorId = gc13a0_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = gc13a0_spm_get_sensor_i2c_addr,
};
