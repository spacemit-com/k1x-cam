/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */
#include "viisp_common.h"

#include <string.h>

#include "cam_log.h"

//#define EN_ISP_TUNING
static const char ispInputFwFile[2][128] = {"/tmp/isp0_input_setting.data", "/tmp/isp1_input_setting.data"};
static const char ispOutputFwFile[2][128] = {"/tmp/isp0_output_setting.data", "/tmp/isp1_output_setting.data"};
static const char ispIndividualSettingFile[2][128] = {"/tmp/isp0_cae_setting.data", "/tmp/isp1_cae_setting.data"};

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
        case PIXEL_FORMAT_RGB565:
            return CAM_VI_PIXEL_FORMAT_RGB_565;
        case PIXEL_FORMAT_RGB888:
            return CAM_VI_PIXEL_FORMAT_RGB_888;
        case PIXEL_FORMAT_Y210:
            return CAM_VI_PIXEL_FORMAT_Y210;
        case PIXEL_FORMAT_P210:
            return CAM_VI_PIXEL_FORMAT_P210;
        case PIXEL_FORMAT_P010:
            return CAM_VI_PIXEL_FORMAT_P010;
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

int viisp_vi_offline_preview_config(int pipelineId, ISP_PUB_ATTR_S *pstIspPubAttr)
{
    int ret = 0;
    VI_DEV_ATTR_S viDevAttr = {};
    VI_CHN_ATTR_S viChnAttr = {};
    VI_BAYER_READ_ATTR_S viBayerReadAttr;
    int devId = pipelineId;
    int chnId = pipelineId;

    viDevAttr.enWorkMode = CAM_VI_WORK_MODE_OFFLINE;
    viDevAttr.enRawType = pstIspPubAttr->enRawType;
    viDevAttr.width = pstIspPubAttr->stInputSize.width;
    viDevAttr.height = pstIspPubAttr->stInputSize.height;
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

    viChnAttr.width = pstIspPubAttr->stOutSize.width;
    viChnAttr.height = pstIspPubAttr->stOutSize.height;
    viChnAttr.enPixFormat = CAM_VI_PIXEL_FORMAT_YUV_SEMIPLANAR_420; //toViFormat(out_info.format);
    ret = ASR_VI_SetChnAttr(0, &viChnAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_SetChnAttr failed, ret = %d", __func__, ret);
        return ret;
    }
    ret = ASR_VI_GetChnAttr(chnId, &viChnAttr);
    if (ret) {
        CLOG_ERROR("%s: ASR_VI_GetChnAttr failed, ret = %d", __func__, ret);
        return ret;
    }

    viBayerReadAttr.bGenTiming = true;
    viBayerReadAttr.s32FrmRate = 30;
    ret = ASR_VI_SetBayerReadAttr(pipelineId, &viBayerReadAttr);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_VI_SetBayerReadAttr failed, ret = %d", __func__, ret);
    }

    return 0;
}

int viisp_vi_offline_preview_streamOn(int pipelineId)
{
    int ret = 0;
    int devId = pipelineId;
    int chnId = pipelineId;

    ret = ASR_VI_EnableDev(devId);
    if (ret) {
        CLOG_ERROR("ASR_VI_EnableDev failed, ret = %d", ret);
    }
    ret = ASR_VI_EnableChn(chnId);
    if (ret) {
        CLOG_ERROR("ASR_VI_EnableChn failed, ret = %d", ret);
    }
    ret = ASR_VI_EnableBayerRead(devId);
    if (ret) {
        CLOG_ERROR("ASR_VI_EnableBayerRead failed, ret = %d", ret);
    }
    return ret;
}

int viisp_vi_offline_preview_streamOff(int pipelineId)
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
        CLOG_ERROR("ASR_VI_DisableBayerDump failed, ret = %d", ret);
    }
    ret |= ASR_VI_FlushDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_FlushDev failed, ret = %d", ret);
    }

    return 0;
}

