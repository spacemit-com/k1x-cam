/*
 * Copyright (C) 2024
 * All Rights Reserved.
 *
 * RN6752V1 video decoder module implementation
 */
#include "rn6752_spm.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "rn6752_spm_setting.h"

/* RN6752V1 I2C address (7-bit): 0x2C */
static const uint8_t module_i2c_addr = 0x2c;

/* Vendor ID registers: chip ID high byte and low byte */
struct regval_tab rn6752_spm_vendor_id[] = {
    {0xFE, 0x26}, /* CHIP_ID_H expected value */
    {0xFD, 0x01}, /* CHIP_ID_L expected value */
};

static SENSOR_MODULE_SETTING_S rn6752_spm_setting = {
    .ispPreviewSetting =
        {
#include "rn6752_spm_rear_primary_isp_setting.h"
        },
    .ispVideoSetting =
        {
#include "rn6752_spm_rear_primary_isp_setting_video.h"
        },
    .cppPreviewSetting =
        {
#include "rn6752_spm_rear_primary_cpp_preview_setting.h"
        },
    .cppVideoSetting =
        {
#include "rn6752_spm_rear_primary_cpp_video_setting.h"
        },
    .cppSnapshotSetting =
        {
#include "rn6752_spm_rear_primary_cpp_snapshot_setting.h"
        },
    .cppNightshotSetting =
        {
#include "rn6752_spm_rear_primary_cpp_nightshot_setting.h"
        },
    .nightshotSetting =
        {
#include "rn6752_spm_rear_primary_nightshot_setting.h"
        },
};

const CapabilityElement rn6752BackCapCfg[] = {
    {"capSensorPhysicalSize", "6.4, 4.8"},
    {"capLensFocalLength", "3.6"},
    {"capLensOpticalStabMode", "off"},
    {"capLensMinFocusDistanc", "10"},
    {"capLensFocusDistanceCalibration", "uncalibrated"},
    {"capLensApertures", "2.0"},
    {"capLensFilterDensities", "0"},
};

/********************************************************************/
static int rn6752_spm_get_sensor_cap_size(int32_t *capArraySize) {
  SENSORS_CHECK_PARA_POINTER(capArraySize);

  *capArraySize = RN6752_SPM_WORK_MODE_SIZE;
  return 0;
}

static int
rn6752_spm_get_sensor_capbility(int32_t capArraySize,
                                SENSOR_CAPABILITY_S *sensor_capability) {
  int i;

  SENSORS_CHECK_PARA_POINTER(sensor_capability);
  SENSORS_CHECK_PARA_POINTER(sensor_capability->snr_config);
  if (capArraySize != RN6752_SPM_WORK_MODE_SIZE) {
    CLOG_ERROR("%s: snr_config_num(%d) is not equal with work_mode size(%d)",
               __FUNCTION__, sensor_capability->snr_config_num,
               RN6752_SPM_WORK_MODE_SIZE);
    return -EINVAL;
  }

  sensor_capability->snr_config_num = RN6752_SPM_WORK_MODE_SIZE;
  for (i = 0; i < RN6752_SPM_WORK_MODE_SIZE; i++) {
    switch (i) {
    case RN6752_SPM_1920x1080_8bit_LINEAR_25_4LANE: {
      sensor_capability->snr_config[i].width = 1920;
      sensor_capability->snr_config[i].height = 1080;
      sensor_capability->snr_config[i].bitDepth = 8;
      sensor_capability->snr_config[i].maxFps = 25;
      sensor_capability->snr_config[i].minFps = 25;
      sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
      sensor_capability->snr_config[i].lane_num = 4;
      sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
      sensor_capability->snr_config[i].supportPDAF = 0;
      sensor_capability->snr_config[i].work_mode =
          RN6752_SPM_1920x1080_8bit_LINEAR_25_4LANE;
      sensor_capability->snr_config[i].setting = &rn6752_spm_setting;
    } break;
    case RN6752_SPM_1280x720_8bit_LINEAR_25_4LANE: {
      sensor_capability->snr_config[i].width = 1280;
      sensor_capability->snr_config[i].height = 720;
      sensor_capability->snr_config[i].bitDepth = 8;
      sensor_capability->snr_config[i].maxFps = 25;
      sensor_capability->snr_config[i].minFps = 25;
      sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
      sensor_capability->snr_config[i].lane_num = 4;
      sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
      sensor_capability->snr_config[i].supportPDAF = 0;
      sensor_capability->snr_config[i].work_mode =
          RN6752_SPM_1280x720_8bit_LINEAR_25_4LANE;
      sensor_capability->snr_config[i].setting = &rn6752_spm_setting;
    } break;
    case RN6752_SPM_1280x720_8bit_LINEAR_30_4LANE: {
      sensor_capability->snr_config[i].width = 1280;
      sensor_capability->snr_config[i].height = 720;
      sensor_capability->snr_config[i].bitDepth = 8;
      sensor_capability->snr_config[i].maxFps = 30;
      sensor_capability->snr_config[i].minFps = 30;
      sensor_capability->snr_config[i].image_mode = SENSOR_LINEAR_MODE;
      sensor_capability->snr_config[i].lane_num = 4;
      sensor_capability->snr_config[i].pattern = ISP_BAYER_PATTERN_BGGR;
      sensor_capability->snr_config[i].supportPDAF = 0;
      sensor_capability->snr_config[i].work_mode =
          RN6752_SPM_1280x720_8bit_LINEAR_30_4LANE;
      sensor_capability->snr_config[i].setting = &rn6752_spm_setting;
    } break;
    default: {
      CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)",
                 __FUNCTION__, i, RN6752_SPM_WORK_MODE_SIZE);
    } break;
    }
  }

  return 0;
}

