/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _GST_CAM_API_H_
#define _GST_CAM_API_H_

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "bufferPool.h"
#include "cam_list.h"
#include "cam_log.h"
#include "cam_interface.h"
#include "condition.h"
#include "config.h"
#include "cpp_common.h"
#include "sensor_common.h"
#include "viisp_common.h"
#include "tuning_server.h"

#define MAX_BUFFER_RAWDUMP_NUM 5
#define MAX_BUFFER_NUM   4
#define MAX_PIPELINE_NUM 2
#define MAX_FIRMWARE_NUM 2
#define RAW8_DUMP_SIZE(w, h) ((w / 16 + (w % 16 ? 1 : 0)) * 16 * h)
#define RAW10_DUMP_SIZE(w, h) ((w / 12 + (w % 12 ? 1 : 0)) * 16 * h)
#define RAW12_DUMP_SIZE(w, h) ((w / 10 + (w % 10 ? 1 : 0)) * 16 * h)
#define RAW14_DUMP_SIZE(w, h) ((w / 8 + (w % 8 ? 1 : 0)) * 16 * h)
#define VRF_INFO_LEN (128)

typedef void* (*threadFunc)(void* param);
typedef struct {
    pthread_t threadId;
    char threadName[20];
    int threadRunning;
    threadFunc threadProcessFunc;
    struct condition cond;
    int pipelineId;
    int firmwareId;
} THREAD_INFO;

typedef struct spmVI_BUFFER_INFO {
    IMAGE_BUFFER_S* buffer;
    uint32_t frameId;
} spmVI_BUFFER_INFO_S;

typedef struct spmISP_BUFFER_INFO {
    FRAME_INFO_S frameInfo;
    uint32_t frameId;
} spmISP_BUFFER_INFO_S;

typedef struct spmTuning_BUFFER_INFO {
    TUNING_BUFFER_S tuningInfo;
    char hasVrf;
} spmTuning_BUFFER_INFO_S;

struct rawdump_info {
    int width;
    int height;
    int format;
    int start;
    int count;
    char addVrf;
    int bitDepth;
    uint32_t ispInfoListLen;
    CAM_VI_WORK_MODE_E viWorkMode;
    LIST_HANDLE rawdumpList;
    LIST_HANDLE ispInfoList;
    LIST_HANDLE isp_done_list;
    pthread_mutex_t rawdumpListLock;
    pthread_mutex_t ispInfoListLock;
    pthread_cond_t rawdumpListCond;
    BUFFER_POOL *rawdumpPool;
};

typedef struct VRF_INFO {
    uint16_t imageWidth;   /* [0  ] Raw image width in pixel number */
    uint16_t imageHeight;  /* [2  ] Raw image height in pixel number */
    uint16_t totalGain;    /* [4  ] Total gain for the raw file. Q4 format. 0x0010 means 1x, 0x0028 means 2.5x */
    uint16_t exposureLine; /* [6  ] Exposure line number for the raw file. Q0 format. 0x0001 means 1 line, 0x0010 means
                              16 line */
    uint16_t sensorVts;    /* [8  ] Sensor VTS line number. Q0 format */
    uint16_t awbBGain1;    /* [10 ] AWB gain set 1(before AWB shift), Q7 format, 0x80 means 1x */
    uint16_t awbGGain1;    /* [12 ] */
    uint16_t awbRGain1;    /* [14 ] */
    uint16_t awbBGain2;    /* [16 ] */
    uint16_t awbGbGain2;   /* [18 ] */
    uint16_t awbGrGain2;   /* [20 ] */
    uint16_t awbRGain2;    /* [22 ] */
    uint8_t blackLevel;    /* [24 ] In 10 bit */
    uint8_t bayerOrder;    /* [25 ] 0 BGGR, 1 GBRG, 2 GRBG, 3 RGGB */
    uint16_t blcApply;     /* [26 ] */
    uint8_t reserved[78];  /* [28 ~ 105] */
    uint16_t wbGoldenSignatureRG; /* [106] golden WB signature R*512/G (under version 2.8 and above)*/
    uint16_t wbGoldenSignatureBG; /* [108] golden WB signature B*512/G (under version 2.8 and above)*/
    uint16_t wbModuleSignatureRG; /* [110] module WB signature R*512/G (under version 2.8 and above)*/
    uint16_t wbModuleSignatureBG; /* [112] module WB signature B*512/G (under version 2.8 and above)*/
    uint8_t drcGainDark;          /* [114] Q4 format, range in [16, 255], (under version 2.9)*/
    uint8_t drcGain;              /* [115] Q4 format, range in [16, 255], (under version 2.9)*/
    uint32_t exposureTime;        /* [116] us */
    uint8_t packRawFlag;          /* [120] 0 - no pack, 1 - pack */
    uint8_t rawBitDepth;          /* [121] can be one of 8,10,12,14,16 */
    uint16_t analogGain; /* [122] Analog gain for the raw file. Q4 format. 0x0010 means 1x, 0x0028 means 2.5x */
    uint8_t version;     /* [124] 0x20 means v2.0, 0x21means v2.1 ... */
    uint8_t v;           /* [125] 'V'. In ASCII code, can be used for file format identification */
    uint8_t r;           /* [126] 'R'. In ASCII code, can be used for file format identification */
    uint8_t f;           /* [127] 'F'. In ASCII code, can be used for file format identification */
} VRF_INFO_S;            // V2.6

struct gstParam {
    char *jsonfile;

    int (*gst_get_cpp_buffer)(IMAGE_BUFFER_S*, int);
    int (*gst_cpp_buf_prepare)(void *, IMAGE_BUFFER_S*);
    void *gst_cpp_buf_prepare_data;
    int sensorInfoId;
    int pipelineId;
    int firmwareId;
    void* sensorHandle;
    int out_height;
    int out_width;
};
void gst_release_cpp_buffer(IMAGE_BUFFER_S* outputBuf, int index);

int gst_setup_camera_start(struct gstParam *para);
int gst_setup_camera_stop(struct gstParam *para);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _ONLINE_PIPELINE_TEST_H_ */
