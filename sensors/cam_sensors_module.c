/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include "cam_sensors_module.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "spm_cam_sensors.h"
#include "cam_sensors_module_list.h"
#include "cam_spm_otp_handle.h"

typedef struct SENSORS_MODULE_CONTEXT {
    int devId;
    SENSORS_MODULE_OBJ_S* sensors_module_obj_p;
    uint32_t magic;

    void* sensor_handle;
    SENSOR_CAPABILITY_S sensor_capability;
    uint8_t sensor_i2c_addr;

    void* vcm_handle;
    void* flash_handle;
    CCSensorTestPatternMode test_pattern_mode;
} SENSORS_MODULE_CONTEXT_S;


#define SENSORS_MODULE_MAGIC 0x5A6B7C8D
#define SENSORS_MODULE_CHECK_HANDLE_IS_ERR(ptr)                                                      \
    do {                                                                                             \
        if ((ptr)->magic != SENSORS_MODULE_MAGIC) {                                                  \
            CLOG_ERROR("%s: sensor module context magic 0x%08x is err", __FUNCTION__, (ptr)->magic); \
            return -ENXIO;                                                                           \
        }                                                                                            \
    } while (0)

#ifndef CAM_API
#define CAM_API __attribute__((visibility("default")))
#endif

/**********************************************************************************************/
static int sensors_module_check_obj(const char* name, SENSORS_MODULE_OBJ_S* sensors_module_obj_p)
{
    SENSORS_CHECK_PARA_POINTER(sensors_module_obj_p);

    if (sensors_module_obj_p->module_obj_p) {
        if (sensors_module_obj_p->module_obj_p->pfnGetsnrCapbility == NULL) {
            CLOG_ERROR("%s: sensors module %s MODULE_OBJ_S.pfnGetsnrCapbility must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->module_obj_p->pfnGetsnrCapSize == NULL) {
            CLOG_ERROR("%s: sensors module %s MODULE_OBJ_S.pfnGetsnrCapSize must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->module_obj_p->pfnGetSnrI2cAddr == NULL) {
            CLOG_ERROR("%s: sensors module %s MODULE_OBJ_S.pfnGetSnrI2cAddr must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->module_obj_p->pfnGetSnrVendorId == NULL) {
            CLOG_ERROR("%s: sensors module %s MODULE_OBJ_S.pfnGetSnrVendorId must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->module_obj_p->pfnGetSnrWorkInfo == NULL) {
            CLOG_ERROR("%s: sensors module %s MODULE_OBJ_S.pfnGetSnrWorkInfo must be not NULL", name, __FUNCTION__);
            return -1;
        }
    } else {
        CLOG_ERROR("%s: sensors module %s MODULE_OBJ_S must be not NULL", name, __FUNCTION__);
        return -1;
    }

    if (sensors_module_obj_p->sensor_obj_p) {
        if (sensors_module_obj_p->sensor_obj_p->pfnInit == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnInit must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnDeinit == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnDeinit must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnDetectSns == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnDetectSns must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnGetSensorOps == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnGetSensorOps must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnGloablConfig == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnGloablConfig must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnReadReg == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnReadReg must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnStreamOff == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnStreamOff must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnStreamOn == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnStreamOn must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnSetParam == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnSetParam must be not NULL", name, __FUNCTION__);
            return -1;
        }
        if (sensors_module_obj_p->sensor_obj_p->pfnWriteReg == NULL) {
            CLOG_ERROR("%s: sensors module %s SENSOR_OBJ_S.pfnWriteReg must be not NULL", name, __FUNCTION__);
            return -1;
        }
    }

    return 0;
}

static int sensors_module_find_obj(const char* name)
{
    int module_id, rc;
    int module_num = ARRAY_SIZE(sensors_module_list);

    SENSORS_CHECK_PARA_POINTER(name);

    for (module_id = 0; module_id < module_num; module_id++) {
        if (sensors_module_list[module_id].module_obj_p) {
            rc = strcmp(name, sensors_module_list[module_id].module_obj_p->name);
            if (rc == 0) {
                rc = sensors_module_check_obj(name, &sensors_module_list[module_id]);
                if (rc == 0) {
                    CLOG_DEBUG("find matched sensors module in sensors_module_list");
                    return module_id;
                }
            }
        }
    }

    CLOG_ERROR("don't find matched sensors module %s in sensors_module_list", name);
    return -1;
}

static int sensors_module_detect_sensor(SENSORS_MODULE_OBJ_S* sensors_module_obj_p, int devId)
{
    void* snr_handle = NULL;
    int ret = 0;
    SENSOR_VENDOR_ID_S vendor_id_table;
    SENSOR_OBJ_S* sensorObj = NULL;
    MODULE_OBJ_S* moduleObj = NULL;
    uint8_t sensor_i2c_addr;

    SENSORS_CHECK_PARA_POINTER(sensors_module_obj_p);
    SENSORS_CHECK_PARA_POINTER(sensors_module_obj_p->module_obj_p);
    SENSORS_CHECK_PARA_POINTER(sensors_module_obj_p->sensor_obj_p);
    moduleObj = sensors_module_obj_p->module_obj_p;
    sensorObj = sensors_module_obj_p->sensor_obj_p;

    moduleObj->pfnGetSnrVendorId(&vendor_id_table);
    moduleObj->pfnGetSnrI2cAddr(&sensor_i2c_addr);

    sensorObj->pfnInit(&snr_handle, devId, sensor_i2c_addr);
    ret = sensorObj->pfnDetectSns(snr_handle, &vendor_id_table);
    sensorObj->pfnDeinit(snr_handle);

    return ret;
}
/**********************************************************************************************/
CAM_API int SPM_SENSORS_MODULE_Detect(const char* name, int devId)
{
    int ret = 0;
    int module_id;
    SENSORS_MODULE_OBJ_S* sensors_module_obj_p = NULL;

    SENSORS_CHECK_PARA_POINTER(name);

    module_id = sensors_module_find_obj(name);
    if (module_id < 0) {
        return -EINVAL;
    }
    sensors_module_obj_p = &sensors_module_list[module_id];
    SENSORS_CHECK_POINTER(sensors_module_obj_p);

    /*sensor*/
    if (sensors_module_obj_p->sensor_obj_p) {
        ret = sensors_module_detect_sensor(sensors_module_obj_p, devId);
        if (ret) {
            goto out;
        }
    }

out:
    if (ret) {
        CLOG_INFO("can not detect sensors module %s devId(%d)", name, devId);
    }
    return ret;
}

CAM_API int SPM_SENSORS_MODULE_Init(void** pHandle, const char* name, int devId, SENSORS_MODULE_INFO_S* module_info)
{
    int module_id;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    MODULE_OBJ_S* moduleObj = NULL;
    int sensor_config_num = 0;

    SENSORS_CHECK_PARA_POINTER(pHandle);
    SENSORS_CHECK_PARA_POINTER(name);
    SENSORS_CHECK_PARA_POINTER(module_info);

    module_id = sensors_module_find_obj(name);
    if (module_id < 0) {
        return -EINVAL;
    }
    SENSORS_CHECK_POINTER(&sensors_module_list[module_id]);
    SENSORS_CHECK_POINTER(sensors_module_list[module_id].module_obj_p);

    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)calloc(1, sizeof(SENSORS_MODULE_CONTEXT_S));
    if (NULL == sensors_module_context) {
        CLOG_ERROR("%s: sensors_module_context malloc memory failed!", __FUNCTION__);
        return -ENOMEM;
    }
    sensors_module_context->devId = devId;
    sensors_module_context->magic = SENSORS_MODULE_MAGIC;
    sensors_module_context->sensors_module_obj_p = &sensors_module_list[module_id];
    moduleObj = sensors_module_context->sensors_module_obj_p->module_obj_p;

    moduleObj->pfnGetsnrCapSize(&sensor_config_num);
    if (sensor_config_num) {
        sensors_module_context->sensor_capability.snr_config =
            (SENSOR_CONFIG_S*)calloc(1, sensor_config_num * sizeof(SENSOR_CONFIG_S));
        if (NULL == sensors_module_context->sensor_capability.snr_config) {
            CLOG_ERROR("%s: sensors_module_context malloc memory failed!", __FUNCTION__);
            if (sensors_module_context) {
                memset(sensors_module_context, 0, sizeof(SENSORS_MODULE_CONTEXT_S));
                free(sensors_module_context);
                sensors_module_context = NULL;
            }
            return -ENOMEM;
        }
        moduleObj->pfnGetsnrCapbility(sensor_config_num, &sensors_module_context->sensor_capability);
    }
    moduleObj->pfnGetSnrI2cAddr(&sensors_module_context->sensor_i2c_addr);

    module_info->snr_config_num = sensors_module_context->sensor_capability.snr_config_num;
    *pHandle = sensors_module_context;
    return 0;
}

CAM_API int SPM_SENSORS_MODULE_Deinit(void* handle)
{
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p);

    /*sensor*/
    if ((sensors_module_context->sensors_module_obj_p->sensor_obj_p) && (sensors_module_context->sensor_handle)) {
        SENSOR_OBJ_S* sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;
        sensorObj->pfnDeinit(sensors_module_context->sensor_handle);
        sensors_module_context->sensor_handle = NULL;
    }

    if (sensors_module_context) {
        if (sensors_module_context->sensor_capability.snr_config) {
            memset(sensors_module_context->sensor_capability.snr_config, 0,
                   sensors_module_context->sensor_capability.snr_config_num * sizeof(SENSOR_CONFIG_S));
            free(sensors_module_context->sensor_capability.snr_config);
            sensors_module_context->sensor_capability.snr_config = NULL;
        }
        memset(sensors_module_context, 0, sizeof(SENSORS_MODULE_CONTEXT_S));
        free(sensors_module_context);
        sensors_module_context = NULL;
    }

    return 0;
}

CAM_API int SPM_SENSORS_MODULE_EnumCapability(void* handle, SENSORS_MODULE_CAPABILITY_S* cap)
{
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(cap);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);

    cap->sensor_capability.snr_config_num = sensors_module_context->sensor_capability.snr_config_num;
    if (cap->sensor_capability.snr_config_num) {
        SENSORS_CHECK_POINTER(cap->sensor_capability.snr_config);
        memcpy(cap->sensor_capability.snr_config, sensors_module_context->sensor_capability.snr_config,
               sensors_module_context->sensor_capability.snr_config_num * sizeof(SENSOR_CONFIG_S));
    }

    return 0;
}

CAM_API int SPM_SENSORS_MODULE_ProcessOTPData(void* handle, SENSOR_OTP_DATA_S* otp_data)
{
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(otp_data);

    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;

    //TODO: rear primary sensor support otp temporary.
    if (sensors_module_context->devId != 0)
        return ret;

    //1. get otp from sensor
    ret = cam_read_otp_lsc_profile_from_file(otp_data->lsc_profile);
    if (ret)
        return ret;

    //2. process
    ret = cam_spm_otp_handle_ctx_init(sensors_module_context->devId);
    if (ret) {
        return ret;
    }

    ret = cam_spm_otp_handle_ctx_set_lsc_params(otp_data->lsc_profile, OTP_LSC_PROFILE_LEN);
    if (ret) {
        goto Safe_Exit;
    }

    ret = cam_spm_otp_handle_ctx_process_lsc_data(otp_data->lsc_merged_profile[0], LSC_PROFILE_DIMENSION * OTP_LSC_PROFILE_LEN);
    if (ret)
        goto Safe_Exit;

    //awb
    otp_data->awb_goldenRG = 0;
    otp_data->awb_goldenBG = 0;
    otp_data->awb_currentRG = 0;
    otp_data->awb_currentBG = 0;
    otp_data->data_valid = 1;

    CLOG_INFO("sensor%d merge otp data successful.", sensors_module_context->devId);
Safe_Exit:
    cam_spm_otp_handle_ctx_deinit();
    return ret;
}

/*sensor*/
CAM_API int SPM_SENSOR_Open(void* handle)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSOR_OBJ_S* sensorObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    if (sensors_module_context->sensor_handle) {
        CLOG_WARNING("sensor%d has been opened, don't need to open again", sensors_module_context->devId);
        return -EBUSY;
    }
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->sensor_obj_p);
    sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;

    ret = sensorObj->pfnInit(&sensors_module_context->sensor_handle, sensors_module_context->devId,
                             sensors_module_context->sensor_i2c_addr);
    return ret;
}

CAM_API int SPM_SENSOR_Close(void* handle)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSOR_OBJ_S* sensorObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    if (sensors_module_context->sensor_handle == NULL) {
        CLOG_WARNING("sensor%d has been closed, don't need to close again", sensors_module_context->devId);
        return -EBUSY;
    }
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->sensor_obj_p);
    sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;

    ret = sensorObj->pfnDeinit(sensors_module_context->sensor_handle);
    sensors_module_context->sensor_handle = NULL;
    return ret;
}

CAM_API int SPM_SENSOR_Config(void* handle, int32_t work_mode)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSOR_OBJ_S* sensorObj = NULL;
    MODULE_OBJ_S* moduleObj = NULL;
    SENSOR_WORK_INFO_S snr_info;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->sensor_handle);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->sensor_obj_p);
    sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->module_obj_p);
    moduleObj = sensors_module_context->sensors_module_obj_p->module_obj_p;

    ret = moduleObj->pfnGetSnrWorkInfo(work_mode, &snr_info);
    if (ret) {
        return ret;
    }

    snr_info.test_pattern_mode = sensors_module_context->test_pattern_mode;

    ret = sensorObj->pfnGloablConfig(sensors_module_context->sensor_handle, &snr_info);
    return ret;
}

CAM_API int SPM_SENSOR_setTestPatternMode(void* handle, int test_pattern_mode)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    if(test_pattern_mode)
        sensors_module_context->test_pattern_mode = test_pattern_mode;
    return ret;
}

