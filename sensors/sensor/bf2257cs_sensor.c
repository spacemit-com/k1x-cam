/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cam_sensor.h"

#define SENSOR_MAGIC 0x2257
#define BF2257CS_NAME "bf2257cs"
static const unsigned int bf2257cs_reg_addr_byte = I2C_8BIT; /*byte width of the sensor register address*/
static const unsigned int bf2257cs_reg_data_byte = I2C_8BIT; /*byte width of sensor register data*/

static struct regval_tab stream_on_regs[] = {
    {0x01, 0x4b},
    {0xf3, 0x00},
    {0xd0, 0x00},
    {0x01, 0x43},
};

static struct regval_tab stream_off_regs[] = {
    {0xf3, 0x01},
};
static struct regval_tab stream_soft_reset_regs[] = {
    {0xf2, 0x01},
};
// static struct regval_tab color_bar_regs[] = {
//     {0x8c, 0x01},  // 0: no pattern; 1:solid; 2:full color bar; 3:fade to gray color bar
// };

#define BF2257CS_VTS_ADJUST     (0)
#define BF2257CS_VB_MAX         (0x1fff)
#define BF2257CS_VTS_OFFSET     (1234)
#define BF2257CS_VTS_LINES_MAX  (BF2257CS_VB_MAX + BF2257CS_VTS_OFFSET)
#define BF2257CS_EXPO_LINES_MIN (0x0001)

#define BF2257CS_EXPO_H    (0x6b)
#define BF2257CS_EXPO_L    (0x6c)
#define BF2257CS_AGAIN_0   (0x6a)
#define BF2257CS_VB_H      (0x07)
#define BF2257CS_VB_L      (0x06)

typedef enum {
    BF2257CS_INFO_PAGE,
    BF2257CS_INFO_AGAIN_0,
    BF2257CS_INFO_VB_H,
    BF2257CS_INFO_VB_L,
    BF2257CS_INFO_EXP_H,
    BF2257CS_INFO_EXP_L,
    BF2257CS_INFO_MAX
} BF2257CS_SENSOR_INFO_E;

/*******************************************************************/
static int bf2257cs_write_register(void* handle, uint16_t regAddr, uint16_t value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = bf2257cs_reg_addr_byte;
    reg_data.val_len = bf2257cs_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = value;
    ret = sensor_write_register(sensor_context->devId, &reg_data);

    return ret;
}

static int bf2257cs_read_register(void* handle, uint16_t regAddr, uint16_t* value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = bf2257cs_reg_addr_byte;
    reg_data.val_len = bf2257cs_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = 0;
    ret = sensor_read_register(sensor_context->devId, &reg_data);
    if (!ret) {
        *value = reg_data.tab.val;
    }

    return ret;
}

static int bf2257cs_write_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

#if 1
    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = bf2257cs_reg_addr_byte;
    reg_table_data.val_len = bf2257cs_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_write_burst_register(sensor_context->devId, &reg_table_data);
#else
    {
        int i;
        for (i = 0; i < reg_table_num; i++) {
            bf2257cs_write_register(handle, reg_table[i].reg, reg_table[i].val);
        }
    }
#endif
    return ret;
}

#if 0
static int bf2257cs_read_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_table_data.addr = sensor_context->work_info.i2c_addr;
    reg_table_data.reg_len = bf2257cs_reg_addr_byte;
    reg_table_data.val_len = bf2257cs_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_read_burst_register(sensor_context->devId, &reg_table_data);

    return ret;
}
#endif

