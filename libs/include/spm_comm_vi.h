/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef __SPM_CAM_COMM_VI_H_
#define __SPM_CAM_COMM_VI_H_

#include <cam_module_interface.h>
#include <stdbool.h>
#include <stdint.h>

#include "spm_comm_cam.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#define SUCCESS                        (0)
#define ASR_ERR_VI_NULL_PTR            (-1000)
#define ASR_ERR_VI_INVAL               (-1001)
#define ASR_ERR_VI_NOMEM               (-1002)
#define ASR_ERR_VI_BUSY                (-1003)
#define ASR_ERR_VI_INVALID_DEVID       (-1004)
#define ASR_ERR_VI_INVALID_CHNID       (-1005)
#define ASR_ERR_VI_NOT_CONFIG          (-1006)
#define ASR_ERR_VI_NOT_SUPPORT         (-1007)
#define ASR_ERR_VI_NOT_PERM            (-1008)
#define ASR_ERR_VI_ERR_UNEXIST         (-1009)
#define ASR_ERR_VI_ERR_SIZE_NOT_ENOUGH (-1010)
#define ASR_ERR_VI_NOBUF               (-1011)
#define ASR_ERR_VI_SYS_NOTREADY        (-1012)

#define VIU_MAX_DEV_NUM        (2)
#define VIU_MAX_PHYCHN_PER_DEV (1)
#define VIU_MAX_PHYCHN_NUM     (VIU_MAX_PHYCHN_PER_DEV * VIU_MAX_DEV_NUM)
#define VIU_MAX_CHN_NUM        (VIU_MAX_PHYCHN_NUM)
#define VIU_MAX_RAWCHN_NUM     (2)
#define VIU_MAX_UFLIST_NUM     (VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM)
#define VIU_DEV_MIN_WIDTH      (256)
#define VIU_DEV_MIN_HEIGHT     (144)
#define VIU_DEV_MAX_WIDTH      (65535)
#define VIU_DEV_MAX_HEIGHT     (65535)
#define VIU_CHN_MIN_WIDTH      VIU_DEV_MIN_WIDTH
#define VIU_CHN_MIN_HEIGHT     VIU_DEV_MIN_HEIGHT
#define VIU_CHN_MAX_WIDTH      VIU_DEV_MAX_WIDTH
#define VIU_CHN_MAX_HEIGHT     VIU_DEV_MAX_HEIGHT

#define VIU_GET_RAW_CHN(viDev, rawChn)        \
    do {                                      \
        (rawChn) = VIU_MAX_CHN_NUM + (viDev); \
    } while (0)

#define VIU_GET_RAW_READ_CHN(viDev, rawChn)                        \
    do {                                                           \
        (rawChn) = VIU_MAX_CHN_NUM + VIU_MAX_RAWCHN_NUM + (viDev); \
    } while (0)

typedef enum spmCAM_VI_WORK_MODE_E {
    CAM_VI_WORK_MODE_INVALID = -1,
    CAM_VI_WORK_MODE_ONLINE,
    CAM_VI_WORK_MODE_RAWDUMP,
    CAM_VI_WORK_MODE_OFFLINE,
    CAM_VI_WORK_MODE_MAX,
} CAM_VI_WORK_MODE_E;

typedef enum spmCAM_VI_PIXEL_FORMAT_E {
    CAM_VI_PIXEL_FORMAT_RGB_565 = 0,
    CAM_VI_PIXEL_FORMAT_RGB_888,

    CAM_VI_PIXEL_FORMAT_RGB_BAYER_8BPP,
    CAM_VI_PIXEL_FORMAT_RGB_BAYER_10BPP,
    CAM_VI_PIXEL_FORMAT_RGB_BAYER_12BPP,
    CAM_VI_PIXEL_FORMAT_RGB_BAYER_14BPP,

    CAM_VI_PIXEL_FORMAT_YVU_SEMIPLANAR_420,

    CAM_VI_PIXEL_FORMAT_YUV_SEMIPLANAR_420,

    CAM_VI_PIXEL_FORMAT_Y210,
    CAM_VI_PIXEL_FORMAT_P210,
    CAM_VI_PIXEL_FORMAT_P010,

    CAM_VI_PIXEL_FORMAT_BUTT
} CAM_VI_PIXEL_FORMAT_E;

/*
typedef enum spmVI_DUMP_TYPE_E {
    VI_DUMP_TYPE_RAW,
} VI_DUMP_TYPE_E;
*/

typedef struct spmVI_DEV_ATTR_S {
    CAM_VI_WORK_MODE_E enWorkMode;
    CAM_SENSOR_RAWTYPE_E enRawType;
    uint32_t width;
    uint32_t height;
    uint32_t bindSensorIdx;
    uint32_t mipi_lane_num;
    bool bOfflineSlice;
    bool bCapture2Preview;
} VI_DEV_ATTR_S;

typedef struct spmVI_CHN_ATTR_S {
    CAM_VI_PIXEL_FORMAT_E enPixFormat;
    uint32_t width;
    uint32_t height;
} VI_CHN_ATTR_S;

/*
typedef struct spmVI_DUMP_ATTR_S {
    VI_DUMP_TYPE_E enDumpType;
    CAM_VI_PIXEL_FORMAT_E enPixelFormat;
} VI_DUMP_ATTR_S;
*/

typedef struct spmVI_BAYER_READ_ATTR_S {
    bool bGenTiming;
    int32_t s32FrmRate;
} VI_BAYER_READ_ATTR_S;

typedef struct spmVI_IMAGE_BUFFER_S {
    IMAGE_BUFFER_S *buffer;
    bool bValid;
    bool bCloseDown;
    uint64_t timeStamp;
    uint32_t frameId;
} VI_IMAGE_BUFFER_S;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
