/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */
#include "viisp_common.h"

#include <string.h>

#include "cam_log.h"

static CAM_SENSOR_RAWTYPE_E toCamRawType(int bitDepth)
{
    switch (bitDepth) {
        case 8:
            return CAM_SENSOR_RAWTYPE_RAW8;
        case 10:
            return CAM_SENSOR_RAWTYPE_RAW10;
        case 12:
            return CAM_SENSOR_RAWTYPE_RAW12;
        case 14:
            return CAM_SENSOR_RAWTYPE_RAW14;
        default:
            CLOG_ERROR("donot support pixel bitDepth %d", bitDepth);
            break;
    }
    return CAM_SENSOR_RAWTYPE_INVALID;
}

CAM_VI_PIXEL_FORMAT_E toViFormat(int format)
{
    switch (format) {
        case PIXEL_FORMAT_NV12:
        case PIXEL_FORMAT_NV12_DWT:
            return CAM_VI_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        case PIXEL_FORMAT_FBC:
        case PIXEL_FORMAT_FBC_DWT:
            return CAM_VI_PIXEL_FORMAT_FBC;
        default:
            break;
    }
    return CAM_VI_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
}

static CAM_SENSOR_RAWTYPE_E formatToCamRawType(int format)
{
    switch (format) {
        case PIXEL_FORMAT_RAW_8BPP:
            return CAM_SENSOR_RAWTYPE_RAW8;
        case PIXEL_FORMAT_RAW_10BPP:
            return CAM_SENSOR_RAWTYPE_RAW10;
        case PIXEL_FORMAT_RAW_12BPP:
            return CAM_SENSOR_RAWTYPE_RAW12;
        case PIXEL_FORMAT_RAW_14BPP:
            return CAM_SENSOR_RAWTYPE_RAW14;
        default:
            CLOG_ERROR("donot support format %d", format);
            break;
    }
    return CAM_SENSOR_RAWTYPE_INVALID;
}

/**********************************************************************************/
int viisp_vi_init()
{
    int ret = 0;
    ret = ASR_VI_Init();
    if (ret) {
        CLOG_ERROR("ASR_VI_Init failed, ret = %d", ret);
    }

    return ret;
}

int viisp_vi_deInit()
{
    int ret = 0;
    ret = ASR_VI_Deinit();
    if (ret) {
        CLOG_ERROR("ASR_VI_Deinit failed, ret = %d", ret);
    }

    return ret;
}

int viisp_vi_online_config(int pipelineId, IMAGE_INFO_S out_info, SENSOR_MODULE_INFO* sensor_info)
{
    int ret = 0;
    VI_DEV_ATTR_S viDevAttr = {};
    VI_CHN_ATTR_S viChnAttr = {};
    int devId = pipelineId;
    int chnId = pipelineId;

    viDevAttr.enWorkMode = CAM_VI_WORK_MODE_ONLINE;
    viDevAttr.enRawType = (CAM_SENSOR_RAWTYPE_E)toCamRawType(sensor_info->sensor_cfg->bitDepth);
    viDevAttr.width = sensor_info->sensor_cfg->width;
    viDevAttr.height = sensor_info->sensor_cfg->height;
    viDevAttr.mipi_lane_num = sensor_info->sensor_cfg->lane_num;
    viDevAttr.bindSensorIdx = sensor_info->sensorId;
    ret = ASR_VI_SetDevAttr(devId, &viDevAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_SetDevAttr failed, ret = %d", __func__, ret);
        return ret;
    }
    ret = ASR_VI_GetDevAttr(devId, &viDevAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_GetDevAttr failed, ret = %d", __func__, ret);
        return ret;
    }

    viChnAttr.width = out_info.width;
    viChnAttr.height = out_info.height;
    viChnAttr.enPixFormat = toViFormat(out_info.format);
    ret = ASR_VI_SetChnAttr(chnId, &viChnAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_SetChnAttr failed, ret = %d", __func__, ret);
        return ret;
    }
    ret = ASR_VI_GetChnAttr(chnId, &viChnAttr);
    if (ret) {
        CLOG_ERROR("%s: ASR_VI_GetChnAttr failed, ret = %d", __func__, ret);
        return ret;
    }

    return 0;
}

