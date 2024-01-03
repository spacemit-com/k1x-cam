/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */

#include "cpp_common.h"

#include <string.h>

#include "cam_log.h"

int cpp_init(int groupId, IMAGE_INFO_S config, CppCallback callback)
{
    int ret = 0;
    CPP_GRP_ATTR_S attr = {};
    ret = cam_cpp_create_grp(groupId);
    if (ret) {
        CLOG_ERROR("%s: create cpp groupId %d fail", __func__, groupId);
        return ret;
    }

    attr.width = config.width;
    attr.height = config.height;
    attr.format = config.format;
    attr.mode = (attr.width > 1920) ? CPP_GRP_SLICE_MODE : CPP_GRP_FRAME_MODE;
    ret = cam_cpp_set_grp_attr(groupId, &attr);
    if (ret < 0) {
        CLOG_ERROR("cam_cpp_set_grp_attr %d failed, %dx%d@%d", groupId, attr.width, attr.height, attr.format);
        return ret;
    }

    ret = cam_cpp_set_callback(groupId, callback);
    if (ret < 0) {
        CLOG_ERROR("cam_cpp_set_callback %d failed", groupId);
        return ret;
    }

    return ret;
}

int cpp_deInit(int groupId)
{
    int ret = 0;
    ret = cam_cpp_destroy_grp(groupId);
    if (ret != 0)
        CLOG_ERROR("%s: cam_cpp_destroy_grp group %d failed", __func__, groupId);

    return ret;
}

int cpp_start(int groupId)
{
    int ret = 0;
    ret = cam_cpp_start_grp(groupId);
    if (ret != 0)
        CLOG_ERROR("%s: cam_cpp_start_grp group %d failed", __func__, groupId);

    return ret;
}

int cpp_stop(int groupId)
{
    int ret = 0;
    ret = cam_cpp_stop_grp(groupId);
    if (ret != 0)
        CLOG_ERROR("%s: cam_cpp_stop_grp group %d failed", __func__, groupId);

    return ret;
}

int cpp_post_buffer(int groupId, const IMAGE_BUFFER_S *inputBuf, const IMAGE_BUFFER_S *outputBuf, int32_t frameId,
                    FRAME_INFO_S *frameInfo)
{
    int ret = 0;
    ret = cam_cpp_post_buffer(groupId, inputBuf, outputBuf, frameInfo);
    if (ret != 0)
        CLOG_ERROR("%s: cam_cpp_post_buffer group %d failed", __func__, groupId);

    return ret;
}