/*******************************************************************/
/*isp sensor function*/
static int bf2257cs_sensor_write_reg(void* snsHandle, uint32_t regAddr, uint32_t value)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = bf2257cs_write_register(snsHandle, regAddr, value);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int bf2257cs_sensor_get_isp_default(void* snsHandle, uint32_t u32ChanelId, uint32_t camScene,
                                          ISP_SENSOR_DEFAULT_S* pstDef)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstDef);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    memset(pstDef, 0, sizeof(ISP_SENSOR_DEFAULT_S));
    if (camScene >= CAM_ISP_SCENE_INVALID) {
        CLOG_WARNING("%s: invalid camera scene:%d for isp get sensor default", __FUNCTION__, camScene);
        pstDef->pstIspDefaultSetting = NULL;
    } else {
        pstDef->pstIspDefaultSetting = sensor_context->work_info.pstIspDefaultSettings[camScene];
    }

#if 0
    // Transfer isp tunning file to struct like this if you need.
    if (sensor_context->work_info.image_mode == SENSOR_LINEAR_MODE) {
        if (0 == u32ChanelId)
            pstDef->pstIspDefaultSetting = NULL;
        else if (1 == u32ChanelId)
            pstDef->pstIspDefaultSetting = NULL;
    } else {
        if (0 == u32ChanelId)
            pstDef->pstIspDefaultSetting = NULL;
        else if (0 == u32ChanelId)
            pstDef->pstIspDefaultSetting = NULL;
    }
#endif
    pthread_mutex_unlock(&sensor_context->apiLock);
    CLOG_DEBUG("%s: isp get sensor default, u32ChanelId %d", __FUNCTION__, u32ChanelId);

    return 0;
}

static int bf2257cs_sensor_get_isp_black_level(void* snsHandle, uint32_t u32ChanelId,
                                              ISP_SENSOR_BLACK_LEVEL_S* pstBlackLevel)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int i = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstBlackLevel);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = false;
    if (SENSOR_LINEAR_MODE == sensor_context->work_info.image_mode) {
        for (i = 0; i < 4; i++) {
            pstBlackLevel->sensorBlackLevel[i] = 259; /*10bit,0x40*/
            pstBlackLevel->bitDepth = 12;
        }
    }
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int bf2257cs_sensor_get_reg_info(void* snsHandle, ISP_SENSOR_REGS_INFO_S* pstSensorRegsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t i = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorRegsInfo);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (false == sensor_context->syncInit) {
        sensor_context->sensorRegs[0].u8CfgDelayMax = 2;
        sensor_context->sensorRegs[0].u32RegNum = BF2257CS_INFO_MAX;
        sensor_context->sensorRegs[0].stSensorComBus.s8I2cDev = sensor_context->twsi_no;

        for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
            sensor_context->sensorRegs[0].astI2cData[i].bUpdate = true;
            sensor_context->sensorRegs[0].astI2cData[i].u8DevAddr = sensor_context->i2c_addr;
            sensor_context->sensorRegs[0].astI2cData[i].u32AddrWidth = bf2257cs_reg_addr_byte;
            sensor_context->sensorRegs[0].astI2cData[i].u32DataWidth = bf2257cs_reg_data_byte;
        }
        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_EXP_L].u8DelayFrmNum = 2;
        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_EXP_L].u32RegAddr = BF2257CS_EXPO_L;  // exposure time[7:0]   page 0
        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_EXP_H].u8DelayFrmNum = 2;
        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_EXP_H].u32RegAddr = BF2257CS_EXPO_H;  // exposure time[13:8]

        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_AGAIN_0].u8DelayFrmNum = 2;
        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_AGAIN_0].u32RegAddr = BF2257CS_AGAIN_0;  // Again

        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_VB_H].u8DelayFrmNum = 2;
        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_VB_H].u32RegAddr = BF2257CS_VB_H;
        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_VB_L].u8DelayFrmNum = 2;
        sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_VB_L].u32RegAddr = BF2257CS_VB_L;

        sensor_context->syncInit = true;
    } else {
        for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
            if (sensor_context->sensorRegs[0].astI2cData[i].u32Data
                == sensor_context->sensorRegs[1].astI2cData[i].u32Data) {
                sensor_context->sensorRegs[0].astI2cData[i].bUpdate = false;
            } else {
                sensor_context->sensorRegs[0].astI2cData[i].bUpdate = true;
            }
        }
        for (i = 1; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
            if (sensor_context->sensorRegs[0].astI2cData[i].bUpdate) {
                // sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_PAGE].bUpdate = true;
                break;
            }
        }
    }

    memcpy(pstSensorRegsInfo, &sensor_context->sensorRegs[0], sizeof(ISP_SENSOR_REGS_INFO_S));
    memcpy(&sensor_context->sensorRegs[1], &sensor_context->sensorRegs[0], sizeof(ISP_SENSOR_REGS_INFO_S));
    sensor_context->vts[1] = sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

