/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dual_pipeline_capture_test.h"
#include "online_pipeline_test.h"
#include "config.h"

void showTestConfig(struct testConfig config)
{
    int i;

    printf("tuningServerScene: %d\n", config.tuningServerEnalbe);
    printf("show_fps: %d\n", config.showFps);
    printf("auto_run: %d\n", config.autoRun);
    for (i = 0; i < 2; i++) {
        printf("cpp%d enable: %d\n", i, config.cppConfig[i].enable);
        printf("cpp%d src_path: %s\n", i,  config.cppConfig[i].srcFile);
        printf("cpp%d size_width: %d\n", i, config.cppConfig[i].width);
        printf("cpp%d size_height: %d\n", i, config.cppConfig[i].height);
    }
    for (i = 0; i < 2; i++) {
        printf("isp%d enable: %d\n",i, config.ispFeConfig[i].enable);
        printf("isp%d work_mode: %d\n", i, config.ispFeConfig[i].workMode);
        printf("isp%d format: %s\n", i, config.ispFeConfig[i].format);
        printf("isp%d out_width: %d\n", i, config.ispFeConfig[i].outWidth);
        printf("isp%d out_height: %d\n", i, config.ispFeConfig[i].outHeight);
        printf("isp%d sensor_name: %s\n", i, config.ispFeConfig[i].sensorName);
        printf("isp%d sensor_id: %d\n", i, config.ispFeConfig[i].sensorId);
        printf("isp%d sensor_work_mode: %d\n", i, config.ispFeConfig[i].sensorWorkMode);
        printf("isp%d fps: %d\n", i, config.ispFeConfig[i].fps);
        printf("isp%d src_file: %s\n", i, config.ispFeConfig[i].srcFile);
        printf("isp%d bit_depth: %d\n", i, config.ispFeConfig[i].bitDepth);
        printf("isp%d in_width: %d\n", i, config.ispFeConfig[i].inWidth);
        printf("isp%d in_height: %d\n", i, config.ispFeConfig[i].inHeight);
    }
}

int checkTestConfig(struct testConfig *cfg)
{
    if (!cfg)
        return -1;

    if (cfg->ispFeConfig[1].workMode == ISP_WORKMODE_OFFLINE_CAPTURE &&
        cfg->ispFeConfig[0].workMode != ISP_WORKMODE_ONLINE) {
            printf("isp1 workmode is offline_capture but isp0 workmode is not online.\n");
            return -1;
    }
    if (cfg->ispFeConfig[0].workMode == ISP_WORKMODE_OFFLINE_CAPTURE) {
        printf("isp0 offline capture not complemented in demo. Refs to isp1 offline capture case.\n");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int caseId = 7;
    struct testConfig config = {0};
    int ret = 0;

    if (argc == 2) {
        ret = getTestConfig(&config, argv[1]);
    } else {
        ret = getTestConfig(&config, NULL);
    }
    if (ret) {
        printf("getTestConfig failed\n");
        return ret;
    }

    showTestConfig(config);
    ret = checkTestConfig(&config);
    if (ret) {
        printf("checkTestConfig failed\n");
        return ret;
    }

    if (config.cppConfig[0].enable && config.cppConfig[1].enable) {
        if (config.ispFeConfig[0].enable && config.ispFeConfig[1].enable) {
            if (config.ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE &&
                config.ispFeConfig[1].workMode == ISP_WORKMODE_ONLINE) {
                    caseId = 1;
            }
            if (config.ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE &&
                config.ispFeConfig[1].workMode == ISP_WORKMODE_OFFLINE_CAPTURE)
                caseId = 2;
        }
    } else if (config.cppConfig[0].enable && !config.cppConfig[1].enable) {
        if (config.ispFeConfig[0].enable && !config.ispFeConfig[1].enable) {
            if (config.ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE)
                caseId = 0;
        } else if (!config.ispFeConfig[0].enable && !config.ispFeConfig[1].enable)
            caseId = 6;
    } else if (!config.cppConfig[0].enable && !config.cppConfig[1].enable) {
        if (config.ispFeConfig[0].enable && !config.ispFeConfig[1].enable) {
            if (config.ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE)
                caseId = 4;
            else if (config.ispFeConfig[0].workMode == ISP_WORKMODE_RAWDUMP)
                caseId = 3;
            else if (config.ispFeConfig[0].workMode == ISP_WORKMODE_OFFLINE_PREVIEW)
                caseId = 5;
        }
    }

    switch (caseId) {
    case 0:
        single_pipeline_online_test(&config);
        break;
    case 1:
        dual_pipeline_online_test(&config);
        break;
    case 2:
        dual_pipeline_capture_test(&config);
        break;
    case 3:
        only_rawdump_test(&config);
        break;
    case 4:
        only_viisp_online_test(&config);
        break;
    case 5:
        only_viisp_offline_preview_test(&config);
        break;
    case 6:
        only_cpp_test(&config);
        break;
    default:
        break;
    }

    return 0;
}
