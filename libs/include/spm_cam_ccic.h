/*
 * Copyright (C) 2024 SPACEMIT Micro Limited
 * All Rights Reserved.
 */

#ifndef _SPM_CAM_CCIC_H_
#define _SPM_CAM_CCIC_H_

#include "spm_comm_ccic.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

int32_t ASR_CCIC_SetDevAttr(uint32_t nDev, CCIC_DEV_ATTR_S *pstDevAttr);

int32_t ASR_CCIC_GetDevAttr(uint32_t nDev, CCIC_DEV_ATTR_S *pstDevAttr);

int32_t ASR_CCIC_EnableDev(uint32_t nDev);

int32_t ASR_CCIC_DisableDev(uint32_t nDev);

int32_t ASR_CCIC_FlushDev(uint32_t nDev);

int32_t ASR_CCIC_SetChnAttr(uint32_t nChn, CCIC_CHN_ATTR_S *pstAttr);

int32_t ASR_CCIC_GetChnAttr(uint32_t nChn, CCIC_CHN_ATTR_S *pstAttr);

int32_t ASR_CCIC_SetCallback(uint32_t nChn, int32_t (*callback)(uint32_t nChn, CCIC_IMAGE_BUFFER_S *vi_buffer));

int32_t ASR_CCIC_EnableChn(uint32_t nChn);

int32_t ASR_CCIC_DisableChn(uint32_t nChn);

int32_t ASR_CCIC_Init(void);

int32_t ASR_CCIC_Deinit(void);

int32_t ASR_CCIC_ChnQueueBuffer(uint32_t nChn, IMAGE_BUFFER_S *camBuf);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
