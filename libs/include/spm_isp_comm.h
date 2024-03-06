/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 * *****************************************************************************
 * File Name     : spm_isp_comm.h
 * Description   : define data types which are used by external.
 */

#ifndef __SPM_ISP_COMM_H__
#define __SPM_ISP_COMM_H__

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "spm_comm_cam.h"
#include "cam_module_interface.h"

#define ASR_ERR_ISP_ILLEGAL_PARAM (-EINVAL)
#define ASR_ERR_ISP_EXIST         (-EEXIST)
#define ASR_ERR_ISP_NOTEXIST      (-ENODEV)
#define ASR_ERR_ISP_NULL_PTR      (-EINVAL)
#define ASR_ERR_ISP_NOT_SUPPORT   (-EPERM)
#define ASR_ERR_ISP_NOT_PERM      (-EPERM)
#define ASR_ERR_ISP_NOMEM         (-ENOMEM)
#define ASR_ERR_ISP_BADADDR       (-EFAULT)
#define ASR_ERR_ISP_BUSY          (-EBUSY)

#define ISP_METERING_REGION_MAX_NUM 6

#define _EPC_SIZESHIFT 12
#define _EPC_NRSHIFT   0
#define _EPC(nr, size) (((nr) << _EPC_NRSHIFT) | ((size) << _EPC_SIZESHIFT))

#define _ISP_EPC(nr, size) _EPC((nr), sizeof(size))
#define _ISP_EPC_SIZE(nr)  (((nr) >> _EPC_SIZESHIFT) & 0xffff)

enum spmCAM_ISP_CTX_PIPELINE_ID {
    CAM_ISP_CTX_PIPELINE_ID0,
    CAM_ISP_CTX_PIPELINE_ID1,
    CAM_ISP_CTX_PIPELINE_ID_MAX,
};

enum spmCAM_ISP_PIPELINE_CHANNEL_ID {
    CAM_ISP_CH_ID_PREVIEW,
    CAM_ISP_CH_ID_CAPTURE,
    CAM_ISP_CH_ID_MAX_NUM,
};

enum spmCAM_ISP_HW_PIPE_ID {
    ISP_HW_PIPE_ID_ID_0,
    ISP_HW_PIPE_ID_ID_1,
    ISP_HW_PIPE_ID_MAX,
};

typedef struct spmISP_SIZE_S {
    uint32_t width;
    uint32_t height;
} ISP_SIZE_S;

typedef struct spmISP_RANGE_S {
    uint32_t minValue;
    uint32_t maxValue;
} ISP_RANGE_S;

/* ISP public attribute, contains the public image attribute */
typedef struct spmISP_PUB_ATTR_S {
    ISP_SIZE_S stInputSize;         /* Width and height of the image input for the pipeline*/
    ISP_SIZE_S stOutSize;           /* Width and height of the image output for the pipeline*/
    ISP_BAYER_PATTERN_E enBayerFmt; /* the format of the input Bayer image.*/
    CAM_SENSOR_RAWTYPE_E enRawType;
} ISP_PUB_ATTR_S;

typedef struct spmISP_OFFLINE_ATTR_S {
    uint32_t blcBitDepth;
    uint32_t blackLevel[4];
    uint32_t exposureTime;  // ns
    uint32_t AGain;         // Q8
    uint32_t imageTGain;    // Q8
    // awb gain.
    uint32_t wbRGain;  // Q12 format
    uint32_t wbGGain;  // Q12 format
    uint32_t wbBGain;  // Q12 format
} ISP_OFFLINE_ATTR_S;

typedef struct spmISP_TUNING_ATTRS_S {
    const char *pTuningFile;
    uint32_t camScene; /*defined by @CAM_ISP_SCENE_E*/
    int *pMergedLscProfile;
    uint32_t mergedLscProfileCnt;
    int *pSnsOtpLscProfile;
    uint32_t snsOtpLscProfileCnt;
    int wbGoldenRG;
    int wbGoldenBG;
    int wbCurrentRG;
    int wbCurrentBG;
    uint32_t skipExposureCtrl; //ae don't take effective on this pipeline.
} ISP_TUNING_ATTRS_S;

typedef int (*GetFrameInfoCallBack)(uint32_t pipelineID, void *pstFrameinfoBuf);

typedef enum spmISP_AE_MODE_E {
    ISP_AE_MODE_AUTO,
    ISP_AE_MODE_MANUAL,
    ISP_AE_MODE_LOCK,
    ISP_AE_MODE_MANUAL_EXPOSURE_INDEX,
    ISP_AE_MODE_INVALID,
} ISP_AE_MODE_E;