int viisp_vi_online_streamOn(int pipelineId)
{
    int ret = 0;
    int devId = pipelineId;
    int chnId = pipelineId;

    ret = ASR_VI_EnableDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_EnableDev failed, ret = %d", ret);
    }
    ret = ASR_VI_EnableChn(chnId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_EnableChn failed, ret = %d", ret);
    }
    ret = ASR_VI_EnableBayerDump(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_EnableBayerDump failed, ret = %d", ret);
    }

    return ret;
}

int viisp_vi_online_streamOff(int pipelineId)
{
    int ret = 0;
    int devId = pipelineId;
    int chnId = pipelineId;

    ret = ASR_VI_DisableDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_DisableDev failed, ret = %d", ret);
    }
    ret |= ASR_VI_DisableChn(chnId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_DisableChn failed, ret = %d", ret);
    }
    ret |= ASR_VI_DisableBayerDump(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_DisableBayerDump failed, ret = %d", ret);
    }
    ret |= ASR_VI_FlushDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_FlushDev failed, ret = %d", ret);
    }

    return 0;
}

int viisp_set_vi_callback(int chnId, int32_t (*callback)(uint32_t nChn, VI_IMAGE_BUFFER_S* vi_buffer))
{
    int ret;
    ret = ASR_VI_SetCallback(chnId, callback);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_SetCallback failed, ret = %d", __func__, ret);
        return ret;
    }
    // ASR_VI_ChnEnableForceShadow(chnId, true);

    return ret;
}

int viisp_vi_queueBuffer(int chnId, IMAGE_BUFFER_S* buffer)
{
    int ret = 0;
    ret = ASR_VI_ChnQueueBuffer(chnId, buffer);
    if (ret) {
        CLOG_ERROR("%s: ASR_VI_ChnQueueBuffer failed, chn %d, %dx%d@%d", __func__, chnId, buffer->size.width,
                   buffer->size.height, buffer->format);
    }

    return 0;
}

int viisp_vi_offline_config(int pipelineId, IMAGE_INFO_S out_info, IMAGE_INFO_S in_info, bool isCaptureMode)
{
    int ret = 0;
    VI_DEV_ATTR_S viDevAttr = {};
    VI_CHN_ATTR_S viChnAttr = {};
    int devId = pipelineId;
    int chnId = pipelineId;
    VI_BAYER_READ_ATTR_S viBayerReadAttr = {};

    viDevAttr.enWorkMode = CAM_VI_WORK_MODE_OFFLINE;
    viDevAttr.enRawType = (CAM_SENSOR_RAWTYPE_E)formatToCamRawType(in_info.format);
    viDevAttr.width = in_info.width;
    viDevAttr.height = in_info.height;
    viDevAttr.bOfflineSlice = isCaptureMode ? true : false;
    ret = ASR_VI_SetDevAttr(devId, &viDevAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_SetDevAttr failed, ret = %d", __func__, ret);
        return ret;
    }
    ret = ASR_VI_GetDevAttr(devId, &viDevAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_GetDevAttr failed, ret = %d", __func__, ret);
        return ret;
    }

    viChnAttr.width = out_info.width;
    viChnAttr.height = out_info.height;
    viChnAttr.enPixFormat = toViFormat(out_info.format);
    ret = ASR_VI_SetChnAttr(chnId, &viChnAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_SetChnAttr failed, ret = %d", __func__, ret);
        return ret;
    }
    ret = ASR_VI_GetChnAttr(chnId, &viChnAttr);
    if (ret) {
        CLOG_ERROR("%s: ASR_VI_GetChnAttr failed, ret = %d", __func__, ret);
        return ret;
    }

    viBayerReadAttr.bGenTiming = false;
    ret = ASR_VI_SetBayerReadAttr(devId, &viBayerReadAttr);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_SetBayerReadAttr failed, ret = %d", ret);
        return ret;
    }

    return 0;
}