int viisp_isp_offline_preview_init(int firmwareId, ISP_PUB_ATTR_S *stIspPubAttr, ISP_OFFLINE_ATTR_S *stOfflineAttr, GetFrameInfoCallBack callback)
{
    int ret = 0;
    ISP_TUNING_ATTRS_S stTuningAttr;

    ret = ASR_ISP_Construct(firmwareId);
    if (ret) {
        CLOG_ERROR("%s: ASR_ISP_Construct %d failed", __func__, firmwareId);
        return ret;
    }

    memset(&stTuningAttr, 0, sizeof(ISP_TUNING_ATTRS_S));
    stTuningAttr.camScene = CAM_ISP_SCENE_PREVIEW;
    ASR_ISP_SetTuningParams(firmwareId, &stTuningAttr);
    ASR_ISP_SetPubAttr(firmwareId, CAM_ISP_CH_ID_PREVIEW, stIspPubAttr);
    ASR_ISP_SetChHwPipeID(firmwareId, CAM_ISP_CH_ID_PREVIEW, firmwareId == 0 ? ISP_HW_PIPE_ID_ID_0 : ISP_HW_PIPE_ID_ID_1);
    ret = ASR_ISP_SetFrameinfoCallback(firmwareId, callback);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_SetFrameinfoCallback failed, ret = %d", __func__, ret);
        return ret;
    }
    ASR_ISP_EnableOfflineMode(firmwareId, 1, stOfflineAttr);
    ret = ASR_ISP_Init(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_Init %d failed, ret = %d", __func__, firmwareId, ret);
    }

    return ret;
}

int viisp_isp_offline_preview_deinit(int firmwareId)
{
    ISP_SENSOR_ATTR_S stSensorAttr = {};
    int ret = 0;

    ret = ASR_ISP_DeInit(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_DeInit failed, ret = %d", __func__, ret);
        return ret;
    }

    ret = ASR_ISP_Destruct(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_Destruct failed, ret = %d", __func__, ret);
        return ret;
    }

    return ret;
}

int viisp_vi_onlyrawdump_config(int pipelineId, IMAGE_INFO_S out_info, SENSOR_MODULE_INFO* sensor_info)
{
    int ret = 0;
    VI_DEV_ATTR_S viDevAttr = {};
    VI_CHN_ATTR_S viChnAttr = {};
    int devId = pipelineId;
    int chnId = pipelineId;

    viDevAttr.enWorkMode = CAM_VI_WORK_MODE_RAWDUMP;
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

    return 0;
}

int viisp_vi_onlyrawdump_streamOn(int pipelineId)
{
    int ret = 0;
    int devId = pipelineId;
    int chnId = pipelineId;

    ret = ASR_VI_EnableDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_EnableDev failed, ret = %d", ret);
    }

    ret = ASR_VI_EnableBayerDump(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_EnableBayerDump failed, ret = %d", ret);
    }

    return ret;
}

