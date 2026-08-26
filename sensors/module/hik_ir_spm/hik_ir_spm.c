/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 */
#include "hik_ir_spm.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


/*
 * The public product manual does not provide a readable vendor-id register map.
 * Use an empty probe table for now and rely on runtime stream validation.
 */
static struct regval_tab hik_ir_spm_vendor_id[] = {
    {0x0000, 0x0000},
};

static SENSOR_MODULE_SETTING_S hik_ir_spm_setting = {
//     .ispPreviewSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_isp_setting.h"
//         },
//     .ispVideoSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_isp_setting_video.h"
//         },
//     .cppPreviewSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_cpp_preview_setting.h"
//         },
//     .cppVideoSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_cpp_video_setting.h"
//         },
//     .cppSnapshotSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_cpp_snapshot_setting.h"
//         },
//     .cppNightshotSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_cpp_nightshot_setting.h"
//         },
//     .nightshotSetting =
//         {
// #include "../rn6752_spm/rn6752_spm_rear_primary_nightshot_setting.h"
//         },
};

static const CapabilityElement hik_ir_back_cap_cfg[] = {
    {"capSensorPhysicalSize", "5.12, 4.10"},
    {"capLensFocalLength", "6.0"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "150"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "0.8"},
    {"capLensFilterDensities", "0"},
};

static int hik_ir_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = HIK_IR_SPM_WORK_MODE_SIZE;
    return 0;
}

static int hik_ir_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != HIK_IR_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, HIK_IR_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = HIK_IR_SPM_WORK_MODE_SIZE;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].width = 640;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].height = 512;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].bitDepth = 8;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].maxFps = 50;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].minFps = 50;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].image_mode = SENSOR_LINEAR_MODE;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].lane_num = 1;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].pattern = ISP_BAYER_PATTERN_MONO;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].supportPDAF = 0;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].work_mode = HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE;
    sensor_capability->snr_config[HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE].setting = &hik_ir_spm_setting;

    return 0;
}

static int hik_ir_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    switch (work_mode) {
        case HIK_IR_SPM_640x512_8bit_LINEAR_50_1LANE:
            snr_info->linetime = HIK_IR_LINETIME_512P50_8bit_LINEAR;
            snr_info->vts = HIK_IR_VMAX_512P50_8bit_LINEAR;
            snr_info->f32maxFps = 50;
            snr_info->exp_time[0] = snr_info->vts * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;
            snr_info->dgain[0] = 1 * 0x1000;
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = NULL;
            snr_info->setting_table_size = 0;
            snr_info->mipi_clock = 297;
            break;
        default:
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       HIK_IR_SPM_WORK_MODE_SIZE);
            return -EINVAL;
    }

    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &hik_ir_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &hik_ir_spm_setting.ispVideoSetting;
    return 0;
}

static int hik_ir_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:
            dev_info->deviceConfig = hik_ir_back_cap_cfg;
            dev_info->deviceConfigCnt = sizeof(hik_ir_back_cap_cfg) / sizeof(hik_ir_back_cap_cfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int hik_ir_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    vendor_id->id_table = hik_ir_spm_vendor_id;
    vendor_id->id_table_size = 0;
    return 0;
}

static int hik_ir_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

    /* The ABI field remains present, but this sensor never opens an I2C bus. */
    *i2c_addr = 0;
    return 0;
}

MODULE_OBJ_S hik_ir_spm_Obj = {
    .name = "hik_ir_uart_spm",
    .pfnGetsnrCapSize = hik_ir_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = hik_ir_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = hik_ir_spm_get_sensor_work_info,
    .pfnGetDevInfo = hik_ir_spm_get_device_info,
    .pfnGetSnrVendorId = hik_ir_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = hik_ir_spm_get_sensor_i2c_addr,
};