int viisp_vi_offline_streamOn(int pipelineId)
{
    int ret = 0;
    int devId = pipelineId;
    int chnId = pipelineId;

    ret = ASR_VI_EnableDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_EnableDev failed, ret = %d", ret);
    }
    ret = ASR_VI_EnableChn(chnId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_EnableChn failed, ret = %d", ret);
    }
    ret = ASR_VI_EnableBayerRead(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_EnableBayerRead failed, ret = %d", ret);
    }

    return ret;
}

int viisp_vi_offline_streamOff(int pipelineId)
{
    int ret = 0;
    int devId = pipelineId;
    int chnId = pipelineId;

    ret = ASR_VI_DisableDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_DisableDev failed, ret = %d", ret);
    }
    ret |= ASR_VI_DisableChn(chnId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_DisableChn failed, ret = %d", ret);
    }
    ret |= ASR_VI_DisableBayerRead(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_DisableBayerRead failed, ret = %d", ret);
    }
    ret |= ASR_VI_FlushDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_FlushDev failed, ret = %d", ret);
    }

    return 0;
}
/**********************************************************************************/
int viisp_isp_init(int firmwareId, IMAGE_INFO_S out_info, SENSOR_MODULE_INFO* sensor_info,
                   GetFrameInfoCallBack callback, bool isCaptureMode)
{
    ISP_SENSOR_ATTR_S stSensorAttr = {};
    ISP_PUB_ATTR_S stIspPubAttr = {};
    ISP_TUNING_ATTRS_S stTuningAttr = {0};

    if ((firmwareId != 0) && (isCaptureMode)) {
        CLOG_ERROR("%s: invalid input parameter firmwareId %d, isCaptureMode %d", __func__, firmwareId, isCaptureMode);
        return -1;
    }

    int ret = ASR_ISP_Construct(firmwareId);
    if (ret) {
        CLOG_ERROR("%s: ASR_ISP_Construct %d failed", __func__, firmwareId);
        return ret;
    }

    stSensorAttr.u32SensorId = sensor_info->sensorId;
    ret = ASR_ISP_RegSensorCallBack(firmwareId, &stSensorAttr, sensor_info->pSensorOps);
    if (ret) {
        CLOG_ERROR("%s: ASR_ISP_RegSensorCallBack failed, ret = %d", __func__, ret);
        return ret;
    }

    stIspPubAttr.stInputSize.width = sensor_info->sensor_cfg->width;
    stIspPubAttr.stInputSize.height = sensor_info->sensor_cfg->height;
    stIspPubAttr.stOutSize.width = out_info.width;
    stIspPubAttr.stOutSize.height = out_info.height;
    stIspPubAttr.enBayerFmt = (ISP_BAYER_PATTERN_E)sensor_info->sensor_cfg->pattern;
    stIspPubAttr.enRawType = (CAM_SENSOR_RAWTYPE_E)toCamRawType(sensor_info->sensor_cfg->bitDepth);
    ret = ASR_ISP_SetPubAttr(firmwareId, CAM_ISP_CH_ID_PREVIEW, &stIspPubAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_SetPubAttr preview failed, ret = %d", __func__, ret);
        return ret;
    }

    ret = ASR_ISP_SetChHwPipeID(firmwareId, CAM_ISP_CH_ID_PREVIEW,
                                firmwareId == 0 ? ISP_HW_PIPE_ID_ID_0 : ISP_HW_PIPE_ID_ID_1);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_SetChHwPipeID preview failed, ret = %d", __func__, ret);
        return ret;
    }

    if (isCaptureMode) {
        stIspPubAttr.stInputSize.width = sensor_info->sensor_cfg->width;
        stIspPubAttr.stInputSize.height = sensor_info->sensor_cfg->height;
        stIspPubAttr.stOutSize.width = sensor_info->sensor_cfg->width;
        stIspPubAttr.stOutSize.height = sensor_info->sensor_cfg->height;
        stIspPubAttr.enBayerFmt = (ISP_BAYER_PATTERN_E)sensor_info->sensor_cfg->pattern;
        stIspPubAttr.enRawType = (CAM_SENSOR_RAWTYPE_E)toCamRawType(sensor_info->sensor_cfg->bitDepth);
        ret = ASR_ISP_SetPubAttr(firmwareId, CAM_ISP_CH_ID_CAPTURE, &stIspPubAttr);
        if (ret != 0) {
            CLOG_ERROR("%s: ASR_ISP_SetPubAttr preview failed, ret = %d", __func__, ret);
            return ret;
        }

        ret = ASR_ISP_SetChHwPipeID(firmwareId, CAM_ISP_CH_ID_CAPTURE, ISP_HW_PIPE_ID_ID_1);
        if (ret != 0) {
            CLOG_ERROR("%s: ASR_ISP_SetChHwPipeID preview failed, ret = %d", __func__, ret);
            return ret;
        }
    }

    stTuningAttr.pTuningFile = NULL;
    ret = ASR_ISP_SetTuningParams(firmwareId, &stTuningAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_SetTuningParams %d failed, ret = %d", __func__, firmwareId, ret);
        return ret;
    }

    ret = ASR_ISP_SetFrameinfoCallback(firmwareId, callback);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_SetFrameinfoCallback failed, ret = %d", __func__, ret);
        return ret;
    }

    ret = ASR_ISP_Init(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_Init %d failed, ret = %d", __func__, firmwareId, ret);
        return ret;
    }

    return ret;
}