/*ae function*/
static int bf2257cs_sensor_get_ae_default(void* snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AE_DEFAULT_S* pstSensorAeDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t exp_time = 0;
    uint32_t again = 0, dgain = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorAeDft);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    exp_time = sensor_context->work_info.exp_time[u32ChanelId];
    again = sensor_context->work_info.again[u32ChanelId];
    dgain = sensor_context->work_info.dgain[u32ChanelId];

    // pstSensorAeDft->maxAnaGain = 0xf80;  // 15.5x Q8 format
    // pstSensorAeDft->minAnaGain = 0x100;  // 1x Q8 format
    // pstSensorAeDft->maxTGain = 0x3f8 * pstSensorAeDft->maxAnaGain / 0x100;      // max dgain = 1016/256 = 3.96875x
    // pstSensorAeDft->minTGain = 1 * 0x100 * pstSensorAeDft->minAnaGain / 0x100;  // min dgain = 1x

    pstSensorAeDft->initSceneLuma = sensor_context->init_3a_attr.initSceneLuma[u32ChanelId];
    pstSensorAeDft->initSceneLux = sensor_context->init_3a_attr.initSceneLux[u32ChanelId];

    pstSensorAeDft->initExpTime = exp_time;
    pstSensorAeDft->initAnaGain = again;
    pstSensorAeDft->initDGain = dgain;
    pstSensorAeDft->initTGain = pstSensorAeDft->initAnaGain * pstSensorAeDft->initDGain / 0x1000;

    pstSensorAeDft->maxDelayCfg = 2;
    pstSensorAeDft->minDelayCfg = 2;

    // pstSensorAeDft->maxExpTime = (pstSensorState->initVTS - BF2257CS_VTS_ADJUST) * sensor_context->lineTime / 1000;
    // pstSensorAeDft->minExpTime = BF2257CS_EXPO_LINES_MIN * sensor_context->lineTime / 1000;         //us

    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int bf2257cs_sensor_get_expotime_by_fps(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t max_expotime = 0;
    uint32_t vts = 0;
    float minFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / BF2257CS_VTS_LINES_MAX;
    if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps))
        vts = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    else {
        CLOG_ERROR("Not support Fps: %f", f32Fps);
        max_expotime = -EINVAL;
        goto out;
    }

    max_expotime = (vts - BF2257CS_VTS_ADJUST) * sensor_context->lineTime / 1000;  // us
out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return max_expotime;
}

static int bf2257cs_sensor_fps_set(void* snsHandle, float f32Fps)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t lines, vb = 0;
    float minFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / BF2257CS_VTS_LINES_MAX;
    if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps))
        lines = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    else {
        CLOG_ERROR("Not support Fps: %f", f32Fps);
        ret = -1;
        goto out;
    }
    sensor_context->initVTS = lines;
    sensor_context->initFps = f32Fps;
    sensor_context->vts[0] = sensor_context->initVTS;
    vb = sensor_context->vts[0] - BF2257CS_VTS_OFFSET;
    sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_VB_H].u32Data = HIGH_8BITS(vb);
    sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_VB_L].u32Data = LOW_8BITS(vb);

