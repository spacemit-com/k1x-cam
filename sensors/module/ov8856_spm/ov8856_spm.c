/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include "ov8856_spm.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "ov8856_spm_setting.h"

static const uint8_t module_i2c_addr = 0x36; /* I2C Address 7-bit, SID LOW */
//static const uint8_t module_i2c_addr = 0x10; /* I2C Address 7-bit, SID HIGH */

struct regval_tab ov8856_spm_vendor_id[] = {
    {0x3004, 0x6c},
};

static SENSOR_MODULE_SETTING_S ov8856_spm_setting = {
    .ispPreviewSetting =
        {
#include "ov8856_spm_rear_primary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "ov8856_spm_rear_primary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "ov8856_spm_rear_primary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "ov8856_spm_rear_primary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "ov8856_spm_rear_primary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "ov8856_spm_rear_primary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "ov8856_spm_rear_primary_nightshot_setting.h"
        },
};

const CapabilityElement ov8856BackCapCfg[] = {
    {"capSensorPhysicalSize", "4.7, 3.5"},
    {"capLensFocalLength", "3.64"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.2"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int ov8856_spm_get_sensor_cap_size(int32_t* capArraySize)
{
    SENSORS_CHECK_PARA_POINTER(capArraySize);

    *capArraySize = OV8856_SPM_WORK_MODE_SIZE;
    return 0;
}

static int ov8856_spm_get_sensor_capbility(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability)
{
    int i;

    SENSORS_CHECK_PARA_POINTER(sensor_capability);
    SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
    if (capArraySize != OV8856_SPM_WORK_MODE_SIZE) {
        CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)", __FUNCTION__,
                   sensor_capability->snr_config_num, OV8856_SPM_WORK_MODE_SIZE);
        return -EINVAL;
    }

    sensor_capability->snr_config_num = OV8856_SPM_WORK_MODE_SIZE;
    for (i = 0; i < OV8856_SPM_WORK_MODE_SIZE; i++) {
        switch (i) {
            case OV8856_SPM_2560x1440_10bit_LINEAR_30_4LANE: {
                sensor_capability->snr_config[i].width = 2560;
                sensor_capability->snr_config[i].height = 1440;
                sensor_capability->snr_config[i].bitDepth = 10;
                sensor_capability->snr_config[i].maxFps = 30;
                sensor_capability->snr_config[i].minFps = 25;
                sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
                sensor_capability->snr_config[i].lane_num = 4;
                sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
                sensor_capability->snr_config[i].supportPDAF = 0;
                sensor_capability->snr_config[i].work_mode = OV8856_SPM_2560x1440_10bit_LINEAR_30_4LANE;
                sensor_capability->snr_config[i].setting = &ov8856_spm_setting;
            } break;
            default: {
                CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, i,
                           OV8856_SPM_WORK_MODE_SIZE);
            } break;
        }
    }

    return 0;
}

static int ov8856_spm_get_sensor_work_info(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info)
{
    SENSORS_CHECK_PARA_POINTER(snr_info);

    // snr_info->i2c_addr = module_i2c_addr;
    // snr_info->id_table = ov8856_spm_vendor_id;
    // snr_info->id_table_size = ARRAY_SIZE(ov8856_spm_vendor_id);

    switch (work_mode) {
        case OV8856_SPM_2560x1440_10bit_LINEAR_30_4LANE: {
            snr_info->linetime = OV8856_LINETIME_2K30_10bit_LINEAR;  // ns
            snr_info->vts = OV8856_VMAX_2K30_10bit_LINEAR;
            snr_info->f32maxFps = 30;
            snr_info->exp_time[0] = 2482 * snr_info->linetime / 1000;
            snr_info->again[0] = 1 * 0x100;   // Q8 format
            snr_info->dgain[0] = 1 * 0x1000;  // Q12 format
            snr_info->image_mode = SENSOR_LINEAR_MODE;
            snr_info->setting_table = ov8856_spm_2560x1440_10bit_30fps_tab;
            snr_info->setting_table_size = ARRAY_SIZE(ov8856_spm_2560x1440_10bit_30fps_tab);
            snr_info->mipi_clock = 360;  // Mhz
        } break;
        default: {
            CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__, work_mode,
                       OV8856_SPM_WORK_MODE_SIZE);
        } break;
    }
    snr_info->work_mode = work_mode;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] = &ov8856_spm_setting.ispPreviewSetting;
    snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] = &ov8856_spm_setting.ispVideoSetting;

    return 0;
}

static int ov8856_spm_get_device_info(int devId, CapDeviceCapInfo* dev_info)
{
    switch (devId) {
        case 0:  // back
            dev_info->deviceConfig = ov8856BackCapCfg;
            dev_info->deviceConfigCnt = sizeof(ov8856BackCapCfg) / sizeof(ov8856BackCapCfg[0]);
            break;
        default:
            CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__, devId);
            break;
    }
    return 0;
}

static int ov8856_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSORS_CHECK_PARA_POINTER(vendor_id);

    // vendor_id->i2c_addr = module_i2c_addr;
    vendor_id->id_table = ov8856_spm_vendor_id;
    vendor_id->id_table_size = ARRAY_SIZE(ov8856_spm_vendor_id);

    return 0;
}

static int readenv_atoi(char *env) {
  char *p;
  //getenv函数是gcc自带的函数，在stdlib.h头文件中声明
  if (( p = getenv(env) ))
  	return (atoi(p));
  else
	return(0);
}
static int ov8856_spm_get_sensor_i2c_addr(uint8_t* i2c_addr)
{
    SENSORS_CHECK_PARA_POINTER(i2c_addr);

	int addr = readenv_atoi("I2C_ADDRESS");
	printf("I2C_ADDRESS = %d\n",addr);

    *i2c_addr = module_i2c_addr;

    if (addr != 0) {
        *i2c_addr = addr;
        printf("i2c_addr = 0x%x\n", addr);
    }

    // *i2c_addr = module_i2c_addr;
    return 0;
}

MODULE_OBJ_S ov8856_spm_Obj = {
    .name = "ov8856_spm",
    .pfnGetsnrCapSize = ov8856_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = ov8856_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = ov8856_spm_get_sensor_work_info,
    .pfnGetDevInfo = ov8856_spm_get_device_info,
    .pfnGetSnrVendorId = ov8856_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = ov8856_spm_get_sensor_i2c_addr,
};
