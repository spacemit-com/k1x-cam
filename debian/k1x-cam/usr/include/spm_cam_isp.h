/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 * *****************************************************************************
 * File Name     : spm_cam_isp.h
 * Description   : define API of isp used by external.
 */

#ifndef __SPM_CAM_ISP_API_H__
#define __SPM_CAM_ISP_API_H__

#include "spm_isp_comm.h"
#include "spm_isp_sensor_comm.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

int ASR_ISP_Construct(uint32_t pipelineID);
int ASR_ISP_Destruct(uint32_t pipelineID);
int ASR_ISP_GetFwFrameInfoSize();

/*
 * Function:
 *   register sensor to isp ctx pipeline.
 * Notice:
 *   must call this function before ASR_ISP_Init, isp need sensor callback to get setting.
 */
int ASR_ISP_RegSensorCallBack(uint32_t pipelineID, ISP_SENSOR_ATTR_S *pstSensorInfo,
                              ISP_SENSOR_REGISTER_S *pstRegister);
int ASR_ISP_UnRegSensorCallBack(uint32_t pipelineID, ISP_SENSOR_ATTR_S *pstSensorInfo);

int ASR_ISP_RegAfMotorCallBack(uint32_t pipelineID, ISP_AF_MOTOR_REGISTER_S *pstAfRegister);

/*
 * Function:
 *   set offline attribute to the isp ctx pipeline.
 * Notice:
 *  1.must call this function before ASR_ISP_Init.
 *  2.ISP consider the data comes from offline once this API is called.
 */
int ASR_ISP_EnableOfflineMode(uint32_t pipelineID, uint32_t enable, const ISP_OFFLINE_ATTR_S *pstOfflineAttr);

/*
 * Function:
 *   set public attribute to the channel of isp ctx pipeline.
 * Notice:
 *  1.must call this function before ASR_ISP_Init.
 *  2.the size of input and output must be equal for capture channel.
 */
int ASR_ISP_SetPubAttr(uint32_t pipelineID, uint32_t channelID, const ISP_PUB_ATTR_S *pstPubAttr);

/*
 * Function:
 *   set tuning file and scene of isp ctx pipeline.
 * Params:
 *   1.pTuningFile, the path and name of tuning file. It could be NULL, then isp doesn't load setting file.
 *   2.camScene, the scene of current camera, defined by @CAM_ISP_SCENE_E.
 *   3.pMergedLscProfile, the memory of otp, it always "int array[3][576]"
 *   4.mergedLscProfileCnt, the number of pExternalOtpProfile, must be 3 * 576.
 * Notice:
 *  1.must call this function before ASR_ISP_Init.
 *  2.The tuning file is the highest priority if it is valid.
 */
int ASR_ISP_SetTuningParams(uint32_t pipelineID, ISP_TUNING_ATTRS_S *pstTuningAttr);

/*
 * Function:
 *   set hardware pipeID to the channel of isp ctx pipeline.
 * Notice:
 *  1.must call this function before ASR_ISP_Init.
 *  2.the two channels(preview and capture) can work at any hardware pipe.
 */
int ASR_ISP_SetChHwPipeID(uint32_t pipelineID, uint32_t channelID, uint32_t hwPipeID);

/*
 * Function:
 * Init isp ctx pipeline with the params setted early.
 */
int ASR_ISP_Init(uint32_t pipelineID);

/*
 * Function:
 *   Deinit isp ctx pipeline, the params(public attribute,offline attribute and hw pipe id) except things which
 * are registered to pipeline are clear.
 */
int ASR_ISP_DeInit(uint32_t pipelineID);

int ASR_ISP_EnablePDAF(uint32_t pipelineID, uint32_t enable);

/*
 * Function:
 *   Set fps to isp pipeline.
 * Notice:
 *   This api is used after ASR_ISP_Init.
 */
int ASR_ISP_SetFps(uint32_t pipelineID, float fminFps, float fmaxFps);

int ASR_ISP_SetFrameinfoCallback(uint32_t pipelineID, GetFrameInfoCallBack callback);
int ASR_ISP_QueueFrameinfoBuffer(uint32_t pipelineID, IMAGE_BUFFER_S *pFrameInfoBuf);
int ASR_ISP_FlushFrameinfoBuffer(uint32_t pipelineID);

int ASR_ISP_Streamon(uint32_t pipelineID);
int ASR_ISP_Streamoff(uint32_t pipelineID);

int ASR_ISP_SetFwPara(uint32_t pipelineID, const char *paramter, const char *name, uint32_t row, uint32_t column,
                      int value);
int ASR_ISP_GetFWPara(uint32_t pipelineID, const char *paramter, const char *name, uint32_t row, uint32_t column,
                      int *pValue);

/*
 * Function:
 *  trigger isp pipeline to enter raw capture process.
 * Params:
 *  1.pipelineID: capture on which pipeline
 *  2.pFrameInfoBuf: frameinfo for this capture.
 *  3.hdrCapture: rawcapture for hdr capture.
 * Notice:
 *  must use the frameinfo according to the raw image.
 */
int ASR_ISP_TriggerRawCapture(uint32_t pipelineID, IMAGE_BUFFER_S *pFrameInfoBuf, uint32_t hdrCapture);

int ASR_ISP_SaveSettingFile(uint32_t pipelineID, const char *pFileName);
int ASR_ISP_LoadSettingFile(uint32_t pipelineID, const char *pFileName);
int ASR_ISP_SaveIndividualSettingFile(uint32_t pipelineID, const char *pFilterName, const char *pFileName);

int ASR_ISP_SetRegister(uint32_t addr, uint32_t value, uint32_t mask);
int ASR_ISP_GetRegister(uint32_t addr, uint32_t *pValue);
int ASR_ISP_SetEffectParams(uint32_t pipelineID, uint32_t effectCmd, void *pData, int dataSize);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif  //__SPM_CAM_ISP_API_H__
