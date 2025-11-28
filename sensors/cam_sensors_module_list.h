/*
 * Copyright (C) 2023 Spacemit Limited
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
extern SENSOR_OBJ_S ov13855Obj;
extern SENSOR_OBJ_S ov13850Obj;
extern SENSOR_OBJ_S ov2735Obj;
extern SENSOR_OBJ_S ov08d10Obj;
extern SENSOR_OBJ_S ov5647Obj;
extern SENSOR_OBJ_S gc5035Obj;
extern SENSOR_OBJ_S imx135Obj;
extern SENSOR_OBJ_S imx219Obj;
extern SENSOR_OBJ_S imx415Obj;
extern SENSOR_OBJ_S s5k5e3yxObj;
extern SENSOR_OBJ_S gc2375hObj;
extern SENSOR_OBJ_S gc08a0Obj;
extern SENSOR_OBJ_S gc13a0Obj;
extern SENSOR_OBJ_S ov16a10Obj;
extern SENSOR_OBJ_S sc031Obj;
extern SENSOR_OBJ_S sc501aiObj;
extern SENSOR_OBJ_S sc520csObj;
extern SENSOR_OBJ_S sc533haiObj;
extern SENSOR_OBJ_S ov8856Obj;
extern SENSOR_OBJ_S og02b10Obj;
extern SENSOR_OBJ_S mlx75027Obj;
extern SENSOR_OBJ_S bf2257csObj;
extern SENSOR_OBJ_S max96716Obj;

/*module*/
extern MODULE_OBJ_S os05a10_spm_Obj;
extern MODULE_OBJ_S ov13b10_spm_Obj;
extern MODULE_OBJ_S ov13855_spm_Obj;
extern MODULE_OBJ_S ov2735_spm_Obj;
extern MODULE_OBJ_S ov13850_spm_Obj;
extern MODULE_OBJ_S ov5647_spm_Obj;
extern MODULE_OBJ_S ov08d10_spm_Obj;
extern MODULE_OBJ_S gc5035_spm_Obj;
extern MODULE_OBJ_S imx135_spm_Obj;
extern MODULE_OBJ_S imx219_spm_Obj;
extern MODULE_OBJ_S imx415_spm_Obj;
extern MODULE_OBJ_S s5k5e3yx_spm_Obj;
extern MODULE_OBJ_S gc2375h_spm_Obj;
extern MODULE_OBJ_S gc13a0_spm_Obj;
extern MODULE_OBJ_S gc08a8_spm_Obj;
extern MODULE_OBJ_S bf2257cs_spm_Obj;

extern MODULE_OBJ_S ov16a10_spm_Obj;
extern MODULE_OBJ_S sc031_spm_Obj;
extern MODULE_OBJ_S sc501ai_spm_Obj;
extern MODULE_OBJ_S sc520cs_spm_Obj;
extern MODULE_OBJ_S sc533hai_spm_Obj;
extern MODULE_OBJ_S ov8856_spm_Obj;
extern MODULE_OBJ_S og02b10_spm_Obj;
extern MODULE_OBJ_S mlx75027_spm_Obj;
extern MODULE_OBJ_S max96716_spm_Obj;

/*vcm*/
extern VCM_OBJ_S gt9772VcmObj;
extern VCM_OBJ_S dw9714VcmObj;
extern VCM_OBJ_S dw9763VcmObj;

/*flash*/
extern FLASH_OBJ_S aw36515FlashObj;
extern FLASH_OBJ_S aw3641eFlashObj;
/*sensors module*/
typedef struct SENSORS_MODULE_OBJ {
    MODULE_OBJ_S* module_obj_p;
    SENSOR_OBJ_S* sensor_obj_p;
    VCM_OBJ_S* vcm_obj_p;
    FLASH_OBJ_S* flash_obj_p;
} SENSORS_MODULE_OBJ_S;

SENSORS_MODULE_OBJ_S sensors_module_list[] = {
    {&os05a10_spm_Obj, &os05a10Obj, NULL, NULL},
    {&ov13b10_spm_Obj, &ov13b10Obj, &gt9772VcmObj, &aw36515FlashObj},
    {&ov13855_spm_Obj, &ov13855Obj, NULL, NULL},
    {&ov13850_spm_Obj, &ov13850Obj, NULL, NULL},
    {&ov2735_spm_Obj, &ov2735Obj, NULL, NULL},
    {&ov5647_spm_Obj, &ov5647Obj, NULL, NULL},
    {&ov08d10_spm_Obj, &ov08d10Obj, NULL, NULL},
    {&gc5035_spm_Obj, &gc5035Obj, NULL, NULL},
    {&imx135_spm_Obj, &imx135Obj, &dw9714VcmObj, &aw36515FlashObj},
    {&imx219_spm_Obj, &imx219Obj, NULL, NULL},
    {&imx415_spm_Obj, &imx415Obj, NULL, NULL},
    {&s5k5e3yx_spm_Obj, &s5k5e3yxObj, NULL, NULL},
    {&gc2375h_spm_Obj, &gc2375hObj, NULL, NULL},
    {&gc13a0_spm_Obj, &gc13a0Obj, &dw9714VcmObj, &aw3641eFlashObj},
    {&gc08a8_spm_Obj, &gc08a0Obj, NULL, NULL},

    {&ov16a10_spm_Obj, &ov16a10Obj, NULL, NULL},
    {&sc031_spm_Obj, &sc031Obj, NULL, NULL},
    {&sc501ai_spm_Obj, &sc501aiObj, NULL, NULL},
    {&sc520cs_spm_Obj, &sc520csObj, NULL, NULL},
    {&sc533hai_spm_Obj, &sc533haiObj, NULL, NULL},
    {&ov8856_spm_Obj, &ov8856Obj, NULL, NULL},
    {&og02b10_spm_Obj, &og02b10Obj, NULL, NULL},
    {&mlx75027_spm_Obj, &mlx75027Obj, NULL, NULL},
    {&bf2257cs_spm_Obj, &bf2257csObj, NULL, NULL},
    {&max96716_spm_Obj, &max96716Obj, NULL, NULL},
};

VCM_OBJ_S *sensors_vcms_list[] = {
    &gt9772VcmObj,
    &dw9714VcmObj,
    &dw9763VcmObj,
};

FLASH_OBJ_S *sensors_flashs_list[] = {
    &aw36515FlashObj,
    &aw3641eFlashObj,
};

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
