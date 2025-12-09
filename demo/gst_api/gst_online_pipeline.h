/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _GST_ONLINE_PIPELINE_H_
#define _GST_ONLINE_PIPELINE_H_

#include "gst_cam_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

#define MAX_BUFFER_RAWDUMP_NUM 5
#define MAX_BUFFER_NUM   4
#define MAX_PIPELINE_NUM 2
#define MAX_FIRMWARE_NUM 2
#define RAW8_DUMP_SIZE(w, h) ((w / 16 + (w % 16 ? 1 : 0)) * 16 * h)
#define RAW10_DUMP_SIZE(w, h) ((w / 12 + (w % 12 ? 1 : 0)) * 16 * h)
#define RAW12_DUMP_SIZE(w, h) ((w / 10 + (w % 10 ? 1 : 0)) * 16 * h)
#define RAW14_DUMP_SIZE(w, h) ((w / 8 + (w % 8 ? 1 : 0)) * 16 * h)
#define VRF_INFO_LEN (128)

void single_pipeline_online_release_buffer(IMAGE_BUFFER_S* outputBuf, int index);
int single_pipeline_online_start(struct gstParam *para, struct testConfig *config);
int single_pipeline_online_stop(struct gstParam *para);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _GST_ONLINE_PIPELINE_H_ */
