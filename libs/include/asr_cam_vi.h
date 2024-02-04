/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _ASR_CAM_VI_H_
#define _ASR_CAM_VI_H_

#include "asr_comm_vi.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

int32_t ASR_VI_SetDevAttr(uint32_t nDev, VI_DEV_ATTR_S *pstDevAttr);

int32_t ASR_VI_GetDevAttr(uint32_t nDev, VI_DEV_ATTR_S *pstDevAttr);

int32_t ASR_VI_EnableDev(uint32_t nDev);

int32_t ASR_VI_DisableDev(uint32_t nDev);

int32_t ASR_VI_FlushDev(uint32_t nDev);

int32_t ASR_VI_SetChnAttr(uint32_t nChn, VI_CHN_ATTR_S *pstAttr);

int32_t ASR_VI_GetChnAttr(uint32_t nChn, VI_CHN_ATTR_S *pstAttr);

int32_t ASR_VI_SetCallback(uint32_t nChn, int32_t (*callback)(uint32_t nChn, VI_IMAGE_BUFFER_S *vi_buffer));

int32_t ASR_VI_EnableChn(uint32_t nChn);

int32_t ASR_VI_DisableChn(uint32_t nChn);

int32_t ASR_VI_Init(void);

int32_t ASR_VI_Deinit(void);

int32_t ASR_VI_EnableBayerDump(uint32_t nDev);

int32_t ASR_VI_DisableBayerDump(uint32_t nDev);

int32_t ASR_VI_SetBayerReadAttr(uint32_t nDev, const VI_BAYER_READ_ATTR_S *pstBayerReadAttr);

int32_t ASR_VI_GetBayerReadAttr(uint32_t nDev, const VI_BAYER_READ_ATTR_S *pstBayerReadAttr);

int32_t ASR_VI_EnableBayerRead(uint32_t nDev);

int32_t ASR_VI_DisableBayerRead(uint32_t nDev);

int32_t ASR_VI_ChnQueueBuffer(uint32_t nChn, IMAGE_BUFFER_S *camBuf);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif
