/*
 * Copyright (C) 2022 SPM Micro Limited
 * All Rights Reserved.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "spm_cam_sensors.h"
#include "spm_comm_sensors.h"
#include "spm_isp_sensor_comm.h"
#include "cam_log.h"

int main(int argc, char* argv[])
{
    int ret = 0;
    int devId = 0;
    char* sensors_name = NULL;
    void* sensors_handle = NULL;
    SENSORS_MODULE_INFO_S sensors_module_info;
    SENSORS_MODULE_CAPABILITY_S sensors_cap;
    int i = 0;
    ISP_SENSOR_REGISTER_S sensor_ops;

    if (argc == 3) {
        devId = atoi(argv[1]);
        sensors_name = argv[2];
    } else {
        CLOG_ERROR("cam_sensors_test [devId] [sensors_name]");
        return -1;
    }

    ret = SPM_SENSORS_MODULE_Detect(sensors_name, devId);
    if (ret) {
        CLOG_ERROR("detect sensor fail");
        return ret;
    }

    ret = SPM_SENSORS_MODULE_Init(&sensors_handle, sensors_name, devId, &sensors_module_info);
    if (ret) {
        CLOG_ERROR("sensors module init fail");
        return ret;
    }
    sensors_cap.sensor_capability.snr_config_num = sensors_module_info.snr_config_num;
    if (sensors_cap.sensor_capability.snr_config_num != 0) {
        sensors_cap.sensor_capability.snr_config =
            (SENSOR_CONFIG_S*)calloc(1, sensors_cap.sensor_capability.snr_config_num * sizeof(SENSOR_CONFIG_S));
        if (NULL == sensors_cap.sensor_capability.snr_config) {
            CLOG_ERROR("%s: sensors_cap.sensor_capability.snr_config malloc memory failed!", __FUNCTION__);
            return -ENOMEM;
        }
    } else {
        CLOG_INFO("sensors module has no sensor");
    }

    ret = SPM_SENSORS_MODULE_EnumCapability(sensors_handle, &sensors_cap);
    if (ret == 0) {
        CLOG_INFO("sensor config info number %d", sensors_cap.sensor_capability.snr_config_num);
        for (i = 0; i < sensors_cap.sensor_capability.snr_config_num; i++) {
            CLOG_INFO("sensor_config_info[%d].width %d", i, sensors_cap.sensor_capability.snr_config[i].width);
            CLOG_INFO("sensor_config_info[%d].height %d", i, sensors_cap.sensor_capability.snr_config[i].height);
            CLOG_INFO("sensor_config_info[%d].bitDepth %d", i, sensors_cap.sensor_capability.snr_config[i].bitDepth);
            CLOG_INFO("sensor_config_info[%d].fps %d", i, sensors_cap.sensor_capability.snr_config[i].maxFps);
            CLOG_INFO("sensor_config_info[%d].image_mode %d", i,
                      sensors_cap.sensor_capability.snr_config[i].image_mode);
            CLOG_INFO("sensor_config_info[%d].lane_num %d", i, sensors_cap.sensor_capability.snr_config[i].lane_num);
            CLOG_INFO("sensor_config_info[%d].work_mode %d", i, sensors_cap.sensor_capability.snr_config[i].work_mode);
        }
    } else {
        goto out;
    }

    ret = SPM_SENSOR_Open(sensors_handle);
    if (ret) {
        goto out;
    }

    ret = SPM_SENSOR_GetOps(sensors_handle, &sensor_ops);
    if (ret) {
        goto out_sensor;
    }

    while (1) {
        char ch;
        CLOG_INFO("Input a character:");
        ch = getc(stdin);
        if (ch == 'q' || ch == 'Q') {
            CLOG_INFO("enter q exit");
            break;
        }
        if (ch <= '9' && ch >= '0') {
            int work_mode = ch - '0';
            SPM_SENSOR_Config(sensors_handle, work_mode);
            CLOG_INFO("config sensor work mode %d", work_mode);
            continue;
        }
        if (ch == 's' || ch == 'S') {
            SPM_SENSOR_StreamOn(sensors_handle);
            CLOG_INFO("sensor stream on");
            continue;
        }
        if (ch == 'c' || ch == 'C') {
            SPM_SENSOR_StreamOff(sensors_handle);
            CLOG_INFO("sensor stream off");
            continue;
        }
    }
out_sensor:
    SPM_SENSOR_Close(sensors_handle);

out:
    SPM_SENSORS_MODULE_Deinit(sensors_handle);

    return ret;
}