int viisp_isp_deinit(int firmwareId, int sensorId)
{
    ISP_SENSOR_ATTR_S stSensorAttr = {};
    int ret = 0;

    ret = ASR_ISP_DeInit(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_DeInit failed, ret = %d", __func__, ret);
        return ret;
    }

    stSensorAttr.u32SensorId = sensorId;
    ret = ASR_ISP_UnRegSensorCallBack(firmwareId, &stSensorAttr);
    if (ret) {
        CLOG_ERROR("%s: ASR_ISP_UnRegSensorCallBack failed, ret = %d", __func__, ret);
        return ret;
    }

    ret = ASR_ISP_Destruct(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_Destruct failed, ret = %d", __func__, ret);
        return ret;
    }

    return ret;
}

int viisp_isp_streamOn(int firmwareId)
{
    int ret = 0;
    ret = ASR_ISP_Streamon(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_Streamon failed, ret = %d", __func__, ret);
        return ret;
    }

    return ret;
}

int viisp_isp_streamOff(int firmwareId)
{
    int ret = 0;
    ret = ASR_ISP_FlushFrameinfoBuffer(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_FlushFrameinfoBuffer failed, ret = %d", __func__, ret);
        return ret;
    }

    ret = ASR_ISP_Streamoff(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_Streamoff failed, ret = %d", __func__, ret);
        return ret;
    }

    return ret;
}

int viisp_isp_queueBuffer(int firmwareId, IMAGE_BUFFER_S* buffer)
{
    int ret = 0;
    ret = ASR_ISP_QueueFrameinfoBuffer(firmwareId, buffer);
    if (ret) {
        CLOG_ERROR("%s: ASR_ISP_QueueFrameinfoBuffer failed, chn %d, %dx%d@%d", __func__, firmwareId,
                   buffer->size.width, buffer->size.height, buffer->format);
    }

    return ret;
}

int viisp_isp_triggerRawCapture(int firmwareId, IMAGE_BUFFER_S* buffer)
{
    int ret = 0;
    ret = ASR_ISP_TriggerRawCapture(firmwareId, buffer, 0);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_TriggerRawCapture failed, ret = %d", __func__, ret);
        return ret;
    }

    return ret;
}

void ispout_framerate_stat(uint32_t nChn)
{
    uint64_t diff_time = 0;

    struct timeval tv;
    static struct timeval stv[2];
    static uint32_t count[2] = {0};
    static uint32_t sum_count[2] = {0};
    uint32_t fps = 0;

    gettimeofday(&tv, NULL);

    if (count[nChn] == 0) {
        stv[nChn] = tv;
    }
    sum_count[nChn]++;

    if (count[nChn]++ >= 30) {
        diff_time = (tv.tv_sec - stv[nChn].tv_sec) * 1000 + (tv.tv_usec - stv[nChn].tv_usec) / 1000;
        fps = 1000	* (count[nChn] - 1) / diff_time;
        count[nChn] = 0;
        printf("isp vi ch%d----------------- output fps: (%d %d) \r\n", nChn, sum_count[nChn], fps);
    }
}