int viisp_vi_onlyrawdump_streamOff(int pipelineId)
{
    int ret = 0;
    int devId = pipelineId;
    int chnId = pipelineId;

    ret = ASR_VI_DisableDev(devId);
    if (ret != 0) {
        CLOG_ERROR("ASR_VI_DisableDev failed, ret = %d", ret);
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

/**********************************************************************************/
static int viisp_isp_fw_test_init(int firmwareId)
{
    int val = 0;
    ISP_AE_INFO_S data = {0};
    int ret = 0;

    ret = ASR_ISP_LoadSettingFile(firmwareId, ispInputFwFile[firmwareId]);
    if (ret) {
        CLOG_ERROR("ASR_ISP_LoadSettingFile failed\n");
    }

    ret = ASR_ISP_GetFWPara(firmwareId, "CAEMFirmwareFilter", "m_nAEStatMode", 0, 0, &val);
    if (ret) {
        CLOG_ERROR("ASR_ISP_GetFWPara CAEMFirmwareFilter:m_nAEStatMode failed\n");
    } else {
        CLOG_INFO("ASR_ISP_GetFWPara CAEMFirmwareFilter:m_nAEStatMode val: %d", val);
    }
    ret = ASR_ISP_SetFwPara(firmwareId, "CAEMFirmwareFilter", "m_nAEStatMode", 0, 0, 1);
    if (ret) {
        CLOG_ERROR("ASR_ISP_SetFwPara CAEMFirmwareFilter:m_nAEStatMode failed\n");
    } else {
        CLOG_INFO("ASR_ISP_SetFwPara CAEMFirmwareFilter:m_nAEStatMode OK\n");
    }
    ret = ASR_ISP_GetFWPara(firmwareId, "CAEMFirmwareFilter", "m_nAEStatMode", 0, 0, &val);
    if (ret) {
        CLOG_ERROR("ASR_ISP_GetFWPara CAEMFirmwareFilter:m_nAEStatMode failed\n");
    } else {
        CLOG_INFO("ASR_ISP_GetFWPara CAEMFirmwareFilter:m_nAEStatMode val: %d", val);
    }

    data.aeMode = ISP_AE_MODE_AUTO;
    ret = ASR_ISP_SetEffectParams(firmwareId, ISP_EFFECT_CMD_S_AE_MODE, &data, sizeof(data));
    if (ret) {
        CLOG_ERROR("ASR_ISP_SetEffectParams ISP_EFFECT_CMD_S_AE_MODE failed\n");
    } else {
        CLOG_INFO("ASR_ISP_SetEffectParams ISP_EFFECT_CMD_S_AE_MODE val: %d", val);
    }

    return ret;
}

static int viisp_isp_fw_test_deinit(int firmwareId)
{
    int ret = 0;

    ret = ASR_ISP_SaveSettingFile(firmwareId, ispOutputFwFile[firmwareId]);
    if (ret) {
        CLOG_ERROR("ASR_ISP_SaveSettingFile failed\n");
    }
    ret = ASR_ISP_SaveIndividualSettingFile(firmwareId, "CAEMFirmwareFilter",
                                            ispIndividualSettingFile[firmwareId]);
    if (ret) {
        CLOG_ERROR("ASR_ISP_SaveIndividualSettingFile failed\n");
    }
    return ret;
}

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

#ifdef EN_ISP_TUNING
    viisp_isp_fw_test_init(firmwareId);
#endif

    ret = ASR_ISP_SetFrameinfoCallback(firmwareId, callback);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_SetFrameinfoCallback failed, ret = %d", __func__, ret);
        return ret;
    }

    ret = ASR_ISP_SetFps(firmwareId, sensor_info->sensor_cfg->minFps, sensor_info->sensor_cfg->maxFps);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_SetFps failed, ret = %d", __func__, ret);
        return ret;
    }

    ret = ASR_ISP_Init(firmwareId);
    if (ret != 0) {
        CLOG_ERROR("%s: ASR_ISP_Init %d failed, ret = %d", __func__, firmwareId, ret);
        return ret;
    }

#ifdef EN_ISP_TUNING
    uint32_t addr = 0;
    uint32_t cur = 0;
    uint32_t value2 = 0;
    uint32_t mask = 0xffffffff;

    //ISP input raw image info, [28:16] height, [12:0] width
    addr = 0xc0231700;
    ret = ASR_ISP_GetRegister(addr, &cur);
    if (ret) {
        CLOG_ERROR("ASR_ISP_GetRegister addr: 0x%x failed\n", addr);
        return ret;
    } else {
        CLOG_INFO("ASR_ISP_GetRegister addr: 0x%x value:0x%x (%dx%d)\n",
                   addr, cur, cur & 0x1fff, (cur >> 16) & 0x1fff);
    }
    value2 = (out_info.width & 0x1fff) | ((out_info.height & 0x1fff) << 16);
    ret = ASR_ISP_SetRegister(addr, value2, mask);
    if (ret) {
        CLOG_ERROR("ASR_ISP_SetRegister addr: 0x%x failed\n", addr);
        return ret;
    } else {
        CLOG_INFO("ASR_ISP_SetRegister addr: 0x%x value:0x%x OK\n", addr, value2);
    }
    ret = ASR_ISP_GetRegister(addr, &value2);
    if (ret) {
        CLOG_ERROR("ASR_ISP_GetRegister addr: 0x%x failed\n", addr);
        return ret;
    } else {
        CLOG_INFO("ASR_ISP_GetRegister addr: 0x%x value:0x%x (%dx%d)\n",
                   addr, value2, value2 & 0x1fff, (value2 >> 16) & 0x1fff);
    }
    ret = ASR_ISP_SetRegister(addr, cur, mask);
    if (ret) {
        CLOG_ERROR("ASR_ISP_SetRegister addr: 0x%x failed\n", addr);
        return ret;
    } else {
        CLOG_INFO("ASR_ISP_SetRegister addr: 0x%x value:0x%x OK\n", addr, cur);
    }
#endif

    return ret;
}

int viisp_isp_deinit(int firmwareId, int sensorId)
{
    ISP_SENSOR_ATTR_S stSensorAttr = {};
    int ret = 0;

#ifdef EN_ISP_TUNING
    viisp_isp_fw_test_deinit(firmwareId);
#endif

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