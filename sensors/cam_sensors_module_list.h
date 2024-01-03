/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _CAM_SENSORS_MODULE_LIST_H_
#define _CAM_SENSORS_MODULE_LIST_H_

#include "cam_sensors_module.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

/*sensor*/
extern SENSOR_OBJ_S os05a10Obj;
extern SENSOR_OBJ_S ov13b10Obj;
extern SENSOR_OBJ_S ov08d10Obj;
extern SENSOR_OBJ_S gc5035Obj;
extern SENSOR_OBJ_S imx135Obj;
extern SENSOR_OBJ_S s5k5e3yxObj;
extern SENSOR_OBJ_S gc2375hObj;

/*module*/
extern MODULE_OBJ_S os05a10_asr_Obj;
extern MODULE_OBJ_S ov13b10_asr_Obj;
extern MODULE_OBJ_S ov08d10_asr_Obj;
extern MODULE_OBJ_S gc5035_asr_Obj;
extern MODULE_OBJ_S imx135_asr_Obj;
extern MODULE_OBJ_S s5k5e3yx_asr_Obj;
extern MODULE_OBJ_S gc2375h_asr_Obj;

/*vcm*/
extern VCM_OBJ_S gt9772VcmObj;
extern VCM_OBJ_S dw9714VcmObj;
extern VCM_OBJ_S dw9763VcmObj;

/*flash*/
extern FLASH_OBJ_S aw36515FlashObj;

/*sensors module*/
typedef struct SENSORS_MODULE_OBJ {
    MODULE_OBJ_S* module_obj_p;
    SENSOR_OBJ_S* sensor_obj_p;
    VCM_OBJ_S* vcm_obj_p;
    FLASH_OBJ_S* flash_obj_p;
} SENSORS_MODULE_OBJ_S;

SENSORS_MODULE_OBJ_S sensors_module_list[] = {
    {&os05a10_asr_Obj, &os05a10Obj, NULL, NULL},
    {&ov13b10_asr_Obj, &ov13b10Obj, &gt9772VcmObj, &aw36515FlashObj},
    {&ov08d10_asr_Obj, &ov08d10Obj, NULL, NULL},
    {&gc5035_asr_Obj, &gc5035Obj, NULL, NULL},
    {&imx135_asr_Obj, &imx135Obj, &dw9714VcmObj, &aw36515FlashObj},
    {&s5k5e3yx_asr_Obj, &s5k5e3yxObj, NULL, NULL},
    {&gc2375h_asr_Obj, &gc2375hObj, NULL, NULL},
};

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif