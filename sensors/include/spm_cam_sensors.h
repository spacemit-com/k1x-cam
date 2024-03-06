/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _SPM_CAM_SENSORS_H_
#define _SPM_CAM_SENSORS_H_

#include "spm_comm_sensors.h"
#include "spm_isp_sensor_comm.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef struct _CapabilityElement {
    const char* description;
    const char* value;
} CapabilityElement;

typedef struct _CapDeviceCapInfo {
    const CapabilityElement* deviceConfig;
    int32_t deviceConfigCnt;
} CapDeviceCapInfo;

int SPM_SENSORS_MODULE_Detect(const char* name, int devId);
int SPM_SENSORS_MODULE_Init(void** pHandle, const char* name, int devId, SENSORS_MODULE_INFO_S* module_info);
int SPM_SENSORS_MODULE_Deinit(void* handle);
int SPM_SENSORS_MODULE_EnumCapability(void* handle, SENSORS_MODULE_CAPABILITY_S* cap);
int SPM_GetDeviceInfo(void* handle, void* devInfo);


/*sensor*/
int SPM_SENSOR_Open(void* handle);
int SPM_SENSOR_Close(void* handle);
int SPM_SENSOR_Config(void* handle, int32_t work_mode);
int SPM_SENSOR_SetParam(void* handle, const SENSOR_INIT_ATTR_S* sensor_init_attr);
int SPM_SENSOR_StreamOn(void* handle);
int SPM_SENSOR_StreamOff(void* handle);
int SPM_SENSOR_GetOps(void* handle, ISP_SENSOR_REGISTER_S* ops);
int SPM_SENSOR_setTestPatternMode(void* handle, int  test_pattern_mode);
int SPM_SENSOR_ReadReg(void* handle, uint16_t regAddr, uint16_t* value);

/*otp*/
int SPM_SENSORS_MODULE_ProcessOTPData(void* handle, SENSOR_OTP_DATA_S* otp_data);

/*vcm*/
int SPM_VCM_Open(void* handle);
int SPM_VCM_Close(void* handle);
int SPM_VCM_GetOps(void* handle, ISP_AF_MOTOR_REGISTER_S* ops);

/*flash*/
int SPM_FLASH_Open(void* handle);
int SPM_FLASH_Close(void* handle);
int SPM_FLASH_SetMode(void* handle, int mode);

#ifdef __cplusplus
}
#endif /* extern "C" */
#endif
