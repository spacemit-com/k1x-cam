/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _SENSOR_COMMON_H_
#define _SENSOR_COMMON_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#include "spm_cam_sensors.h"
#include "spm_comm_sensors.h"
#include "spm_isp_sensor_comm.h"
#include "config.h"

typedef struct {
    int sensorId;
    ISP_SENSOR_REGISTER_S* pSensorOps;
    SENSOR_CONFIG_S* sensor_cfg;
    ISP_AF_MOTOR_REGISTER_S* pAfOps;
    int vcm_en;
} SENSOR_MODULE_INFO;

// typedef struct {
//     int snr_i2c_addr;

//     int flash_en;

//     int vcm_en;
//     int vcm_i2c_bus;
//     int vcm_i2c_addr;
// } SENSOR_CONFIG_INFO;

int testSensorInit(void** ppHandle, const char* sensors_name, int devId, int work_mode, struct testConfig *config);
int testSensorDeInit(void* phandle);
int testSensorStart(void* phandle);
int testSensorStop(void* phandle);
int testSensorGetDevInfo(void* phandle, SENSOR_MODULE_INFO* sensorInfo);
int testSensorAuxFlashMode(void* phandle, int mode);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_LIST_H_ */
