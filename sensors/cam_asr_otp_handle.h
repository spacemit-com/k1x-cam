/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#ifndef _CAM_ASR_OTP_HANDLE_H_
#define _CAM_ASR_OTP_HANDLE_H_

#define INPUT_SENSOR_OTP_DATA_COUNT 576
#define MERGED_OPT_DATA_COUNT       3 * 576

#ifdef __cplusplus
extern "C" {
#endif /* extern "C" */

int cam_asr_otp_handle_ctx_init(int sns_id);
int cam_asr_otp_handle_ctx_deinit();

/*
 * Function:
 *   set sensor otp data to handle ctx
 * Params:
 *   pSensorOtpLscData: the memory stored by sensor otp lsc data, always is "int array[576]".
 *   dataCount: the number of sensor otp data in pSensorOtpLscData, must be 576.
 */
int cam_asr_otp_handle_ctx_set_lsc_params(int *pSensorOtpLscData, int dataCount);

/*
 * Function:
 *   process sensor otp data and return the result
 * Params:
 *   pMergedOtpLscData: the memory stored by merged otp lsc data, always is "int array [3][576]".
 *   dataCount: the number of otp data in pMergedOtpLscData, must be 3 * 576.
 */
int cam_asr_otp_handle_ctx_process_lsc_data(int *pMergedOtpLscData, int dataCount);

int cam_read_otp_lsc_profile_from_file(int *lsc_top_data);

#ifdef __cplusplus
}
#endif /* extern "C" */

#endif