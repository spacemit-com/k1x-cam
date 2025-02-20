/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "cam_sensors_module.h"

void *g_pIspFwHandle = NULL;

int cam_spm_otp_handle_ctx_deinit()
{
    int ret = 0;

    if (g_pIspFwHandle) {
        FirmwareDestroy(g_pIspFwHandle);
        g_pIspFwHandle = NULL;
    }

    return ret;
}

int _get_settingfile_absolute_path(const char *tuning_file, char *file_path)
{
    const char *pathFile = CAM_CONFIG_FILE_PATH "cam_tuning.cfg";
    const char *tag = "@@ASR_Camera_Tuning_Path";
    char lineBuffer[1024];
    char filename[256] = {};
    int ret = -1;

    FILE *fp = fopen(pathFile, "r");
    if (!fp) {
        CLOG_WARNING("no tuning path file, open %s failed", pathFile);
        return false;
    }

    while (fgets(lineBuffer, sizeof(lineBuffer), fp) != NULL) {
        char *str = strstr(lineBuffer, tag);
        if (str == NULL)
            continue;
        fgets(lineBuffer, sizeof(lineBuffer), fp);
        str = strstr(lineBuffer, "{");
        if (str == NULL) {
            CLOG_ERROR("Invalid camera tuning config file");
            goto end;
        }
        while (1) {
            char *r = fgets(lineBuffer, sizeof(lineBuffer), fp);
            str = strstr(lineBuffer, "}");
            if (str || r == NULL) {
                break;
            }
            str = strchr(lineBuffer, '/');
            if (str == NULL) {
                CLOG_ERROR("tuning path must be start with '/'");
                goto end;
            }
            char *comma = strchr(str, ',');
            if (comma) {
                *comma = '\0';
            }
            snprintf(filename, sizeof(filename), "%s/%s", str, tuning_file);
            if (access(filename, R_OK) == 0) {
                ret = 0;
                break;
            }
        }
        if (ret == 0) {
            break;
        }
    }
    if (ret == 0) {
        memcpy(file_path, filename, sizeof(filename));
    }

end:
    fclose(fp);
    return ret;
}

int cam_spm_otp_handle_ctx_init(int sns_id)
{
    int ret = 0;
    char file_path[256] = {0};
    char *filename = NULL;

    if (g_pIspFwHandle) {
        CLOG_ERROR("%s: isp firmware has already been created!", __FUNCTION__);
        return -EEXIST;
    }

    ret = FirmwareCreate(&g_pIspFwHandle);
    if (ret) {
        CLOG_ERROR("%s: create isp firmware failed,ret=%d!", __FUNCTION__, ret);
        return -EPERM;
    }

    switch (sns_id) {
        case 0:
            filename = "rear_primary_isp_setting.data";
            break;
        case 1:
            filename = "rear_secondary_isp_setting.data";
            break;
        case 2:
            filename = "front_isp_setting.data";
            break;
        default:
            return ret;
    }

    ret = _get_settingfile_absolute_path(filename, file_path);
    if (ret) {
        CLOG_ERROR("%s: get settingfile path failed!", __FUNCTION__);
        return -EPERM;
    }

    ret = FirmwareLoadSettingFile(g_pIspFwHandle, file_path);
    if (ret) {
        cam_spm_otp_handle_ctx_deinit();
        CLOG_ERROR("firmware load setting file(%s) failed, ret=%d!", file_path, ret);
        ret = -EPERM;
    }

    return ret;
}

int cam_spm_otp_handle_ctx_set_lsc_params(int *pSensorOtpLscData, int dataCount)
{
    int ret = 0;

    if (!g_pIspFwHandle) {
        CLOG_ERROR("%s: please init cam_spm_otp_handle first!", __FUNCTION__);
        return -EPERM;
    }

    SENSORS_CHECK_PARA_POINTER(pSensorOtpLscData);
    if (dataCount != 576) {
        CLOG_ERROR("%s: invalid count(%d), it must be 576!", __FUNCTION__, dataCount);
        return -EINVAL;
    }

    ret = FirmwareSetFilterParamList(g_pIspFwHandle, "CLSCFirmwareFilter", "m_pOTPProfile", 1, 576, pSensorOtpLscData);
    if (ret) {
        CLOG_ERROR("%s: set sensor opt data failed, ret=%d!", __FUNCTION__, ret);
        ret = -EPERM;
    }

    return ret;
}