typedef enum spmISP_SENSITIVITY_MODE_E {
    ISP_SENSITIVITY_MODE_AUTO,
    ISP_SENSITIVITY_MODE_MANUAL,  // only use when auto ae
    ISP_SENSITIVITY_MODE_INVALID,
} ISP_SENSITIVITY_MODE_E;

typedef enum spmISP_SENSOR_EXPOSURE_MODE_E {
    ISP_SENSOR_EXPOSURE_MODE_AUTO,
    ISP_SENSOR_EXPOSURE_MODE_MANUAL,  // only use when auto ae
    ISP_SENSOR_EXPOSURE_MODE_INVALID,
} ISP_SENSOR_EXPOSURE_MODE_E;

typedef enum spmISP_AWB_MODE_E {
    ISP_AWB_MODE_AUTO,
    ISP_AWB_MODE_MANUAL,
    ISP_AWB_MODE_SHADE,
    ISP_AWB_MODE_CLOUDY_DAYLIGHT,  // 3
    ISP_AWB_MODE_DAYLIGHT,
    ISP_AWB_MODE_WARM_FLUORESCENT,
    ISP_AWB_MODE_FLUORESCENT,  // 6
    ISP_AWB_MODE_INCANDESCENT,
    ISP_AWB_MODE_TWILIGHT,
    ISP_AWB_MODE_LOCK = 9,
    ISP_AWB_MODE_INVALID,
} ISP_AWB_MODE_E;

typedef enum spmISP_AF_MODE_E {
    ISP_AF_MODE_SAF,
    ISP_AF_MODE_CAF,
    ISP_AF_MODE_MANUAL,
    ISP_AF_MODE_LOCK_CAF,
    ISP_AF_MODE_INVALID,
} ISP_AF_MODE_E;

// 0-idle,1-motor busy,2-reached,3-failed,4-caf monitor(motor stop)
typedef enum spmISP_AF_STATUS_E {
    ISP_AF_STATUS_IDLE,
    ISP_AF_STATUS_MOTOR_BUSY,  // 1
    ISP_AF_STATUS_REACHED,
    ISP_AF_STATUS_FAILED,  // 3
    ISP_AF_STATUS_CAF_MONITOR,
    ISP_AF_STATUS_INVALID,
} ISP_AF_STATUS_E;

typedef enum spmISP_ANTIFLICKER_MODE_E {
    ISP_ANTIFLICKER_MODE_OFF,
    ISP_ANTIFLICKER_MODE_50HZ,
    ISP_ANTIFLICKER_MODE_60HZ,
    ISP_ANTIFLICKER_MODE_INVALID,
} ISP_ANTIFLICKER_MODE_E;

typedef enum spmISP_LSC_MODE_E {
    ISP_LSC_MODE_AUTO,
    ISP_LSC_MODE_MANUAL,
    ISP_LSC_MODE_INVALID,
} ISP_LSC_MODE_E;

typedef enum spmISP_CCM_MODE_E {
    ISP_CCM_MODE_AUTO,
    ISP_CCM_MODE_MANUAL,
    ISP_CCM_MODE_INVALID,
} ISP_CCM_MODE_E;

typedef struct spmISP_REGION {
    int32_t start_x;
    int32_t start_y;
    int32_t end_x;
    int32_t end_y;
    int32_t weight;  //[0, 16]
} ISP_REGION_S;

typedef struct spmISP_MANUAL_AE_INFO_S {
    int32_t exposure;           // us
    int32_t totalGainBasedISO;  // Q8
    int32_t analogGain;         // Q8, should be zero if needn't set
    int32_t exposureIndex;      // Q4 used by ISP_AE_MODE_MANUAL_EXPOSURE_INDEX, otherwise is zero.
} ISP_MANUAL_AE_INFO_S;

typedef struct spmISP_AE_INFO_S {
    ISP_AE_MODE_E aeMode;
    ISP_MANUAL_AE_INFO_S *pstManualAe;
} ISP_AE_INFO_S;

typedef struct spmISP_MANUAL_AWB_INFO_S {
    int32_t RGain;  // Q12 mode, 4096 means 1x, range is [4096, 32767]
    int32_t GGain;  // Q12 mode, 4096 means 1x, range is [4096, 32767]
    int32_t BGain;  // Q12 mode, 4096 means 1x, range is [4096, 32767]
} ISP_MANUAL_AWB_INFO_S;

typedef struct spmISP_AWB_INFO_S {
    ISP_AWB_MODE_E awbMode;
    ISP_MANUAL_AWB_INFO_S *pstManualAwb;
} ISP_AWB_INFO_S;

typedef struct spmISP_AF_INFO_S {
    ISP_AF_MODE_E afMode;
    int32_t manualPosition;
    ISP_REGION_S *pstAutoRegion;
} ISP_AF_INFO_S;

