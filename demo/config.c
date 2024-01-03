/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */
#include "config.h"

#include <string.h>

#include "cam_log.h"

int getSinglePipelineTestConfig(TESTCONFIG *config)
{
    if (!config) {
        CLOG_ERROR("invalid input parameter config %p", config);
        return -1;
    }

    memset(config, 0, sizeof(TESTCONFIG));

    config->pipeConfig[0].workMode = TEST_WORKMODE_ONLINE;
    config->pipeConfig[0].sensorId = 0;
    strcpy(config->pipeConfig[0].sensorName, "os05a10_asr");
    config->pipeConfig[0].out_width = 1920;
    config->pipeConfig[0].out_height = 1080;

    config->pipeConfig[1].workMode = TEST_WORKMODE_INVALID;

    config->tuningServer = 0;

    return 0;
}