out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int bf2257cs_sensor_expotime_update(void* snsHandle, uint32_t u32ChanelId, uint32_t u32ExpoTime,
                                          ISP_SENSOR_VTS_INFO_S* pstSensorVtsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t expLine = 0, vb = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    expLine = u32ExpoTime * 1000 / sensor_context->lineTime;  // u32ExpoTime unit: us
    expLine = expLine >> 2;
    expLine = expLine << 2;
    expLine = (expLine < BF2257CS_EXPO_LINES_MIN) ? BF2257CS_EXPO_LINES_MIN : expLine;
    expLine = (expLine > (BF2257CS_VTS_LINES_MAX - BF2257CS_VTS_ADJUST)) ? (BF2257CS_VTS_LINES_MAX - BF2257CS_VTS_ADJUST)
                                                                       : expLine;
    sensor_context->hdrIntTime[u32ChanelId] = expLine * sensor_context->lineTime / 1000;

    if (expLine > (sensor_context->initVTS - BF2257CS_VTS_ADJUST))
        sensor_context->vts[0] = expLine + BF2257CS_VTS_ADJUST;
    else
        sensor_context->vts[0] = sensor_context->initVTS;

    vb = sensor_context->vts[0] - BF2257CS_VTS_OFFSET;
    sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_VB_H].u32Data = HIGH_8BITS(vb);
    sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_VB_L].u32Data = LOW_8BITS(vb);
    sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_EXP_L].u32Data = LOW_8BITS(expLine);
    sensor_context->sensorRegs[0].astI2cData[BF2257CS_INFO_EXP_H].u32Data = HIGH_8BITS(expLine);

    pstSensorVtsInfo->snsLineTime = sensor_context->lineTime;
    pstSensorVtsInfo->snsVts = sensor_context->vts[0];
    pstSensorVtsInfo->snsFps = sensor_context->initFps * sensor_context->initVTS / sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int bf2257cs_sensor_gain_update(void* snsHandle, uint32_t u32ChanelId, uint32_t* pAgainVal, uint32_t* pDgainVal)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;
    uint32_t temp, AGain_Reg = 0, AGain_Reg_temp = 0;
    ISP_SENSOR_REGS_INFO_S* sensorRegs = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pAgainVal);
    SENSORS_CHECK_PARA_POINTER(pDgainVal);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);
    sensorRegs = &sensor_context->sensorRegs[0];

    pthread_mutex_lock(&sensor_context->apiLock);
    AGain_Reg = *pAgainVal << 2; // Q8->Q10
    // if (AGain_Reg < 0x400)
    //     AGain_Reg = 0x400;
    if (AGain_Reg > 0x2000) //8x (256x8 is 8x)
        AGain_Reg = 0x2000;

    AGain_Reg = AGain_Reg * 19 / 16;
	if(AGain_Reg <= 8 * 0x400) 
		AGain_Reg_temp = ((AGain_Reg << 4) / 0x400) - 1;
	else
		AGain_Reg_temp = ((AGain_Reg << 4) / 0x400) - 2;	

    sensorRegs->astI2cData[BF2257CS_INFO_AGAIN_0].u32Data = LOW_8BITS(AGain_Reg_temp);

    *pDgainVal = 4096;
    *pAgainVal = AGain_Reg >> 2;
    pthread_mutex_unlock(&sensor_context->apiLock);
// printf("again: %x, AGain_Reg: %x\n", *pAgainVal, AGain_Reg);
    return ret;
}

static int bf2257cs_get_aelib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                              AE_LIB_DEFAULT_SETTING_S** ppstAeLibDefault)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSORS_CHECK_PARA_POINTER(ppstAeLibDefault);
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    *ppstAeLibDefault = NULL;

