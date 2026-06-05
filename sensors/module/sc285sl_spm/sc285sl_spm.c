/*
 * sc285sl_spm.c
 * Module wrapper providing work modes and i2c addr for SC285SL (mirrors sc533hai_spm.c)
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sc285sl_spm.h"
#include "sc285sl_spm_setting.h"

static const uint8_t module_i2c_addr = 0x30;  // 7-bit i2c address (0x60 in ini as 8-bit)

static struct regval_tab sc285sl_spm_vendor_id[] = {
	{0x3107, 0xBB},
	{0x3108, 0x98},
};

static SENSOR_MODULE_SETTING_S sc285sl_spm_setting = {
    .ispPreviewSetting =
        {
#include "sc285sl_spm_rear_primary_isp_setting.h"
        },
    .cppPreviewSetting =
        {
#include "sc285sl_spm_rear_primary_cpp_preview_setting.h"
        },
};

const CapabilityElement sc285slBackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.46"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

static int sc285sl_spm_get_sensor_cap_size(int32_t* capArraySize) {
    SENSORS_CHECK_PARA_POINTER(capArraySize);
    *capArraySize = SC285SL_SPM_WORK_MODE_SIZE;
    return 0;
}

static int sc285sl_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability) {
    int i;
    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);

    if (capArraySize != SC285SL_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("capArraySize(%d) != work_mode_size(%d)", capArraySize, SC285SL_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = SC285SL_SPM_WORK_MODE_SIZE;
    for (i = 0; i < SC285SL_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case SC285SL_SPM_1920x1080_10bit_90fps_4LANE: {
                sensor_capability->snr_config[i].width = 1920;
                sensor_capability->snr_config[i].height = 1080;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 90;
                sensor_capability->snr_config[i].minFps = 30;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = SC285SL_SPM_1920x1080_10bit_90fps_4LANE;
                sensor_capability->snr_config[i].setting = NULL; /* driver uses global table in sensor file */
            } break;

            default:
                CLOG_ERROR("invalid work_mode(%d), max=%d", i, SC285SL_SPM_WORK_MODE_SIZE);
                break;
        }
    }
    return 0;
}

static int sc285sl_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info) {
    SENSORS_CHECK_PARA_POINTER(snr_info);

    snr_info->image_mode = SENSOR_LINEAR_MODE;

    switch (work_mode) {
        case SC285SL_SPM_1920x1080_10bit_90fps_4LANE: {
            /* Values derived from ini header comment: VTS=1196, Tline ~9.283951us */
            snr_info->linetime = 9284;
            snr_info->vts = 1196;
            snr_info->f32maxFps = 90;
            snr_info->exp_time[0] = (snr_info->vts - 8) * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;
            snr_info->dgain[0] = 1 * 0x1000;
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = sc285sl_spm_1920x1080_10bit_90fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(sc285sl_spm_1920x1080_10bit_90fps_tab);
            snr_info->mipi_clock = 560;
        } break;

        default:{
            CLOG_ERROR("invalid work_mode(%d), max=%d", work_mode, SC285SL_SPM_WORK_MODE_SIZE);
        } break;
    }

    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &sc285sl_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &sc285sl_spm_setting.ispVideoSetting;
    return 0;
}

static int sc285sl_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info) {
    switch (devId) {
        case 0:
            dev_info->deviceConfig = sc285slBackCapCfg;
            dev_info->deviceConfigCnt = sizeof(sc285slBackCapCfg) / sizeof(sc285slBackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("invalid devId(%d), only support devId=0", devId);
            break;
    }
    return 0;
}

static int sc285sl_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id) {
    SENSORS_CHECK_PARA_POINTER(vendor_id);
    vendor_id->id_table = sc285sl_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(sc285sl_spm_vendor_id);
    return 0;
}

static int sc285sl_spm_get_sensor_i2c_addr(uint8_t* i2c_addr) {
    SENSORS_CHECK_PARA_POINTER(i2c_addr);
    *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S sc285sl_spm_Obj = {
    .name = "sc285sl_spm",
    .pfnGetsnrCapSize = sc285sl_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = sc285sl_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = sc285sl_spm_get_sensor_work_info,
    .pfnGetDevInfo = sc285sl_spm_get_device_info,
    .pfnGetSnrVendorId = sc285sl_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = sc285sl_spm_get_sensor_i2c_addr,
};


