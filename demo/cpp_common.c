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

int cpp_load_fw_settingfile(int groupId, char *filename)
{
    int ret = 0;

    if (!filename)
        return -1;

    ret = cam_cpp_load_settingfile(groupId, filename);
    if (ret < 0) {
        CLOG_ERROR("cpp: load setting file %s ret = %d\n", filename, ret);
        return ret;
    }
    CLOG_INFO("cpp: load setting file OK\n");
    return 0;
}

int cpp_save_fw_settingfile(int groupId, char *filename)
{
    int ret = 0;

    if (!filename)
        return -1;

    ret = cam_cpp_save_settingfile(groupId, filename);
    if (ret < 0) {
        CLOG_ERROR("cpp: save setting to file %s ret = %d\n", filename, ret);
        return ret;
    }
    CLOG_INFO("cpp: save setting file OK\n");
    return 0;
}

int cpp_test_fw_infs(int groupId)
{
    int ret = 0;
    cpp_tuning_params_t tuningParam = {0};
    int val = 0;
    uint32_t addr = 0x0;
    uint32_t valreg = 0x0;

    ret = cam_cpp_get_tuning_param(groupId, &tuningParam);
    if (ret) {
        CLOG_ERROR("cam_cpp_get_tuning_param failed\n");
        return ret;
    } else {
        CLOG_INFO("get cp fw fd eb: %d\n", tuningParam.params_3dnr.eb);
    }
    tuningParam.params_3dnr.eb = 0;
    ret = cam_cpp_set_tuning_param(groupId, &tuningParam);
    if (ret) {
        CLOG_ERROR("cam_cpp_set_tuning_param failed\n");
        return ret;
    }
    ret = cam_cpp_get_tuning_param(groupId, &tuningParam);
    if (ret) {
        CLOG_ERROR("cam_cpp_get_tuning_param failed\n");
        return ret;
    } else {
        CLOG_INFO("get cp fw fd eb: %d\n", tuningParam.params_3dnr.eb);
    }

    ret = cam_cpp_read_fw(groupId, "C3DNRFirmwareFilter", "m_pMax_diff_thr", 1, 1, &val);
    if (ret < 0) {
        CLOG_ERROR("cam_cpp_read_fw failed, ret=%d\n", ret);
        return ret;
    } else {
        CLOG_INFO("cam_cpp_read_fw C3DNRFirmwareFilter:m_pMax_diff_thr[1, 1] val: %d\n", val);
    }
    ret = cam_cpp_write_fw(groupId, "C3DNRFirmwareFilter", "m_pMax_diff_thr", 1, 1, 63);
    if (ret) {
        CLOG_ERROR("cam_cpp_write_fw failed\n");
        return ret;
    }
    ret = cam_cpp_read_fw(groupId, "C3DNRFirmwareFilter", "m_pMax_diff_thr", 1, 1, &val);
    if (ret < 0) {
        CLOG_ERROR("cam_cpp_read_fw failed\n");
        return ret;
    } else {
        CLOG_INFO("cam_cpp_read_fw C3DNRFirmwareFilter:m_pMax_diff_thr[1, 1] val: %d\n", val);
    }

    addr = 0xc02f0144;
    ret = cam_cpp_read_reg(addr, &valreg);
    if (ret) {
        CLOG_ERROR("cam_cpp_read_reg failed\n");
        return ret;
    } else {
        CLOG_INFO("cam_cpp_read_reg 0x%x val: 0x%x\n", addr, valreg);
    }
    ret = cam_cpp_write_reg(addr, 0xffffffff, 0xffffffff);
    if (ret) {
        CLOG_ERROR("cam_cpp_write_reg failed, ret = %d\n", ret);
        return ret;
    }
    ret = cam_cpp_read_reg(addr, &valreg);
    if (ret) {
        CLOG_ERROR("cam_cpp_read_reg failed\n");
        return ret;
    } else {
        CLOG_INFO("cam_cpp_read_reg 0x%x val: 0x%x", addr, valreg);
    }
	return 0;
}