#if 1
    // Transfer ae lib tunning file to struct like this if you need.
    if (SENSOR_HDR_MODE == sensor_context->work_info.image_mode) {
        if (0 == u32ChanelId)
            *ppstAeLibDefault = NULL;

        /* spm aelib needn't set setting for pipe1(short exposure),because setting of short exposure has already existed
         * in pipe0, perhaps you can set the same value of pipe0 to pipe1.
         */
    } else {
        if (0 == u32ChanelId)
            *ppstAeLibDefault = NULL;
        else if (1 == u32ChanelId)
            *ppstAeLibDefault = NULL;
    }
#endif
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

/*awb function*/
static int bf2257cs_sensor_get_awb_default(void* snsHandle, uint32_t u32ChanelId,
                                          ISP_SENSOR_AWB_DEFAULT_S* pstSensorAwbDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSORS_CHECK_PARA_POINTER(pstSensorAwbDft);
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId] != 0
        && sensor_context->init_3a_attr.initTint[u32ChanelId] != 0) {
        pstSensorAwbDft->initCorrelationCT = sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId];
        pstSensorAwbDft->initTint = sensor_context->init_3a_attr.initTint[u32ChanelId];
    }
    pthread_mutex_unlock(&sensor_context->apiLock);

    return ret;
}

static int bf2257cs_get_awblib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                               AWB_LIB_DEFAULT_SETTING_S** ppstAwbLibDefault)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(ppstAwbLibDefault);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    *ppstAwbLibDefault = NULL;

#if 1
    // Transfer awb lib tunning file to struct like this if you need.
    if (SENSOR_HDR_MODE == sensor_context->work_info.image_mode) {
        if (0 == u32ChanelId)
            *ppstAwbLibDefault = NULL;
        else if (1 == u32ChanelId)
            *ppstAwbLibDefault = NULL;
    } else {
        if (0 == u32ChanelId)
            *ppstAwbLibDefault = NULL;
        else if (1 == u32ChanelId)
            *ppstAwbLibDefault = NULL;
    }
#endif
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}
static int bf2257cs_power_on(SENSOR_CONTEXT_S* sensor_context)
{
    SENSORS_CHECK_PARA_POINTER(sensor_context);

    // sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);

    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1800000);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1);
    // dvdd is connectd to iovdd

    usleep(1000);

    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_AVDD, 2800000);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 1);
    usleep(1000);

    sensor_set_mclk_rate(sensor_context->devId, 24000000);
    sensor_set_mclk_enable(sensor_context->devId, 1);
    usleep(1000);

    // sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 1);
    // usleep(500);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 1);
    usleep(10000);

    return 0;
}

static int bf2257cs_power_off(SENSOR_CONTEXT_S* sensor_context)
{
    SENSORS_CHECK_PARA_POINTER(sensor_context);
    // sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 1);
    sensor_set_mclk_enable(sensor_context->devId, 0);
    usleep(100);

    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 0);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 0);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);
    // sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);

    return 0;
}
/*******************************************************************/
static int bf2257cs_init(void** pHandle, SENSOR_CUSTOM_S snr_custom)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_sensor_info sensor_hw_info;
    int sns_id = snr_custom.dev_id;
    uint8_t sns_addr = snr_custom.i2c_addr;

    SENSORS_CHECK_PARA_POINTER(pHandle);

    sensor_context = (SENSOR_CONTEXT_S*)calloc(1, sizeof(SENSOR_CONTEXT_S));
    if (NULL == sensor_context) {
        CLOG_ERROR("%s: sensor_context malloc memory failed!", __FUNCTION__);
        return -ENOMEM;
    }
    sensor_context->name = BF2257CS_NAME;
    sensor_context->devId = sns_id;
    sensor_context->i2c_addr = sns_addr;
    sensor_context->magic = SENSOR_MAGIC;
    pthread_mutex_init(&sensor_context->apiLock, NULL);

    sensor_hw_init(sensor_context->devId);
    bf2257cs_power_on(sensor_context);
    sensor_get_hw_info(sensor_context->devId, &sensor_hw_info);
    sensor_context->twsi_no = sensor_hw_info.twsi_no;

    *pHandle = sensor_context;
    return 0;
}

