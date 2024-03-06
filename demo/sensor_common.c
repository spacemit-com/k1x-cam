/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include "sensor_common.h"

#include <string.h>

#include "cam_log.h"

typedef struct {
    void* sensors_handle;
    SENSORS_MODULE_INFO_S sensors_module_info;
    SENSORS_MODULE_CAPABILITY_S sensors_cap;
    ISP_SENSOR_REGISTER_S sensor_ops;
    int workMode;
    int devId;
} SENSOR_MODULE_HANDLE;

int testSensorInit(void** ppHandle, const char* sensors_name, int devId, int work_mode)
{
    int ret = 0;
    int i = 0;
    SENSOR_MODULE_HANDLE* handle = NULL;

    handle = (SENSOR_MODULE_HANDLE*)calloc(1, sizeof(SENSOR_MODULE_HANDLE));
    if (!handle) {
        CLOG_ERROR("SENSOR_MODULE_HANDLE handle malloc fail");
        return -1;
    }

    ret = SPM_SENSORS_MODULE_Detect(sensors_name, devId);
    if (ret) {
        CLOG_ERROR("detect sensor %s devId %d fail", sensors_name, devId);
        return ret;
    }

    ret = SPM_SENSORS_MODULE_Init(&handle->sensors_handle, sensors_name, devId, &handle->sensors_module_info);
    if (ret) {
        CLOG_ERROR("sensors module %s devId %d init fail", sensors_name, devId);
        return ret;
    }
    handle->sensors_cap.sensor_capability.snr_config_num = handle->sensors_module_info.snr_config_num;
    if (handle->sensors_cap.sensor_capability.snr_config_num != 0) {
        handle->sensors_cap.sensor_capability.snr_config =
            (SENSOR_CONFIG_S*)calloc(1, handle->sensors_cap.sensor_capability.snr_config_num * sizeof(SENSOR_CONFIG_S));
        if (NULL == handle->sensors_cap.sensor_capability.snr_config) {
            CLOG_ERROR("%s: sensors_cap.sensor_capability.snr_config malloc memory failed!", __FUNCTION__);
            return -2;
        }
    } else {
        CLOG_INFO("sensors module has no sensor");
    }

    ret = SPM_SENSORS_MODULE_EnumCapability(handle->sensors_handle, &handle->sensors_cap);
    if (ret == 0) {
        CLOG_INFO("sensor config info number %d", handle->sensors_cap.sensor_capability.snr_config_num);
        for (i = 0; i < handle->sensors_cap.sensor_capability.snr_config_num; i++) {
            CLOG_INFO("sensor_config_info[%d].width %d", i, handle->sensors_cap.sensor_capability.snr_config[i].width);
            CLOG_INFO("sensor_config_info[%d].height %d", i,
                      handle->sensors_cap.sensor_capability.snr_config[i].height);
            CLOG_INFO("sensor_config_info[%d].bitDepth %d", i,
                      handle->sensors_cap.sensor_capability.snr_config[i].bitDepth);
            CLOG_INFO("sensor_config_info[%d].fps %f", i, handle->sensors_cap.sensor_capability.snr_config[i].maxFps);
            CLOG_INFO("sensor_config_info[%d].image_mode %d", i,
                      handle->sensors_cap.sensor_capability.snr_config[i].image_mode);
            CLOG_INFO("sensor_config_info[%d].lane_num %d", i,
                      handle->sensors_cap.sensor_capability.snr_config[i].lane_num);
            CLOG_INFO("sensor_config_info[%d].work_mode %d", i,
                      handle->sensors_cap.sensor_capability.snr_config[i].work_mode);
        }
    } else {
        return -3;
    }

    ret = SPM_SENSOR_Open(handle->sensors_handle);
    if (ret) {
        CLOG_ERROR("%s: open sensor failed!", __FUNCTION__);
        return -4;
    }

    ret = SPM_SENSOR_GetOps(handle->sensors_handle, &handle->sensor_ops);
    if (ret) {
        CLOG_ERROR("%s: get ops failed!", __FUNCTION__);
        return -5;
    }

    handle->workMode = work_mode;
    SPM_SENSOR_Config(handle->sensors_handle, handle->workMode);
    handle->devId = devId;

    *ppHandle = handle;
    return 0;
}

int testSensorDeInit(void* phandle)
{
    SENSOR_MODULE_HANDLE* handle = (SENSOR_MODULE_HANDLE*)phandle;
    if (!handle) {
        CLOG_ERROR("invalid input para handle %p", handle);
        return -1;
    }

    SPM_SENSOR_Close(handle->sensors_handle);
    SPM_SENSORS_MODULE_Deinit(handle->sensors_handle);
    free(handle);

    return 0;
}

int testSensorStart(void* phandle)
{
    SENSOR_MODULE_HANDLE* handle = (SENSOR_MODULE_HANDLE*)phandle;
    if (!handle) {
        CLOG_ERROR("invalid input para handle %p", handle);
        return -1;
    }
    SPM_SENSOR_StreamOn(handle->sensors_handle);

    return 0;
}

int testSensorStop(void* phandle)
{
    SENSOR_MODULE_HANDLE* handle = (SENSOR_MODULE_HANDLE*)phandle;
    if (!handle) {
        CLOG_ERROR("invalid input para handle %p", handle);
        return -1;
    }
    SPM_SENSOR_StreamOff(handle->sensors_handle);

    return 0;
}

int testSensorGetDevInfo(void* phandle, SENSOR_MODULE_INFO* sensorInfo)
{
    SENSOR_MODULE_HANDLE* handle = (SENSOR_MODULE_HANDLE*)phandle;
    if (!handle || !sensorInfo) {
        CLOG_ERROR("invalid input para handle %p, sensorInfo %p", handle, sensorInfo);
        return -1;
    }

    sensorInfo->sensorId = handle->devId;
    sensorInfo->pSensorOps = &handle->sensor_ops;
    sensorInfo->sensor_cfg = &handle->sensors_cap.sensor_capability.snr_config[handle->workMode];

    return 0;
}