typedef struct spmISP_LSC_INFO_S {
    ISP_LSC_MODE_E lscMode;
    int32_t *lscManualProfile;
    int32_t profileNum;
} ISP_LSC_INFO_S;

typedef struct spmISP_CCM_INFO_S {
    ISP_CCM_MODE_E ccmMode;
    int32_t ccmManualData[3][3];
} ISP_CCM_INFO_S;

typedef enum spmISP_METERING_MODE_S {
    ISP_METERING_MODE_CENTER_WEIGHTED,
    ISP_METERING_MODE_AVERAGE,
    ISP_METERING_MODE_SPOT,
    ISP_METERING_MODE_MATRIX,
    ISP_METERING_MODE_MAX,
} ISP_METERING_MODE_S;

typedef struct spmISP_METERING_INFO_S {
    ISP_METERING_MODE_S meteringMode;
    ISP_REGION_S *pstAeRegions;
    int32_t regionsNum;
    int32_t frameID;
} ISP_METERING_INFO_S;

typedef enum spmISP_COLOR_SPACE_E {
    ISP_COLOR_SPACE_ITU601,    // ITU601
    ISP_COLOR_SPACE_ITU709,    // ITU709
    ISP_COLOR_SPACE_EXTENDED,  // for extended
} ISP_COLOR_SPACE_E;

typedef enum spmISP_YUV_RANGE_E {
    ISP_YUV_RANGE_FULL,      // y:0~255, uv:0~255
    ISP_YUV_RANGE_COMPRESS,  // y:16~235, uv:16~240
    ISP_YUV_RANGE_INVALID,
} ISP_YUV_RANGE_E;

typedef struct spmISP_SENSITIVITY_INFO_S {
    ISP_SENSITIVITY_MODE_E sensitivityMode;
    int *pstManualValue;
} ISP_SENSITIVITY_INFO_S;

typedef struct spmISP_SENSOR_EXPOSURE_INFO_S {
    ISP_SENSOR_EXPOSURE_MODE_E exposureMode;
    int *pstManualValue;
} ISP_SENSOR_EXPOSURE_INFO_S;

typedef enum spmISP_AE_SCENE_MODE_E {
    ISP_AE_SCENE_MODE_NORMAL,
    ISP_AE_SCENE_MODE_FACE,
    ISP_AE_SCENE_MODE_INVALID,
} ISP_AE_SCENE_MODE_E;

typedef enum spmISP_COLOR_FILTER_MODE_E {
    ISP_COLOR_FILTER_MODE_NORMAL,
    ISP_COLOR_FILTER_MODE_MONOCHROME,
    ISP_COLOR_FILTER_MODE_INVALID,
} ISP_COLOR_FILTER_MODE_E;

typedef enum spmISP_SOLID_COLOR_MODE_E {
    ISP_SOLID_COLOR_MODE_OFF,
    ISP_SOLID_COLOR_MODE_BALCK,
    ISP_SOLID_COLOR_MODE_INVALID,
} ISP_SOLID_COLOR_MODE_E;

typedef enum spmISP_EFFECT_NR_E {
    EFFECT_NR_S_AE_MODE = 1,
    EFFECT_NR_S_AWB_MODE,
    EFFECT_NR_S_AF_MODE,
    EFFECT_NR_S_TRIGGER_AF,
    EFFECT_NR_S_ANTIFLICKER_MODE,
    EFFECT_NR_S_LSC_MODE,
    EFFECT_NR_S_CCM_MODE,
    EFFECT_NR_S_AECOMPENSATION,
    EFFECT_NR_S_METERING_MODE,
    EFFECT_NR_S_ZOOM_RATIO,
    EFFECT_NR_S_SENSITIVITY_MODE,
    EFFECT_NR_S_SENSOR_EXPOSURE_MODE,
    EFFECT_NR_S_TRIGGER_AE_QUICK_RESPONSE,
    EFFECT_NR_S_AECOMPENSATION_STEP,
    EFFECT_NR_S_AE_SCENE_MODE,
    EFFECT_NR_S_FILTER_MODE,
    EFFECT_NR_S_YUV_RANGE,
    EFFECT_NR_S_SOLID_COLOR_MODE,
    EFFECT_NR_G_AF_MODE,
    EFFECT_NR_G_ANTIFLICKER_MODE,
    EFFECT_NR_G_METERING_MODE,
    EFFECT_NR_G_AF_MOTOR_RANGE,
    EFFECT_NR_G_AE_MODE,
    EFFECT_NR_G_AWB_MODE,
    EFFECT_NR_G_LSC_MODE,
    EFFECT_NR_G_CCM_MODE,
    EFFECT_NR_INVALID_NUM,
} ISP_EFFECT_NR_E;