int cam_spm_otp_handle_ctx_process_lsc_data(int *pMergedOtpLscData, int dataCount)
{
    int ret = 0, validCount = 3 * 576;

    if (!g_pIspFwHandle) {
        CLOG_ERROR("%s: please init cam_spm_otp_handle first!", __FUNCTION__);
        return -EPERM;
    }

    SENSORS_CHECK_PARA_POINTER(pMergedOtpLscData);
    if (dataCount != validCount) {
        CLOG_ERROR("%s: invalid count(%d), it must be %d!", __FUNCTION__, dataCount, validCount);
        return -EINVAL;
    }

    ret = FirmwareProcessShadingOTPProfile(g_pIspFwHandle);
    if (ret) {
        CLOG_ERROR("%s: process opt data failed,ret=%d!", __FUNCTION__, ret);
        ret = -EPERM;
    }

    ret = FirmwareGetShadingOTPProfile(g_pIspFwHandle, pMergedOtpLscData);
    if (ret) {
        CLOG_ERROR("%s: get merged opt data failed,ret=%d!", __FUNCTION__, ret);
        ret = -EPERM;
    }

    return ret;
}

char* _return_no_front_space(char *src)
{
    int i = 0;
    char *tmp = src;

    if (tmp) {
        while (src[i] == ' ' || src[i] == '\t' || src[i] == '\0') {
            tmp++;
            i++;
        }
        return tmp;
    } else
        return NULL;
}

int cam_read_otp_lsc_profile_from_file(int *lsc_top_data)
{
    int ret = 0, row = 0, col = 0, token_index = 0, tmp_value;
    char *pOtpFile = CAM_CONFIG_FILE_PATH "otp_profile.data", *ch_tmp = NULL;
    const char *tag = "@@OTP_LSC_Profile";
    char lineBuffer[1024];
    char *str = NULL, *r = NULL, *token = NULL, *pos = NULL;
    const char delims[2] = ",";

    SENSORS_CHECK_PARA_POINTER(lsc_top_data);
    FILE *fp = fopen(pOtpFile, "r");
    if (!fp) {
        CLOG_WARNING("no lsc otp file, open %s failed", pOtpFile);
        return -EPERM;
    }

    while (fgets(lineBuffer, sizeof(lineBuffer), fp) != NULL) {
        str = strstr(lineBuffer, tag);
        if (str == NULL)
            continue;
        fgets(lineBuffer, sizeof(lineBuffer), fp);
        str = strstr(lineBuffer, "{");
        if (str == NULL) {
            CLOG_ERROR("Invalid otp file");
            ret = -EBADF;
            goto end;
        }

        fgets(lineBuffer, sizeof(lineBuffer), fp);
        str = strstr(lineBuffer, "Size:");
        if (str == NULL) {
            CLOG_ERROR("Invalid otp file");
            ret = -EBADF;
            goto end;
        } else {
            sscanf(str, "Size:%dx%d", &row, &col);
        }

        while (1) {
            r = fgets(lineBuffer, sizeof(lineBuffer), fp);
            str = strstr(lineBuffer, "}");
            if (str || r == NULL) {
                break;
            }
  
            token = strtok(lineBuffer, delims);
            while (token != NULL) {
                ch_tmp = _return_no_front_space(token);
                if (0 == strcmp(ch_tmp, "\r\n"))
                    break;
                tmp_value = strtol(token, &pos, 10);
                lsc_top_data[token_index++] = tmp_value;
                if (token_index > row * col) {
                    CLOG_ERROR("%s has more lsc data than size(%d)", pOtpFile, row * col);
                    ret = -EBADF;
                    break;
                }
                token = strtok(NULL, delims);
            }
        }
    }

end:
    fclose(fp);
    return ret;
}