static int bf2257cs_deinit(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    sensor_context->magic = 0;
    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        bf2257cs_write_burst_register(handle, stream_off_regs, ARRAY_SIZE(stream_off_regs));
        sensor_context->stream_on_flag = 0;
    }
    bf2257cs_power_off(sensor_context);
    sensor_hw_exit(sensor_context->devId);
    pthread_mutex_unlock(&sensor_context->apiLock);

    pthread_mutex_destroy(&sensor_context->apiLock);
    if (sensor_context) {
        memset(sensor_context, 0, sizeof(SENSOR_CONTEXT_S));
        free(sensor_context);
        sensor_context = NULL;
    }
    return 0;
}

static int bf2257cs_global_config(void* handle, SENSOR_WORK_INFO_S* work_info)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(work_info);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        CLOG_ERROR("%s: sensor global config must be done before stream on", __FUNCTION__);
        ret = -EPERM;
        goto out;
    }
    memcpy(&sensor_context->work_info, work_info, sizeof(SENSOR_WORK_INFO_S));
    memset(&sensor_context->init_3a_attr, 0x00, sizeof(SENSOR_INIT_ATTR_S));

    sensor_context->initVTS = sensor_context->work_info.vts;
    sensor_context->initFps = sensor_context->work_info.f32maxFps;
    sensor_context->minVTS = sensor_context->work_info.vts;
    sensor_context->maxFps = sensor_context->work_info.f32maxFps;
    sensor_context->lineTime = sensor_context->work_info.linetime;
    sensor_context->vts[0] = sensor_context->initVTS;
    sensor_context->vts[1] = sensor_context->vts[0];
    memset(sensor_context->hdrIntTime, 0, sizeof(sensor_context->hdrIntTime));
    memset(sensor_context->sensorRegs, 0, 2 * sizeof(ISP_SENSOR_REGS_INFO_S));
    sensor_context->syncInit = 0;

    ret = bf2257cs_write_burst_register(handle, stream_soft_reset_regs, ARRAY_SIZE(stream_soft_reset_regs));
    if (ret) {
        goto out;
    }
    usleep(5000);

    ret = bf2257cs_write_burst_register(handle, sensor_context->work_info.setting_table,
                                       sensor_context->work_info.setting_table_size);
    if (ret) {
        goto out;
    }

out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int bf2257cs_set_param(void* handle, const SENSOR_INIT_ATTR_S* init_attr)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    memcpy(&sensor_context->init_3a_attr, init_attr, sizeof(SENSOR_INIT_ATTR_S));

    return 0;
}

static int bf2257cs_stream_on(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;
    uint32_t i = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sensor_mipi_clock_set(sensor_context->devId, sensor_context->work_info.mipi_clock);
    if (ret)
        return ret;
    for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
        bf2257cs_write_register(handle, sensor_context->sensorRegs[0].astI2cData[i].u32RegAddr,
                               sensor_context->sensorRegs[0].astI2cData[i].u32Data);
    }
    bf2257cs_write_register(handle, 0x00, 0x0d);
    ret = bf2257cs_write_burst_register(handle, stream_on_regs, ARRAY_SIZE(stream_on_regs));
    usleep(10*1000);
    sensor_context->stream_on_flag = 1;
    pthread_mutex_unlock(&sensor_context->apiLock);


    return ret;
}