static int rn6752_spm_get_sensor_work_info(int32_t work_mode,
                                           SENSOR_WORK_INFO_S *snr_info) {
  SENSORS_CHECK_PARA_POINTER(snr_info);

  switch (work_mode) {
  case RN6752_SPM_1920x1080_8bit_LINEAR_25_4LANE: {
    snr_info->linetime = RN6752_LINETIME_1080P25_8bit_LINEAR; /* ns */
    snr_info->vts = RN6752_VMAX_1080P25_8bit_LINEAR;
    snr_info->f32maxFps = 25;
    snr_info->exp_time[0] = snr_info->vts * snr_info->linetime / 1000;
    snr_info->again[0] = 1 * 0x100;  /* Q8 format, fixed at 1x */
    snr_info->dgain[0] = 1 * 0x1000; /* Q12 format, fixed at 1x */
    snr_info->image_mode = SENSOR_LINEAR_MODE;
    snr_info->setting_table = rn6752_spm_1080p25_4lane_tab;
    snr_info->setting_table_size = ARRAY_SIZE(rn6752_spm_1080p25_4lane_tab);
    snr_info->mipi_clock = 648; /* MHz - for 1080p25 4-lane UYVY */
  } break;
  case RN6752_SPM_1280x720_8bit_LINEAR_25_4LANE: {
    snr_info->linetime = RN6752_LINETIME_720P25_8bit_LINEAR;
    snr_info->vts = RN6752_VMAX_720P25_8bit_LINEAR;
    snr_info->f32maxFps = 25;
    snr_info->exp_time[0] = snr_info->vts * snr_info->linetime / 1000;
    snr_info->again[0] = 1 * 0x100;
    snr_info->dgain[0] = 1 * 0x1000;
    snr_info->image_mode = SENSOR_LINEAR_MODE;
    snr_info->setting_table = rn6752_spm_720p25_4lane_tab;
    snr_info->setting_table_size = ARRAY_SIZE(rn6752_spm_720p25_4lane_tab);
    snr_info->mipi_clock = 648; /* MHz - for 720p25 4-lane UYVY */
  } break;
  case RN6752_SPM_1280x720_8bit_LINEAR_30_4LANE: {
    snr_info->linetime = RN6752_LINETIME_720P30_8bit_LINEAR;
    snr_info->vts = RN6752_VMAX_720P30_8bit_LINEAR;
    snr_info->f32maxFps = 30;
    snr_info->exp_time[0] = snr_info->vts * snr_info->linetime / 1000;
    snr_info->again[0] = 1 * 0x100;
    snr_info->dgain[0] = 1 * 0x1000;
    snr_info->image_mode = SENSOR_LINEAR_MODE;
    snr_info->setting_table = rn6752_spm_720p30_4lane_tab;
    snr_info->setting_table_size = ARRAY_SIZE(rn6752_spm_720p30_4lane_tab);
    snr_info->mipi_clock = 648; /* MHz - for 720p30 4-lane UYVY */
  } break;
  default: {
    CLOG_ERROR("%s: invalid work mode (%d) for max workmode (%d)", __FUNCTION__,
               work_mode, RN6752_SPM_WORK_MODE_SIZE);
  } break;
  }
  snr_info->work_mode = work_mode;
  snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_PREVIEW] =
      &rn6752_spm_setting.ispPreviewSetting;
  snr_info->pstIspDefaultSettings[CAM_ISP_SCENE_VIDEO] =
      &rn6752_spm_setting.ispVideoSetting;

  return 0;
}

static int rn6752_spm_get_device_info(int devId, CapDeviceCapInfo *dev_info) {
  switch (devId) {
  case 0: /* back */
    dev_info->deviceConfig = rn6752BackCapCfg;
    dev_info->deviceConfigCnt =
        sizeof(rn6752BackCapCfg) / sizeof(rn6752BackCapCfg[0]);
    break;
  default:
    CLOG_ERROR("%s: find no invalid device info for device %d", __FUNCTION__,
               devId);
    break;
  }
  return 0;
}

static int rn6752_spm_get_sensor_vendor_id(SENSOR_VENDOR_ID_S *vendor_id) {
  SENSORS_CHECK_PARA_POINTER(vendor_id);

  vendor_id->id_table = rn6752_spm_vendor_id;
  vendor_id->id_table_size = ARRAY_SIZE(rn6752_spm_vendor_id);

  return 0;
}

static int rn6752_spm_get_sensor_i2c_addr(uint8_t *i2c_addr) {
  SENSORS_CHECK_PARA_POINTER(i2c_addr);

  *i2c_addr = module_i2c_addr;

  return 0;
}

MODULE_OBJ_S rn6752_spm_Obj = {
    .name = "rn6752_spm",
    .pfnGetsnrCapSize = rn6752_spm_get_sensor_cap_size,
    .pfnGetsnrCapbility = rn6752_spm_get_sensor_capbility,
    .pfnGetSnrWorkInfo = rn6752_spm_get_sensor_work_info,
    .pfnGetDevInfo = rn6752_spm_get_device_info,
    .pfnGetSnrVendorId = rn6752_spm_get_sensor_vendor_id,
    .pfnGetSnrI2cAddr = rn6752_spm_get_sensor_i2c_addr,
};
