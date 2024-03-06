/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _VIISP_COMMON_H_
#define _VIISP_COMMON_H_

#include <stdlib.h>
#include <sys/time.h>

#include "spm_cam_isp.h"
#include "spm_cam_vi.h"
#include "cam_interface.h"
#include "sensor_common.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */
typedef enum {
    VIISP_WORKMODE_INVALID = 0,
    VIISP_WORKMODE_RAW,
    VIISP_WORKMODE_ONLINE_RAW,
    VIISP_WORKMODE_OFFLINE,
    VIISP_WORKMODE_OFFLINE_RAW,
    VIISP_WORKMODE_NUM,
} VIISP_WORKMODE;

#define AUTO_FRAME_NUM (500)
#define DUMP_FRAME_NUM (AUTO_FRAME_NUM / 2)

int viisp_vi_init();
int viisp_vi_deInit();
int viisp_vi_online_config(int pipelineId, IMAGE_INFO_S out_info, SENSOR_MODULE_INFO* sensor_info);
int viisp_vi_online_streamOn(int pipelineId);
int viisp_vi_online_streamOff(int pipelineId);
int viisp_set_vi_callback(int chnId, int32_t (*callback)(uint32_t nChn, VI_IMAGE_BUFFER_S* vi_buffer));
int viisp_vi_queueBuffer(int chnId, IMAGE_BUFFER_S* buffer);
int viisp_vi_offline_config(int pipelineId, IMAGE_INFO_S out_info, IMAGE_INFO_S in_info, bool isCaptureMode);
int viisp_vi_offline_streamOn(int pipelineId);
int viisp_vi_offline_streamOff(int pipelineId);

int viisp_isp_init(int firmwareId, IMAGE_INFO_S out_info, SENSOR_MODULE_INFO* sensor_info,
                   GetFrameInfoCallBack callback, bool isCaptureMode);
int viisp_isp_deinit(int firmwareId, int sensorId);
int viisp_isp_streamOn(int firmwareId);
int viisp_isp_streamOff(int firmwareId);
int viisp_isp_queueBuffer(int firmwareId, IMAGE_BUFFER_S* buffer);
int viisp_isp_triggerRawCapture(int firmwareId, IMAGE_BUFFER_S* buffer);
void ispout_framerate_stat(uint32_t nChn);

int viisp_vi_onlyrawdump_config(int pipelineId, IMAGE_INFO_S out_info, SENSOR_MODULE_INFO* sensor_info);
int viisp_vi_onlyrawdump_streamOn(int pipelineId);
int viisp_vi_onlyrawdump_streamOff(int pipelineId);

int viisp_vi_offline_preview_config(int pipelineId, ISP_PUB_ATTR_S *pstIspPubAttr);
int viisp_vi_offline_preview_streamOn(int pipelineId);
int viisp_vi_offline_preview_streamOff(int pipelineId);

int viisp_isp_offline_preview_init(int firmwareId, ISP_PUB_ATTR_S *stIspPubAttr,
                                   ISP_OFFLINE_ATTR_S *stOfflineAttr, GetFrameInfoCallBack callback);
int viisp_isp_offline_preview_deinit(int firmwareId);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_LIST_H_ */
