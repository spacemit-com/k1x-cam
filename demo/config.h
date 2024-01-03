/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

typedef enum {
    TEST_WORKMODE_INVALID = 0,
    TEST_WORKMODE_RAW,
    TEST_WORKMODE_ONLINE,
    TEST_WORKMODE_OFFLINE,
    TEST_WORKMODE_OFFLINE_RAW,
    TEST_WORKMODE_NUM,
} TEST_WORKMODE;

typedef struct {
    TEST_WORKMODE workMode;
    int sensorId;
    char sensorName[20];
    int out_width;
    int out_height;
} PIPELINECONFIG;

typedef struct {
    PIPELINECONFIG pipeConfig[2];
    int tuningServer;
} TESTCONFIG;

int getSinglePipelineTestConfig(TESTCONFIG *config);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_LIST_H_ */