#define ISP_EFFECT_CMD_S_AE_MODE                   _ISP_EPC(EFFECT_NR_S_AE_MODE, ISP_AE_INFO_S)
#define ISP_EFFECT_CMD_S_AWB_MODE                  _ISP_EPC(EFFECT_NR_S_AWB_MODE, ISP_AWB_INFO_S)
#define ISP_EFFECT_CMD_S_AF_MODE                   _ISP_EPC(EFFECT_NR_S_AF_MODE, ISP_AF_INFO_S)
#define ISP_EFFECT_CMD_S_TRIGGER_AF                _ISP_EPC(EFFECT_NR_S_TRIGGER_AF, int32_t)
#define ISP_EFFECT_CMD_S_ANTIFLICKER_MODE          _ISP_EPC(EFFECT_NR_S_ANTIFLICKER_MODE, int32_t)
#define ISP_EFFECT_CMD_S_LSC_MODE                  _ISP_EPC(EFFECT_NR_S_LSC_MODE, ISP_LSC_INFO_S)
#define ISP_EFFECT_CMD_S_CCM_MODE                  _ISP_EPC(EFFECT_NR_S_CCM_MODE, ISP_CCM_INFO_S)
#define ISP_EFFECT_CMD_S_AECOMPENSATION            _ISP_EPC(EFFECT_NR_S_AECOMPENSATION, int32_t)
#define ISP_EFFECT_CMD_S_METERING_MODE             _ISP_EPC(EFFECT_NR_S_METERING_MODE, ISP_METERING_INFO_S)
#define ISP_EFFECT_CMD_S_ZOOM_RATIO_IN_Q8          _ISP_EPC(EFFECT_NR_S_ZOOM_RATIO, int32_t)
#define ISP_EFFECT_CMD_S_SENSITIVITY_MODE          _ISP_EPC(EFFECT_NR_S_SENSITIVITY_MODE, ISP_SENSITIVITY_INFO_S)
#define ISP_EFFECT_CMD_S_SENSOR_EXPOSURE_MODE      _ISP_EPC(EFFECT_NR_S_SENSOR_EXPOSURE_MODE, ISP_SENSOR_EXPOSURE_INFO_S)
#define ISP_EFFECT_CMD_S_TRIGGER_AE_QUICK_RESPONSE _ISP_EPC(EFFECT_NR_S_TRIGGER_AE_QUICK_RESPONSE, int32_t)
#define ISP_EFFECT_CMD_S_AECOMPENSATION_STEP       _ISP_EPC(EFFECT_NR_S_AECOMPENSATION_STEP, float)
#define ISP_EFFECT_CMD_S_AE_SCENE_MODE             _ISP_EPC(EFFECT_NR_S_AE_SCENE_MODE, int32_t)
#define ISP_EFFECT_CMD_S_FILTER_MODE               _ISP_EPC(EFFECT_NR_S_FILTER_MODE, int32_t)
#define ISP_EFFECT_CMD_S_YUV_RANGE                 _ISP_EPC(EFFECT_NR_S_YUV_RANGE, int32_t)
#define ISP_EFFECT_CMD_S_SOLID_COLOR_MODE          _ISP_EPC(EFFECT_NR_S_SOLID_COLOR_MODE, int32_t)

#define ISP_EFFECT_CMD_G_AF_MODE          _ISP_EPC(EFFECT_NR_G_AF_MODE, ISP_AF_INFO_S)
#define ISP_EFFECT_CMD_G_ANTIFLICKER_MODE _ISP_EPC(EFFECT_NR_G_ANTIFLICKER_MODE, int32_t)
#define ISP_EFFECT_CMD_G_METERING_MODE    _ISP_EPC(EFFECT_NR_G_METERING_MODE, ISP_METERING_INFO_S)
#define ISP_EFFECT_CMD_G_AF_MOTOR_RANGE   _ISP_EPC(EFFECT_NR_G_AF_MOTOR_RANGE, ISP_RANGE_S)
#define ISP_EFFECT_CMD_G_AE_MODE          _ISP_EPC(EFFECT_NR_G_AE_MODE, ISP_AE_INFO_S)
#define ISP_EFFECT_CMD_G_AWB_MODE         _ISP_EPC(EFFECT_NR_G_AWB_MODE, ISP_AWB_INFO_S)
#define ISP_EFFECT_CMD_G_LSC_MODE         _ISP_EPC(EFFECT_NR_G_LSC_MODE, ISP_LSC_INFO_S)
#define ISP_EFFECT_CMD_G_CCM_MODE         _ISP_EPC(EFFECT_NR_G_CCM_MODE, ISP_CCM_INFO_S)

#endif  //__SPM_ISP_COMM_H__
