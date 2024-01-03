/*
 * Copyright (C) 2021 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _ASR_CAM_COMM_VI_H_
#define _ASR_CAM_COMM_VI_H_

#include <cam_module_interface.h>
#include <stdbool.h>
#include <stdint.h>

#include "asr_comm_cam.h"

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

typedef enum asrCAM_VI_WORK_MODE_E {
    CAM_VI_WORK_MODE_INVALID = -1,
    CAM_VI_WORK_MODE_ONLINE,
    CAM_VI_WORK_MODE_RAWDUMP,
    CAM_VI_WORK_MODE_OFFLINE,
    CAM_VI_WORK_MODE_MAX,
} CAM_VI_WORK_MODE_E;

typedef enum asrCAM_VI_PIXEL_FORMAT_E {
    CAM_VI_PIXEL_FORMAT_RGB_444 = 0,
    CAM_VI_PIXEL_FORMAT_RGB_555,
    CAM_VI_PIXEL_FORMAT_RGB_565,
    CAM_VI_PIXEL_FORMAT_RGB_888,

    CAM_VI_PIXEL_FORMAT_BGR_444,
    CAM_VI_PIXEL_FORMAT_BGR_555,
    CAM_VI_PIXEL_FORMAT_BGR_565,
    CAM_VI_PIXEL_FORMAT_BGR_888,

    CAM_VI_PIXEL_FORMAT_ARGB_1555,
    CAM_VI_PIXEL_FORMAT_ARGB_4444,
    CAM_VI_PIXEL_FORMAT_ARGB_8565,
    CAM_VI_PIXEL_FORMAT_ARGB_8888,
    CAM_VI_PIXEL_FORMAT_ARGB_2BPP,

    CAM_VI_PIXEL_FORMAT_ABGR_1555,
    CAM_VI_PIXEL_FORMAT_ABGR_4444,
    CAM_VI_PIXEL_FORMAT_ABGR_8565,
    CAM_VI_PIXEL_FORMAT_ABGR_8888,

    CAM_VI_PIXEL_FORMAT_RGB_BAYER_8BPP,
    CAM_VI_PIXEL_FORMAT_RGB_BAYER_10BPP,
    CAM_VI_PIXEL_FORMAT_RGB_BAYER_12BPP,
    CAM_VI_PIXEL_FORMAT_RGB_BAYER_14BPP,
    CAM_VI_PIXEL_FORMAT_RGB_BAYER_16BPP,

    CAM_VI_PIXEL_FORMAT_YVU_PLANAR_422,
    CAM_VI_PIXEL_FORMAT_YVU_PLANAR_420,
    CAM_VI_PIXEL_FORMAT_YVU_PLANAR_444,

    CAM_VI_PIXEL_FORMAT_YVU_SEMIPLANAR_422,
    CAM_VI_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    CAM_VI_PIXEL_FORMAT_YVU_SEMIPLANAR_444,

    CAM_VI_PIXEL_FORMAT_YUV_SEMIPLANAR_422,
    CAM_VI_PIXEL_FORMAT_YUV_SEMIPLANAR_420,
    CAM_VI_PIXEL_FORMAT_YUV_SEMIPLANAR_444,

    CAM_VI_PIXEL_FORMAT_UYVY_PACKAGE_422,
    CAM_VI_PIXEL_FORMAT_YUYV_PACKAGE_422,
    CAM_VI_PIXEL_FORMAT_VYUY_PACKAGE_422,

    CAM_VI_PIXEL_FORMAT_YUV_400,
    CAM_VI_PIXEL_FORMAT_UV_420,

    CAM_VI_PIXEL_FORMAT_BGR_888_PLANAR,
    CAM_VI_PIXEL_FORMAT_HSV_888_PACKAGE,
    CAM_VI_PIXEL_FORMAT_HSV_888_PLANAR,
    CAM_VI_PIXEL_FORMAT_LAB_888_PACKAGE,
    CAM_VI_PIXEL_FORMAT_LAB_888_PLANAR,
    CAM_VI_PIXEL_FORMAT_S8C1,
    CAM_VI_PIXEL_FORMAT_S8C2_PACKAGE,
    CAM_VI_PIXEL_FORMAT_S8C2_PLANAR,
    CAM_VI_PIXEL_FORMAT_S16C1,
    CAM_VI_PIXEL_FORMAT_U8C1,
    CAM_VI_PIXEL_FORMAT_U16C1,
    CAM_VI_PIXEL_FORMAT_S32C1,
    CAM_VI_PIXEL_FORMAT_U32C1,
    CAM_VI_PIXEL_FORMAT_U64C1,
    CAM_VI_PIXEL_FORMAT_S64C1,

    CAM_VI_PIXEL_FORMAT_JPEG,
    CAM_VI_PIXEL_FORMAT_FBC,

    CAM_VI_PIXEL_FORMAT_SBGGR8P,
    CAM_VI_PIXEL_FORMAT_SBGGR10P,
    CAM_VI_PIXEL_FORMAT_SBGGR12P,

    CAM_VI_PIXEL_FORMAT_BUTT
} CAM_VI_PIXEL_FORMAT_E;

/*
typedef enum asrVI_DUMP_TYPE_E {
    VI_DUMP_TYPE_RAW,
} VI_DUMP_TYPE_E;
*/

typedef struct asrVI_DEV_ATTR_S {
    CAM_VI_WORK_MODE_E enWorkMode;
    CAM_SENSOR_RAWTYPE_E enRawType;
    uint32_t width;
    uint32_t height;
    uint32_t bindSensorIdx;
    uint32_t mipi_lane_num;
    bool bOfflineSlice;
    bool bCapture2Preview;
} VI_DEV_ATTR_S;

typedef struct asrVI_CHN_ATTR_S {
    CAM_VI_PIXEL_FORMAT_E enPixFormat;
    uint32_t width;
    uint32_t height;
} VI_CHN_ATTR_S;

/*
typedef struct asrVI_DUMP_ATTR_S {
    VI_DUMP_TYPE_E enDumpType;
    CAM_VI_PIXEL_FORMAT_E enPixelFormat;
} VI_DUMP_ATTR_S;
*/

typedef struct asrVI_BAYER_READ_ATTR_S {
    bool bGenTiming;
    int32_t s32FrmRate;
} VI_BAYER_READ_ATTR_S;

typedef struct asrVI_IMAGE_BUFFER_S {
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
