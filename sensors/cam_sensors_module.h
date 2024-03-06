/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _CAM_SENSORS_MODULE_H_
#define _CAM_SENSORS_MODULE_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "spm_comm_cam.h"
#include "spm_comm_sensors.h"
#include "spm_isp_sensor_comm.h"
#include "cam_log.h"
#include "cam_sensor_uapi.h"
#include "spm_cam_sensors.h"
#include "camera_base.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#define SENSORS_CHECK_PARA_POINTER(ptr)                                           \
    do {                                                                          \
        if (NULL == (ptr)) {                                                      \
            CLOG_ERROR("%s:%d:Input Para Null Pointer!", __FUNCTION__, __LINE__); \
            return -EINVAL;                                                       \
        }                                                                         \
    } while (0)

#define SENSORS_CHECK_POINTER(ptr)                                     \
    do {                                                               \
        if (NULL == (ptr)) {                                           \
            CLOG_ERROR("%s:%d:Null Pointer!", __FUNCTION__, __LINE__); \
            return -EINVAL;                                            \
        }                                                              \
    } while (0)

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define HIGH_8BITS(x) (((x)&0xff00) >> 8)
#define LOW_8BITS(x)  ((x)&0x00ff)

/*sensor*/
typedef struct SENSOR_VENDOR_ID {
    // uint8_t i2c_addr;
    struct regval_tab* id_table;
    int32_t id_table_size;
} SENSOR_VENDOR_ID_S;

typedef struct SENSOR_WORK_INFO {
    // uint8_t i2c_addr;
    // struct regval_tab* id_table;
    // int32_t id_table_size;
    struct regval_tab* setting_table;
    int32_t setting_table_size;
    uint32_t vts;
    float f32maxFps;
    uint32_t linetime;                           // uint: ns
    uint32_t exp_time[MAX_EXP_CHAN_PER_SENSOR];  // uint: us
    uint32_t again[MAX_EXP_CHAN_PER_SENSOR];
    uint32_t dgain[MAX_EXP_CHAN_PER_SENSOR];
    SENSOR_IMAGE_MODE_E image_mode;
    uint32_t mipi_clock;
    uint32_t work_mode;
    const ISP_DEFAULT_SETTING_S* pstIspDefaultSettings[CAM_ISP_SCENE_INVALID];
    CCSensorTestPatternMode test_pattern_mode;
} SENSOR_WORK_INFO_S;

typedef struct SENSOR_OBJ {
    const char* name;
    int (*pfnInit)(void** pHandle, int sns_id, uint8_t sns_addr);
    int (*pfnDeinit)(void* handle);
    int (*pfnGloablConfig)(void* handle, SENSOR_WORK_INFO_S* work_info);
    int (*pfnSetParam)(void* handle, const SENSOR_INIT_ATTR_S* init_attr);
    int (*pfnStreamOn)(void* handle);
    int (*pfnStreamOff)(void* handle);
    int (*pfnGetSensorOps)(void* handle, ISP_SENSOR_REGISTER_S* pSensorFuncOps);
    int (*pfnDetectSns)(void* handle, SENSOR_VENDOR_ID_S* vendor_id);
    int (*pfnWriteReg)(void* handle, uint16_t regAddr, uint16_t value);
    int (*pfnReadReg)(void* handle, uint16_t regAddr, uint16_t* value);
} SENSOR_OBJ_S;

/*module*/
typedef struct MODULE_OBJ {
    const char* name;
    int (*pfnGetsnrCapSize)(int32_t* capArraySize);
    int (*pfnGetsnrCapbility)(int32_t capArraySize, SENSOR_CAPABILITY_S* sensor_capability);
    int (*pfnGetSnrWorkInfo)(int32_t work_mode, SENSOR_WORK_INFO_S* snr_info);
    int (*pfnGetDevInfo)(int devId, CapDeviceCapInfo* dev_info);
    int (*pfnGetSnrVendorId)(SENSOR_VENDOR_ID_S* vendor_id);
    int (*pfnGetSnrI2cAddr)(uint8_t* i2c_addr);
} MODULE_OBJ_S;

typedef struct VCM_OBJ {
    const char* name;
    int (*pfnInit)(void** pHandle);
    int (*pfnDeinit)(void* handle);
    int (*pfnGetVcmOps)(void* handle, ISP_AF_MOTOR_REGISTER_S* pVcmFuncOps);
} VCM_OBJ_S;

typedef struct FLASH_OBJ {
    const char* name;
    int (*pfnInit)(void** pHandle);
    int (*pfnDeinit)(void* handle);
    int (*pfnSetMode)(void* handle, int mode);
} FLASH_OBJ_S;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
