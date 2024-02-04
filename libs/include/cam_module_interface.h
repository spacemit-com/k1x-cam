/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef __CAM_MODULE_INTERFACE_H__
#define __CAM_MODULE_INTERFACE_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#ifndef NULL
#define NULL 0L
#endif

#ifndef ARRAY1_COUNT
#define ARRAY1_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#endif /* ARRAY1_COUNT */

#ifndef ARRAY2_COUNT
#define ARRAY2_COUNT(array) (sizeof(array) / sizeof((array)[0][0]))
#endif /* ARRAY2_COUNT */

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif /* MIN */

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif /* MAX */

#ifndef ALIGN_N
#define ALIGN_N(x, n) (((x) + (n)-1) & (~((n)-1)))
#endif

#define IMAGE_BUFFER_MAX_PLANES 4
#define DWT_MAX_PLANES          2

typedef struct asrSIZE_S {
    uint32_t width;
    uint32_t height;
} SIZE_S;

typedef struct asrRECT_S {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} RECT_S;

typedef enum asrPIXEL_FORMAT_E {
    PIXEL_FORMAT_NV12,
    PIXEL_FORMAT_NV12_DWT,
    PIXEL_FORMAT_FBC,
    PIXEL_FORMAT_FBC_DWT,
    PIXEL_FORMAT_RAW_8BPP,
    PIXEL_FORMAT_RAW_10BPP,
    PIXEL_FORMAT_RAW_12BPP,
    PIXEL_FORMAT_RAW_14BPP,
    PIXEL_FORMAT_RAW,
    PIXEL_FORMAT_JPEG,
    PIXEL_FORMAT_RGB565,
    PIXEL_FORMAT_RGB888,
    PIXEL_FORMAT_Y210,
    PIXEL_FORMAT_P210,
    PIXEL_FORMAT_P010,
    PIXEL_FORMAT_YUYV,
    PIXEL_FORMAT_YVYU,

    PIXEL_FORMAT_MAX,
} PIXEL_FORMAT_E;

typedef struct asrIMAGE_BUFFER_PLANE_S {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t scanline;
    uint32_t offset;
    uint32_t length;
    void* virAddr;
    int fd;
} IMAGE_BUFFER_PLANE_S;

typedef struct asrIMAGE_BUFFER_S {
    SIZE_S size;
    PIXEL_FORMAT_E format;
    uint32_t numPlanes;
    IMAGE_BUFFER_PLANE_S planes[IMAGE_BUFFER_MAX_PLANES];
    IMAGE_BUFFER_PLANE_S dwt1[DWT_MAX_PLANES];
    IMAGE_BUFFER_PLANE_S dwt2[DWT_MAX_PLANES];
    IMAGE_BUFFER_PLANE_S dwt3[DWT_MAX_PLANES];
    IMAGE_BUFFER_PLANE_S dwt4[DWT_MAX_PLANES];
    uint32_t type;
    union {
        uint64_t phyAddr;
        int32_t blockId;
        int32_t fd;
    } m;
} IMAGE_BUFFER_S;

typedef enum asrTOUCH_TAG {
    CC_ISP_TOUCH_TAG_IDLE = 0,
    CC_ISP_TOUCH_TAG_TOUCH_AE_NO_FLASH,
    CC_ISP_TOUCH_TAG_TOUCH_AF_NO_FLASH,
    CC_ISP_TOUCH_TAG_TOUCH_AE_FLASH,
    CC_ISP_TOUCH_TAG_TOUCH_AF_FLASH,
    CC_ISP_TOUCH_TAG_TOUCH_AE_CANCEL,
    CC_ISP_TOUCH_TAG_TOUCH_AF_CANCEL,
} TOUCH_TAG_S;

#ifndef CC_MAX_FACE_COUNT
#define CC_MAX_FACE_COUNT 10
#endif

typedef struct asrFRAME_INFO {
    // fill by isp sdk
    int frameId;
    int imageTGain;     // Q8
    int snsAGain;       // Q8
    int snsDGain;       // Q12
    int awbBgain;       // Q12
    int awbGgain;       // Q12
    int awbRgain;       // Q12
    int awbApplyBgain;  // Q12
    int awbGbgain;      // Q12
    int awbApplyRgain;  // Q12
    int awbGrgain;      // Q12
    int blc12b;
    int bayerOrder;
    int wbGoldenSignatureRG;
    int wbGoldenSignatureBG;
    int wbModuleSignatureRG;
    int wbModuleSignatureBG;
    int drcGain;      // Q8
    int drcGainDark;  // Q8
    int32_t meanY;
    int zoomInput;                   // Q8
    int zoomApply;                   // Q8
    int32_t validInHeightAssociated; /*the output height associated with input for flicker*/
    int32_t sensorVts;
    float curFps;
    uint8_t aeStableFlag;  /* 0 - unstable, 1 - stable */
    uint8_t awbStableFlag; /* 0 - unstable, 1 - stable */
    uint8_t afStatus;
    /* 0-idle, 1-busy, 2-reached, 3-failed */  // yuanlei not matched with CameraMetadata.cpp, please check
    int32_t currentCcm00;
    int32_t currentCcm01;
    int32_t currentCcm02;
    int32_t currentCcm10;
    int32_t currentCcm11;
    int32_t currentCcm12;
    int32_t currentCcm20;
    int32_t currentCcm21;
    int32_t currentCcm22;
    int32_t lscProfile[576];
    int64_t sensorExposureTime;
    int64_t sensorFrameDuration;
    int32_t sensorSensitivity;
    int64_t sensorRollingShutterSkew;
    int32_t colorFilterMode; //defined by @ISP_COLOR_FILTER_MODE_E

    //init ae and awb param
    int32_t curSceneLuma;
    int32_t curSceneLux;
    int32_t curCorrelationCT;
    int32_t curTint;

    // fill by camera core
    int pipelineId;
    int isHdrCapture;
    bool flashNeedIfAuto;
    bool isFlashCapture;
    int64_t sensorTimestamp;

    // TODO
    uint16_t afVcmPosition;       /* [66  : 0x37042] AF VCM current code */
    uint16_t currentCcmSignFlags; /* [108 : 0x3706c] */
    TOUCH_TAG_S touchTag;
    int32_t afLock;
    int32_t curAfMode;
    int32_t curAfTrigger;
    int32_t aeLock;
    int32_t curAeMode;
    int32_t curAwbMode;
    bool AergnSetAvl;
} FRAME_INFO_S;

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* __CAM_MODULE_INTERFACE_H__ */
