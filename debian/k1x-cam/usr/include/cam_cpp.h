/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef __CAM_CPP_H__
#define __CAM_CPP_H__

#include <CPPGlobalDefine.h>
#include <cam_module_interface.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#define ASR_ID_CPP 4
#define CPP_GRP_MAX_NUM (16)
#define CPP_MIN_IMAGE_WIDTH  (480)
#define CPP_MIN_IMAGE_HEIGHT (288)
#define CPP_MAX_IMAGE_WIDTH  (4224)
#define CPP_MAX_IMAGE_HEIGHT (3136)

typedef struct spmMPP_CHN_S {
    int32_t modId;
    int32_t devId;
    int32_t chnId;
} MPP_CHN_S;

typedef int32_t (*CppCallback)(MPP_CHN_S mppCpp, const IMAGE_BUFFER_S *cppBuf, char success);

typedef enum {
    CPP_GRP_FRAME_MODE,
    CPP_GRP_SLICE_MODE,
    CPP_GRP_MODE_MAX,
} CPP_GRP_WORKMODE_E;

typedef struct spmCPP_GRP_ATTR_S {
    uint32_t width;
    uint32_t height;
    PIXEL_FORMAT_E format;
    CPP_GRP_WORKMODE_E mode;
} CPP_GRP_ATTR_S;

int32_t cam_cpp_create_grp(uint32_t grpId);

int32_t cam_cpp_destroy_grp(uint32_t grpId);

int32_t cam_cpp_start_grp(uint32_t grpId);

int32_t cam_cpp_stop_grp(uint32_t grpId);

int32_t cam_cpp_post_buffer(uint32_t grpId, const IMAGE_BUFFER_S *inputBuf, const IMAGE_BUFFER_S *outputBuf,
                            FRAME_INFO_S *frameInfo);

int32_t cam_cpp_set_callback(uint32_t grpId, CppCallback callback);

int32_t cam_cpp_get_grp_attr(uint32_t grpId, CPP_GRP_ATTR_S *attr);

int32_t cam_cpp_set_grp_attr(uint32_t grpId, CPP_GRP_ATTR_S *attr);

int32_t cam_cpp_load_settingfile(uint32_t grpId, const char *fileName);

int32_t cam_cpp_save_settingfile(uint32_t grpId, const char *fileName);

int32_t cam_cpp_get_tuning_param(uint32_t grpId, cpp_tuning_params_t *tuningParam);

int32_t cam_cpp_set_tuning_param(uint32_t grpId, cpp_tuning_params_t *tuningParam);

int32_t cam_cpp_read_fw(uint32_t grpId, const char *filter, const char *param, int32_t row, int32_t colum,
                        int32_t *pVal);

int32_t cam_cpp_write_fw(uint32_t grpId, const char *filter, const char *param, int32_t row, int32_t colum,
                         int32_t val);

int32_t cam_cpp_read_reg(uint32_t addr, uint32_t *val);

int32_t cam_cpp_write_reg(uint32_t addr, uint32_t val, uint32_t mask);

int32_t cam_cpp_dump_frame(uint32_t grpId, const char *path, uint32_t count);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* ifndef __CAM_CPP_H__ */
