/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#include "gst_cam_api.h"
#include "gst_online_pipeline.h"
#include "gst_slice_pipeline.h"

struct testConfig *gconfig = NULL;

void gst_release_cam_buffer(IMAGE_BUFFER_S* outputBuf, int index)
{
    if (!gconfig) {
        printf("no config! please check\n");
        return;
    }

    if (gconfig->cppConfig[0].enable && gconfig->cppConfig[1].enable) {
        if (gconfig->ispFeConfig[0].enable && gconfig->ispFeConfig[1].enable) {
            if (gconfig->ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE &&
                gconfig->ispFeConfig[1].workMode == ISP_WORKMODE_SLICE_CAPTURE)
                slice_pipeline_release_buffer(outputBuf, index);
        }
    } else if (gconfig->cppConfig[0].enable && !gconfig->cppConfig[1].enable) {
        if (gconfig->ispFeConfig[0].enable && !gconfig->ispFeConfig[1].enable) {
            if (gconfig->ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE)
                single_pipeline_online_release_buffer(outputBuf, index);
        }
    } else {
        printf("checkTestConfig failed\n");
        return;
    }

}

int gst_setup_camera_start (struct gstParam *para)
{
    int ret = 0;
    int board_id;

    gconfig = malloc(sizeof(struct testConfig));
    if (!gconfig) {
        printf("no config! please check\n");
        return -1;
    }

    CLOG_INFO("analysis json file");
    ret = getTestConfig(gconfig, para->jsonfile);
    if (ret)
        return -1;

    board_id = checkSpacemitBoard();
    gconfig->boardId = board_id;

    if (gconfig->cppConfig[0].enable && gconfig->cppConfig[1].enable) {
        if (gconfig->ispFeConfig[0].enable && gconfig->ispFeConfig[1].enable) {
            if (gconfig->ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE &&
                gconfig->ispFeConfig[1].workMode == ISP_WORKMODE_SLICE_CAPTURE)
                ret = slice_pipeline_start(para, gconfig);
        }
    } else if (gconfig->cppConfig[0].enable && !gconfig->cppConfig[1].enable) {
        if (gconfig->ispFeConfig[0].enable && !gconfig->ispFeConfig[1].enable) {
            if (gconfig->ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE)
                ret = single_pipeline_online_start(para, gconfig);
        }
    } else {
        printf("checkTestConfig failed\n");
        return -1;
    }

    return ret;
}

int gst_setup_camera_stop (struct gstParam *para)
{
    int ret = 0;

    if (!gconfig) {
        printf("no config! please check\n");
        return -1;
    }

    if (gconfig->cppConfig[0].enable && gconfig->cppConfig[1].enable) {
        if (gconfig->ispFeConfig[0].enable && gconfig->ispFeConfig[1].enable) {
            if (gconfig->ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE &&
                gconfig->ispFeConfig[1].workMode == ISP_WORKMODE_SLICE_CAPTURE) {
                ret = slice_pipeline_stop(para);
            }
        }
    } else if (gconfig->cppConfig[0].enable && !gconfig->cppConfig[1].enable) {
        if (gconfig->ispFeConfig[0].enable && !gconfig->ispFeConfig[1].enable) {
            if (gconfig->ispFeConfig[0].workMode == ISP_WORKMODE_ONLINE)
                ret = single_pipeline_online_stop(para);
        }
    } else {
        printf("checkTestConfig failed\n");
        return -1;
    }

    return 0;
}
