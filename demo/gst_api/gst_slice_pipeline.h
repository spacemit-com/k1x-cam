/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#ifndef _GST_SLICE_PIPELINE_H_
#define _GST_SLICE_PIPELINE_H_

#include "config.h"
#include "gst_cam_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

void slice_pipeline_release_buffer(IMAGE_BUFFER_S* outputBuf, int index);
int slice_pipeline_start(struct gstParam *para, struct testConfig *config);
int slice_pipeline_stop(struct gstParam *para);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* _GST_SLICE_PIPELINE_H_ */
