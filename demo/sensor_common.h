/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _SENSOR_COMMON_H_
#define _SENSOR_COMMON_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#include "asr_cam_sensors.h"
#include "asr_comm_sensors.h"
#include "asr_isp_sensor_comm.h"

typedef struct {
    int sensorId;
    ISP_SENSOR_REGISTER_S* pSensorOps;
    SENSOR_CONFIG_S* sensor_cfg;
} SENSOR_MODULE_INFO;

int testSensorInit(void** ppHandle, const char* sensors_name, int devId, int work_mode);
int testSensorDeInit(void* phandle);
int testSensorStart(void* phandle);
int testSensorStop(void* phandle);
int testSensorGetDevInfo(void* phandle, SENSOR_MODULE_INFO* sensorInfo);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_LIST_H_ */
