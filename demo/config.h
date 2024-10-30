/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "cam_module_interface.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum {
    AUX_NONE = 0,
    AUX_VCM,
    AUX_FLASH,
    AUX_VCM_AND_FLASH,
    AUX_DEVICE_MAX,
} AUX_DEVICE;

struct cppTestConfig {
    int enable;
    char format[32];
    int srcFromFile;
    char srcFile[64];
    int width;
    int height;
};

typedef enum {
    ISP_WORKMODE_ONLINE = 0,
    ISP_WORKMODE_RAWDUMP,
    ISP_WORKMODE_OFFLINE_PREVIEW,
    ISP_WORKMODE_OFFLINE_CAPTURE,
    ISP_WORKMODE_SLICE_CAPTURE,
    ISP_WORKMODE_NUM,
} ISP_WORKMODE;
struct isFeTestConfig {
    int enable;
    ISP_WORKMODE workMode;
    char format[32];

    char sensorName[32];
    int sensorId;
    int sensorWorkMode;
    int fps;

    char srcFile[64];
    int bitDepth;
    int inWidth;
    int inHeight;
    int outWidth;
    int outHeight;
};
struct SensorTestConfig {
    char sensorName[32];
    char vcmName[32];
    char flashName[32];

    int snrI2cAddr;
    int vcmTestOnly;
    int vcmEnable;
    int vcmI2cAddr;
    int vcmI2cBus;
    int flashEnable;
    int auxDevice;  //0: close, 1: af, 2: flash, 3: af+flash
};
struct testConfig {
    int tuningServerEnalbe;
    int showFps;
    int autoRun;
    int testFrame;
    int dumpFrame;
    int useSnrNode;
    int useV4l;

    int vcmEnable;
    int vcmI2cAddr;
    int vcmI2cBus;
    int flashEnable;
    int auxDevice;  //0

    struct cppTestConfig cppConfig[2];
    struct isFeTestConfig ispFeConfig[2];
    struct SensorTestConfig snrConfig[2];
};

int getTestConfig(struct testConfig *config, char *jsonfile);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_LIST_H_ */
