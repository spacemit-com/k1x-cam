/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _ASR_CAM_SENSORS_H_
#define _ASR_CAM_SENSORS_H_

#include "asr_comm_sensors.h"
#include "asr_isp_sensor_comm.h"

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

int ASR_SENSORS_MODULE_Detect(const char* name, int devId);
int ASR_SENSORS_MODULE_Init(void** pHandle, const char* name, int devId, SENSORS_MODULE_INFO_S* module_info);
int ASR_SENSORS_MODULE_Deinit(void* handle);
int ASR_SENSORS_MODULE_EnumCapability(void* handle, SENSORS_MODULE_CAPABILITY_S* cap);
int ASR_GetDeviceInfo(void* handle, void* devInfo);


/*sensor*/
int ASR_SENSOR_Open(void* handle);
int ASR_SENSOR_Close(void* handle);
int ASR_SENSOR_Config(void* handle, int32_t work_mode);
int ASR_SENSOR_SetParam(void* handle, const SENSOR_INIT_ATTR_S* sensor_init_attr);
int ASR_SENSOR_StreamOn(void* handle);
int ASR_SENSOR_StreamOff(void* handle);
int ASR_SENSOR_GetOps(void* handle, ISP_SENSOR_REGISTER_S* ops);
int ASR_SENSOR_setTestPatternMode(void* handle, int  test_pattern_mode);
int ASR_SENSOR_ReadReg(void* handle, uint16_t regAddr, uint16_t* value);

/*otp*/
int ASR_SENSORS_MODULE_ProcessOTPData(void* handle, SENSOR_OTP_DATA_S* otp_data);

/*vcm*/
int ASR_VCM_Open(void* handle);
int ASR_VCM_Close(void* handle);
int ASR_VCM_GetOps(void* handle, ISP_AF_MOTOR_REGISTER_S* ops);

/*flash*/
int ASR_FLASH_Open(void* handle);
int ASR_FLASH_Close(void* handle);
int ASR_FLASH_SetMode(void* handle, int mode);

#ifdef __cplusplus
}
#endif /* extern "C" */
#endif