static int bf2257cs_stream_off(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = bf2257cs_write_burst_register(handle, stream_off_regs, ARRAY_SIZE(stream_off_regs));
    usleep(25*1000);
    sensor_context->stream_on_flag = 0;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int bf2257cs_get_ops(void* handle, ISP_SENSOR_REGISTER_S* pSensorFuncOps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(pSensorFuncOps);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pSensorFuncOps->snsHandle = handle;
    pSensorFuncOps->stSensorFunc.pfn_sensor_write_reg = bf2257cs_sensor_write_reg;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_default = bf2257cs_sensor_get_isp_default;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_black_level = bf2257cs_sensor_get_isp_black_level;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_reg_info = bf2257cs_sensor_get_reg_info;

    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_ae_default = bf2257cs_sensor_get_ae_default;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_fps_set = bf2257cs_sensor_fps_set;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_expotime_by_fps = bf2257cs_sensor_get_expotime_by_fps;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_expotime_update = bf2257cs_sensor_expotime_update;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_gain_update = bf2257cs_sensor_gain_update;
    pSensorFuncOps->stSensorAeFunc.pfn_get_aelib_default_settings = bf2257cs_get_aelib_default_settings;

    pSensorFuncOps->stSensorAwbFunc.pfn_sensor_get_awb_default = bf2257cs_sensor_get_awb_default;
    pSensorFuncOps->stSensorAwbFunc.pfn_get_awblib_default_settings = bf2257cs_get_awblib_default_settings;
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int bf2257cs_detect_sensor(void* handle, SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;
    struct regval_tab* vendor_id_table = NULL;
    int i = 0;
    struct cam_burst_i2c_data reg_table_data;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(vendor_id);
    SENSORS_CHECK_PARA_POINTER(vendor_id->id_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    vendor_id_table = (struct regval_tab*)calloc(vendor_id->id_table_size, sizeof(struct regval_tab));
    if (NULL == vendor_id_table) {
        CLOG_ERROR("vendor id table malloc memory failed!");
        ret = -ENOMEM;
        goto out;
    }
    for (i = 0; i < vendor_id->id_table_size; i++) {
        vendor_id_table[i].reg = vendor_id->id_table[i].reg;
        vendor_id_table[i].val = 0;
    }

    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = bf2257cs_reg_addr_byte;
    reg_table_data.val_len = bf2257cs_reg_data_byte;
    reg_table_data.tab = vendor_id_table;
    reg_table_data.num = vendor_id->id_table_size;
    ret = sensor_read_burst_register(sensor_context->devId, &reg_table_data);
    if (ret) {
        CLOG_INFO("read vendor id register failed: %s\n", strerror(errno));
        goto out;
    }

    for (i = 0; i < vendor_id->id_table_size; i++) {
        if ((vendor_id_table[i].reg != vendor_id->id_table[i].reg)
            || (vendor_id_table[i].val != vendor_id->id_table[i].val)) {
            CLOG_INFO("%s: detect sensor fail", __func__);
            ret = -1;
            break;
        }
    }
    if (ret == -1) {
        for (i = 0; i < vendor_id->id_table_size; i++) {
            CLOG_INFO("read sensor vendor id (0x%04x, 0x%04x)", vendor_id_table[i].reg, vendor_id_table[i].val);
        }
        for (i = 0; i < vendor_id->id_table_size; i++) {
            CLOG_INFO("valid sensor vendor id (0x%04x, 0x%04x)", vendor_id->id_table[i].reg,
                      vendor_id->id_table[i].val);
        }
    } else {
        CLOG_INFO("detect sensor%d success", sensor_context->devId);
    }

out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    if (vendor_id_table) {
        free(vendor_id_table);
        vendor_id_table = NULL;
    }

    return ret;
}

SENSOR_OBJ_S bf2257csObj = {
    .name = BF2257CS_NAME,
    .pfnInit = bf2257cs_init,
    .pfnDeinit = bf2257cs_deinit,
    .pfnGloablConfig = bf2257cs_global_config,
    .pfnSetParam = bf2257cs_set_param,
    .pfnStreamOn = bf2257cs_stream_on,
    .pfnStreamOff = bf2257cs_stream_off,
    .pfnGetSensorOps = bf2257cs_get_ops,
    .pfnDetectSns = bf2257cs_detect_sensor,
    .pfnWriteReg = bf2257cs_write_register,
    .pfnReadReg = bf2257cs_read_register,
};
