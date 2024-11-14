/*
 * Copyright (C) 2021 SPACEMIT Micro Limited
 * All Rights Reserved.
 */

#ifndef _SPM_CAM_COMM_CCIC_H_
#define _SPM_CAM_COMM_CCIC_H_

#include <cam_module_interface.h>
#include <stdbool.h>
#include <stdint.h>

#include "spm_comm_cam.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#define CCIC_SUCCESS                        (0)
#define SPM_ERR_CCIC_NULL_PTR            (-1000)
#define SPM_ERR_CCIC_INVAL               (-1001)
#define SPM_ERR_CCIC_NOMEM               (-1002)
#define SPM_ERR_CCIC_BUSY                (-1003)
#define SPM_ERR_CCIC_INVALID_DEVID       (-1004)
#define SPM_ERR_CCIC_INVALID_CHNID       (-1005)
#define SPM_ERR_CCIC_NOT_CONFIG          (-1006)
#define SPM_ERR_CCIC_NOT_SUPPORT         (-1007)
#define SPM_ERR_CCIC_NOT_PERM            (-1008)
#define SPM_ERR_CCIC_ERR_UNEXIST         (-1009)
#define SPM_ERR_CCIC_ERR_SIZE_NOT_ENOUGH (-1010)
#define SPM_ERR_CCIC_NOBUF               (-1011)
#define SPM_ERR_CCIC_SYS_NOTREADY        (-1012)

#define CCU_MAX_DEV_NUM        (3)
#define CCU_MAX_MAIN_CHN_NUM    (3)
#define CCU_MAX_CHN_PER_DEV (2)
#define CCU_MAX_CHN_NUM     (CCU_MAX_CHN_PER_DEV * CCU_MAX_DEV_NUM)

#define CCU_GET_CCIC_MAIN_CHN(nDev, mainChn)        \
    do {                                      \
        (mainChn) = (nDev); \
    } while (0)

#define CCU_GET_CCIC_SUB_CHN(nDev, subChn)        \
    do {                                      \
        (subChn) = (nDev + CCU_MAX_MAIN_CHN_NUM); \
    } while (0)

typedef enum spmCAM_CCIC_PIXEL_FORMAT_E {
    CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_8BPP = 0,
    CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_10BPP,
    CAM_CCIC_PIXEL_FORMAT_RGB_BAYER_12BPP,

    CAM_CCIC_PIXEL_FORMAT_YUYV_PACKAGE_422,

    CAM_CCIC_PIXEL_FORMAT_BUTT
} CAM_CCIC_PIXEL_FORMAT_E;

typedef enum spmCCIC_DEV_MODE_E {
    CCIC_DEV_MODE_NM = 0,
    CCIC_DEV_MODE_VC,
    CCIC_DEV_MODE_VCDT,
} CCIC_DEV_MODE_E;

typedef struct spmCCIC_DEV_ATTR_S {
    uint32_t mipi_lane_num;
    CCIC_DEV_MODE_E mode;
    uint32_t main_vc;
    uint32_t sub_vc;
    uint32_t main_dt;
    uint32_t sub_dt;
} CCIC_DEV_ATTR_S;

typedef struct spmCCIC_CHN_ATTR_S {
    CAM_CCIC_PIXEL_FORMAT_E enPixFormat;
    uint32_t width;
    uint32_t height;
    uint32_t bytesperline[2];
} CCIC_CHN_ATTR_S;

typedef struct spmCCIC_IMAGE_BUFFER_S {
    IMAGE_BUFFER_S *buffer;
    bool bValid;
    uint64_t timeStamp;
    uint32_t frameId;
} CCIC_IMAGE_BUFFER_S;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
