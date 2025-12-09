/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef __SPM_CAM_ISP_SENSOR_H__
#define __SPM_CAM_ISP_SENSOR_H__

#include <stdbool.h>
#include <stdint.h>

#include "ISPGlobalDefine.h"

#ifdef __cplusplus
extern "C" {
#endif /* End of #ifdef __cplusplus */

#define ISP_MAX_SENSOR_REGS 32

typedef struct spmISP_SENSOR_ATTR_S {
    uint32_t u32SensorId;
} ISP_SENSOR_ATTR_S;

typedef struct spmISP_FIRMWARE_VERSION_S {
    int revisionNumber;
} ISP_FIRMWARE_VERSION_S;

typedef struct spmISP_DEFAULT_SETTING {
    const ISP_FIRMWARE_VERSION_S stFwVersion;
    const _isp_fw_top_t stTopPara;
    const _isp_fw_ae_t stAePara;
    const _isp_fw_digital_gain_t stDigitalGainPara;
    const int s32EnableWbc;
    const int s32EnableStretch;
    const _isp_fw_cmc_t stCmcPara;
    const _isp_fw_bpc_t stBpcPara;
    const _isp_fw_lsc_t stLscPara;
    const _isp_fw_demosaic_t stDemosaicPara;
    const _isp_fw_raw_denoise_t stRawDenoisePara;
    const _isp_fw_afm_t stAfmPara;
    const _isp_fw_pdc_t stPdcPara;
    const _isp_fw_pdf_t stPdfPara;
    const _pdaf_alg_param_t stPdafPara;
    const _isp_fw_wb_t stWbPara;
    const _isp_ct_calc_t stCtCalcPara;
    const _isp_fw_rgb2yuv_t stRgb2YuvPara;
    const _isp_fw_sde_t stSdePara;
    const _isp_fw_gtm_after_t stCurvePara;
    const _isp_fw_ltm_t stLtmPara;
    const _isp_fw_uvdenoise_t stUvdenoise;
    const _isp_ee_lite_fw_param_t stEelitePara;
    const int s32EnableDithering;
    const _isp_fw_formatter_t stFormaterPara;
    const _isp_fw_eis_t stEisPara;
    const _isp_ae_attr_t stAeSoftPara;
    const _isp_af_attr_t stAfSoftPara;
    const _isp_awb_attr_t stAwbSoftPara;
} ISP_DEFAULT_SETTING_S;

typedef struct spmAE_LIB_DEFAULT_SETTING {
    const _isp_ae_attr_t *pstAeAttr;
} AE_LIB_DEFAULT_SETTING_S;

typedef struct spmAWB_LIB_DEFAULT_SETTING {
    const _isp_awb_attr_t *pstAwbAttr;
} AWB_LIB_DEFAULT_SETTING_S;

typedef struct spmISP_SENSOR_BLACK_LEVEL_S {
    bool bUpdate;
    uint32_t bitDepth;
    uint32_t sensorBlackLevel[4];
} ISP_SENSOR_BLACK_LEVEL_S;

typedef struct spmISP_SENSOR_IMAGE_MODE_S {
    uint16_t u16Width;
    uint16_t u16Height;
    float u16Fps;
    uint8_t u8SnsMode;
} ISP_SENSOR_IMAGE_MODE_S;

typedef struct spmISP_SENSOR_DEFAULT_S {
    const ISP_DEFAULT_SETTING_S *pstIspDefaultSetting;
} ISP_SENSOR_DEFAULT_S;

typedef struct spmISP_SENSOR_VTS_INFO_S {
    int snsLineTime; /*unit ns */
    int snsVts;
    float snsFps;
} ISP_SENSOR_VTS_INFO_S;

typedef struct spmISP_SENSOR_AE_DEFAULT_S {
    /* sensor start info at this time. */
    uint32_t initExpTime;  // unit:us
    uint32_t initAnaGain;  // Q8 format
    uint32_t initTGain;    // Q8 format
    uint32_t initDGain;    // Q12 format
    uint32_t initSceneLuma;
    uint32_t initSceneLux;
    /*
     * Maximum number of delayed frames sensor param is effective, which is used to ensure the synchronization between
     * sensor registers and ISP registers. For example, 1 means the param is effective at the next frame, 2 means at the
     * next two frames effect.
     */
    uint32_t maxDelayCfg;
    /*
     * Minemum number of delayed frames sensor param is effective. -1 invalid value.
     * Notice:
     *   It means the params effect immediately when set 0 to this field. In this case we must set the param at vblank.
     */
    int32_t minDelayCfg;
} ISP_SENSOR_AE_DEFAULT_S;

typedef struct spmISP_SENSOR_COMMBUS_S {
    int8_t s8I2cDev;  // i2c devno, < 0 means invalid
} ISP_SENSOR_COMMBUS_S;

typedef struct spmISP_SENSOR_I2C_DATA_S {
    bool bConfig;          /* sensor always set false when called by pfn_sensor_get_reg_info. */
    bool bUpdate;          /* TRUE: The sensor registers are written,FALSE: The sensor registers are not written*/
    uint8_t u8DelayFrmNum; /*Number of delayed frames for the sensor register*/
    uint32_t u32RegAddr;   /*Sensor register address*/
    uint32_t u32AddrWidth; /*Bit width of the sensor register address*/
    uint32_t u32Data;      /*Sensor register data*/
    uint32_t u32DataWidth; /*Bit width of sensor register value*/
    uint8_t u8DevAddr;     /*Sensor device address*/
} ISP_SENSOR_I2C_DATA_S;

typedef struct spmISP_SENSOR_REG_INFO_S {
    uint32_t u32RegNum;    /* Number of registers required when exposure results are written to the sensor */
    uint8_t u8CfgDelayMax; /*Maximum number of delayed frames, which is used to ensure the synchronization between
                              sensor registers and ISP registers*/
    ISP_SENSOR_COMMBUS_S stSensorComBus;
    ISP_SENSOR_I2C_DATA_S astI2cData[ISP_MAX_SENSOR_REGS];
} ISP_SENSOR_REGS_INFO_S;

typedef struct spmISP_SENSOR_FUNC_S {
    //    int (*pfn_sensor_get_otp_info)(void)
    int (*pfn_sensor_write_reg)(void *snsHandle, uint32_t regAddr, uint32_t value);
    int (*pfn_sensor_get_isp_default)(void *snsHandle, uint32_t u32ChanelId, uint32_t camScene,
                                      ISP_SENSOR_DEFAULT_S *pstDef);
    int (*pfn_sensor_get_isp_black_level)(void *snsHandle, uint32_t u32ChanelId,
                                          ISP_SENSOR_BLACK_LEVEL_S *pstBlackLevel);
    int (*pfn_sensor_get_reg_info)(void *snsHandle, ISP_SENSOR_REGS_INFO_S *pstSensorRegsInfo);
    int (*pfn_sensor_dump_info)(void *snsHandle);
    int (*pfn_sensor_group_regs_start)(void *snsHandle);
    int (*pfn_sensor_group_regs_done)(void *snsHandle);
} ISP_SENSOR_FUNC_S;

typedef struct spmISP_SENSOR_AE_FUNC_S {
    /* Notice:
     *   1. u32ChanelId means there is multi streams from sensor.
     *   2.HDR mode, channel 0 means long exposure while channel 1 indicates short exposure.
     *   3.RGBW mode, channel 0 means RGB while channel 1 indicates W channel.
     *   4.RGBIR mode, channel 0 means RGB while channel 1 indicates IR channel.
     * The functions which you must to deal channel 0 and channel 1 are:
     *   1.pfn_sensor_get_ae_default, the setting of channel 0 and channel 1 may be different.
     *   2.pfn_sensor_expotime_update, update exposure, channel 0 and channel 1 may have different values.
     *   3.pfn_sensor_gain_update, update again and dgain, channel 0 and channel 1 may have different values.
     */

    /*
     * function:
     *  get sensor about ae setting
     * Notice:
     *   In hdr mode, channel 0 means long exposure while channel 1 indicates short exposure.
     * return:
     *  success:0; failed:-1.
     */
    int (*pfn_sensor_get_ae_default)(void *snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AE_DEFAULT_S *pstSensorAeDft);

    /*
     * function:
     *  sensor set fps,the result must be save in your ISP_SENSOR_REGS_INFO_S, we will call pfn_sensor_get_reg_info.
     * return: success:0. return -1 if failed.
     */
    int (*pfn_sensor_fps_set)(void *snsHandle, float f32Fps);

    /*
     * function:
     *  get the sensor's max exposure time according to fps.
     * return:
     *  success:the the max exposure time at this fps.
     *  failed: -1.
     */
    int (*pfn_sensor_get_expotime_by_fps)(void *snsHandle, float f32Fps);

    /* while isp notify ae to update sensor regs, ae call these funcs. */
    /*
     * Params:
     *  pstSensorVtsInfo: the result(vts and fps) filled by sensor.
     * Function:
     *   transfer exposure time(us) to exposure line and vts, the result must be save in your ISP_SENSOR_REGS_INFO_S, we
     * will call pfn_sensor_get_reg_info to set to kernel. Notice: In hdr mode, channel 0 means long exposure while
     * channel 1 indicates short exposure. return: success,0; failed, -1.
     */
    int (*pfn_sensor_expotime_update)(void *snsHandle, uint32_t u32ChanelId, uint32_t u32ExpoTime,
                                      ISP_SENSOR_VTS_INFO_S *pstSensorVtsInfo);
    /*
     * Function:
     *   calculate the actual again and dgain which sensor can achieve and update the param, the result must be save in
     * your ISP_SENSOR_REGS_INFO_S, we will call pfn_sensor_get_reg_info to set to kernel. Param: uint32_t *pAgainVal:
     * Q8 format, the target again need to set, and update the actual value which sensor can achieve. uint32_t
     * *pDgainVal: Q12 format, the target dgain need to set, and update the actual value which sensor can achieve.
     * Notice:
     *   In hdr mode, channel 0 means long exposure while channel 1 indicates short exposure.
     * return:
     *   success, 0; failed, -1.
     *   Must fille pAgainVal and pDgainVal with sensor actual set value when success.
     */
    int (*pfn_sensor_gain_update)(void *snsHandle, uint32_t u32ChanelId, uint32_t *pAgainVal, uint32_t *pDgainVal);

    /*
     * function:
     *  get aelib setting according to pipelineID, just for spm aelib temporarily.
     * Notice:
     *   In hdr mode, channel 0 means long exposure while channel 1 indicates short exposure.
     * return:
     *  success:0; failed:-1.
     */
    int (*pfn_get_aelib_default_settings)(void *snsHandle, uint32_t u32ChanelId,
                                          AE_LIB_DEFAULT_SETTING_S **ppstAeLibDefault);
} ISP_SENSOR_AE_FUNC_S;

typedef struct spmISP_SENSOR_AWB_DEFAULT_S {
    int32_t initCorrelationCT; //Color temperature
    int32_t initTint; //Color tone whose value may negative
} ISP_SENSOR_AWB_DEFAULT_S;

typedef struct spmISP_SENSOR_AWB_FUNC_S {
    /*
     * function:
     *  get sensor about awb setting
     * Notice:
     *   In hdr mode, channel 0 means long exposure while channel 1 indicates short exposure.
     * return:
     *  success:0; failed:-1.
     */
    int (*pfn_sensor_get_awb_default)(void *snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AWB_DEFAULT_S *pstSensorAwbDft);

    /*
     * function:
     *  get awblib setting according to pipelineID, just for spm awblib temporarily.
     * Notice:
     *   In hdr mode, channel 0 means long exposure while channel 1 indicates short exposure.
     * return:
     *  success:0; failed:-1.
     */
    int (*pfn_get_awblib_default_settings)(void *snsHandle, uint32_t u32ChanelId,
                                           AWB_LIB_DEFAULT_SETTING_S **ppstAwbLibDefault);
} ISP_SENSOR_AWB_FUNC_S;

typedef struct spmISP_SENSOR_REGISTER_S {
    ISP_SENSOR_FUNC_S stSensorFunc;
    ISP_SENSOR_AE_FUNC_S stSensorAeFunc;
    ISP_SENSOR_AWB_FUNC_S stSensorAwbFunc;
    void *snsHandle;
} ISP_SENSOR_REGISTER_S;

typedef struct spmISP_AF_MOTOR_REGISTER_S {
    int (*pfn_af_motor_move)(void *vcmHandle, int position);
    void *vcmHandle;
} ISP_AF_MOTOR_REGISTER_S;

#ifdef __cplusplus
}
#endif /* extern "C" */
#endif
