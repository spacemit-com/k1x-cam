/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "dual_pipeline_capture_test.h"
#include "online_pipeline_test.h"
#include "slice_capture_test.h"
#include "v4l2_single_online.h"
#include "config.h"
#include "board_option.h"

void showTestConfig(struct testConfig config)
{
    int i;

    printf("tuningServerScene: %d\n", config.tuningServerEnalbe);
    printf("show_fps: %d\n", config.showFps);
    printf("auto_run: %d\n", config.autoRun);
    printf("test_frame: %d\n", config.testFrame);
    printf("dump_one_frame: %d\n", config.dumpFrame);
    printf("use_v4l: %d\n", config.useV4l);
    printf("auto_detect: %d\n", config.autoDetect);

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

    if (config.useSnrNode) {
        for (i = 0; i < config.useSnrNode; i++) {
            printf("sensor name: %s\n", config.snrConfig[i].sensorName);
            if (config.snrConfig[i].snrI2cAddr == -1)
                printf("sensor addr: default\n");
            else
                printf("sensor addr: 0x%x\n", config.snrConfig[i].snrI2cAddr);

            if (config.snrConfig[i].vcmEnable) {
                printf("vcm_name: %s\n", config.snrConfig[i].vcmName);
                if (config.snrConfig[i].vcmI2cBus != -1) {
                    printf("vcm i2c bus: %d\n", config.snrConfig[i].vcmI2cBus);
                } else {
                    printf("vcm i2c bus: default (if use)\n");
                }
                if (config.snrConfig[i].vcmI2cAddr != -1) {
                    printf("vcm i2c addr: %d\n", config.snrConfig[i].vcmI2cAddr);
                } else {
                    printf("vcm i2c addr: default (if use)\n");
                }
            }

            if (config.snrConfig[i].flashEnable)
                printf("flash_name: %s\n", config.snrConfig[i].flashName);
        }
    }
    if (config.gpuRender) {
        printf("render width: %d\n", config.renderW);
        printf("render height: %d\n", config.renderH);
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
    char sensors_name[64] = {0};
    int width = 0, height = 0;
    int caseId = 7;
    struct testConfig config = {0};
    int ret = 0, board_id;

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
            if (config.ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE &&
                config.ispFeConfig[1].workMode == ISP_WORKMODE_SLICE_CAPTURE) {
                caseId = 8;
            }
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
            else if (config.ispFeConfig[0].workMode == ISP_WORKMODE_CCIC)
                caseId = 7;
        } else if (config.ispFeConfig[0].enable && config.ispFeConfig[1].enable) {
            if (config.ispFeConfig[0].workMode == ISP_WORKMODE_CCIC &&
                config.ispFeConfig[1].workMode == ISP_WORKMODE_CCIC)
                caseId = 9;
        }
    }

    if (config.useV4l) {
        caseId |= 0xf0;
    }

    if (config.autoDetect) {
        caseId = 0xe0;
    }
    board_id = checkSpacemitBoard();
    config.boardId = board_id;

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
    case 7:
        only_ccic_test(&config);
        break;
    case 8:
        slice_capture_test(&config);
        break;
    case 9:
        only_dual_ccic_test(&config);
        break;
    case 0xe0:
        ret = auto_detect_camera(sensors_name, &width, &height, config.ispFeConfig[0].sensorId, board_id);
        if (ret == 0) {
            update_json_file(&config, argv[1], sensors_name, width, height);
        }
        break;
    case 0xf0:
        v4l2_single_online_test(&config);
        break;
    default:
        printf("error!! invaild caseid %d\n", caseId);
        break;
    }

    return 0;
}