CAM_API int SPM_SENSOR_SetParam(void* handle, const SENSOR_INIT_ATTR_S* sensor_init_attr)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSOR_OBJ_S* sensorObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->sensor_handle);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->sensor_obj_p);
    sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;

    ret = sensorObj->pfnSetParam(sensors_module_context->sensor_handle, sensor_init_attr);
    return ret;
}

CAM_API int SPM_SENSOR_StreamOn(void* handle)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSOR_OBJ_S* sensorObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->sensor_handle);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->sensor_obj_p);
    sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;
    ret = sensorObj->pfnStreamOn(sensors_module_context->sensor_handle);
    return ret;
}

CAM_API int SPM_SENSOR_StreamOff(void* handle)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSOR_OBJ_S* sensorObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->sensor_handle);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->sensor_obj_p);
    sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;

    ret = sensorObj->pfnStreamOff(sensors_module_context->sensor_handle);
    return ret;
}

CAM_API int SPM_SENSOR_GetOps(void* handle, ISP_SENSOR_REGISTER_S* ops)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSOR_OBJ_S* sensorObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(ops);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->sensor_handle);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->sensor_obj_p);
    sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;

    ret = sensorObj->pfnGetSensorOps(sensors_module_context->sensor_handle, ops);

    return ret;
}

