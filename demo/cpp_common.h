/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved.
 */

#ifndef _CPP_COMMON_H_
#define _CPP_COMMON_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#include "cam_cpp.h"
#include "cam_interface.h"

int cpp_init(int groupId, IMAGE_INFO_S config, CppCallback callback);
int cpp_deInit(int groupId);
int cpp_start(int groupId);
int cpp_stop(int groupId);
int cpp_post_buffer(int groupId, const IMAGE_BUFFER_S *inputBuf, const IMAGE_BUFFER_S *outputBuf, int32_t frameId,
                    FRAME_INFO_S *frameInfo);
int cpp_load_fw_settingfile(int groupId, char *filename);
int cpp_save_fw_settingfile(int groupId, char *filename);
int cpp_test_fw_infs(int groupId);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _CAM_LIST_H_ */