CAM_API int SPM_SENSOR_ReadReg(void* handle, uint16_t regAddr, uint16_t* value)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    SENSOR_OBJ_S* sensorObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->sensor_handle);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->sensor_obj_p);
    sensorObj = sensors_module_context->sensors_module_obj_p->sensor_obj_p;

    ret = sensorObj->pfnReadReg(sensors_module_context->sensor_handle, regAddr, value);

    return ret;
}

CAM_API int SPM_VCM_Open(void* handle)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    VCM_OBJ_S* vcmObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    if (sensors_module_context->vcm_handle) {
        CLOG_WARNING("vcm has been opened, don't need to open again");
        return -EBUSY;
    }
    vcmObj = sensors_module_context->sensors_module_obj_p->vcm_obj_p;
    if (vcmObj == NULL) {
        return -ENOTTY;
    }

    ret = vcmObj->pfnInit(&sensors_module_context->vcm_handle);

    return ret;
}

CAM_API int SPM_VCM_Close(void* handle)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    VCM_OBJ_S* vcmObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    if (sensors_module_context->vcm_handle == NULL) {
        CLOG_WARNING("vcm has been closed, don't need to close again");
        return -EBUSY;
    }
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->vcm_obj_p);
    vcmObj = sensors_module_context->sensors_module_obj_p->vcm_obj_p;

    ret = vcmObj->pfnDeinit(sensors_module_context->vcm_handle);
    sensors_module_context->vcm_handle = NULL;
    return ret;
}

CAM_API int SPM_VCM_GetOps(void* handle, ISP_AF_MOTOR_REGISTER_S* ops)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    VCM_OBJ_S* vcmObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(ops);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->vcm_handle);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->vcm_obj_p);
    vcmObj = sensors_module_context->sensors_module_obj_p->vcm_obj_p;

    ret = vcmObj->pfnGetVcmOps(sensors_module_context->vcm_handle, ops);

    return ret;
}
CAM_API int SPM_FLASH_Open(void* handle)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    FLASH_OBJ_S* flashObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    if (sensors_module_context->flash_handle) {
        CLOG_WARNING("flash has been opened, don't need to open again");
        return -EBUSY;
    }
    flashObj = sensors_module_context->sensors_module_obj_p->flash_obj_p;
    if (flashObj == NULL) {
        return -ENOTTY;
    }

    ret = flashObj->pfnInit(&sensors_module_context->flash_handle);

    return ret;
}

CAM_API int SPM_FLASH_Close(void* handle)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    FLASH_OBJ_S* flashObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    if (sensors_module_context->flash_handle == NULL) {
        CLOG_WARNING("flash has been closed, don't need to close again");
        return -EBUSY;
    }
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->flash_obj_p);
    flashObj = sensors_module_context->sensors_module_obj_p->flash_obj_p;

    ret = flashObj->pfnDeinit(sensors_module_context->flash_handle);
    sensors_module_context->flash_handle = NULL;
    return ret;
}

CAM_API int SPM_FLASH_SetMode(void* handle, int mode)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    FLASH_OBJ_S* flashObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->flash_handle);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->flash_obj_p);
    flashObj = sensors_module_context->sensors_module_obj_p->flash_obj_p;

    ret = flashObj->pfnSetMode(sensors_module_context->flash_handle, mode);

    return ret;
}

CAM_API int SPM_GetDeviceInfo(void* handle, void* devCapInfo)
{
    int ret = 0;
    SENSORS_MODULE_CONTEXT_S* sensors_module_context = NULL;
    MODULE_OBJ_S* moduleObj = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensors_module_context = (SENSORS_MODULE_CONTEXT_S*)handle;
    SENSORS_MODULE_CHECK_HANDLE_IS_ERR(sensors_module_context);
    SENSORS_CHECK_POINTER(sensors_module_context->sensors_module_obj_p->module_obj_p);
    moduleObj = sensors_module_context->sensors_module_obj_p->module_obj_p;

    if (moduleObj->pfnGetDevInfo != NULL)
        ret = moduleObj->pfnGetDevInfo(sensors_module_context->devId, (CapDeviceCapInfo*)devCapInfo);

    return ret;
}
