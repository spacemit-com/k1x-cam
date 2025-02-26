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
#include <math.h>

#include "spm_comm_cam.h"
#include "cam_sensor.h"

#define SENSOR_MAGIC 0x415
#define IMX415_NAME "imx415"
#define USE_12BIT 0

static const unsigned int imx415_reg_addr_byte = I2C_16BIT; /*byte width of the sensor register address*/
static const unsigned int imx415_reg_data_byte = I2C_8BIT;  /*byte width of sensor register data*/

static struct regval_tab stream_on_regs[] = {
    {0x3000, 0x00},
    {0x3002, 0x00},
};

static struct regval_tab stream_off_regs[] = {
    {0x3000, 0x01},
};

static struct regval_tab stream_soft_reset_regs[] = {
    // {0x0103, 0x01},
};

static struct regval_tab color_bar_regs[] = {
    // {0x0601, 0x02},
};

#define IMX415_VTS_ADJUST     (0) /* vts - max_exposure*/
// #define IMX415_VTS_LINES_MAX  (0x08CA)
#define IMX415_VTS_LINES_MAX  (0xffff)
#define IMX415_EXPO_LINES_MIN (0x0008)

#define IMX415_VTS_ADDR_H16 (0x3026)
#define IMX415_VTS_ADDR_H   (0x3025)
#define IMX415_VTS_ADDR_L   (0x3024)
#define IMX415_EXPO_H16     (0x3052)
#define IMX415_EXPO_H       (0x3051)
#define IMX415_EXPO_L       (0x3050)
#define IMX415_AGAIN_L      (0x3090)
#define IMX415_AGAIN_H      (0x3091)

#define IMX415_GROUP_ACCESS (0x3001)

/*******************************************************************/
static int imx415_write_register(void* handle, uint16_t regAddr, uint16_t value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = imx415_reg_addr_byte;
    reg_data.val_len = imx415_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = value;
    ret = sensor_write_register(sensor_context->devId, &reg_data);

    return ret;
}

static int imx415_read_register(void* handle, uint16_t regAddr, uint16_t* value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = imx415_reg_addr_byte;
    reg_data.val_len = imx415_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = 0;
    ret = sensor_read_register(sensor_context->devId, &reg_data);
    if (!ret) {
        *value = reg_data.tab.val;
    }

    return ret;
}

static int imx415_write_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

#if 1
    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = imx415_reg_addr_byte;
    reg_table_data.val_len = imx415_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_write_burst_register(sensor_context->devId, &reg_table_data);
#else
    {
        int i;
        for (i = 0; i < reg_table_num; i++) {
            imx415_write_register(handle, reg_table[i].reg, reg_table[i].val);
        }
    }
#endif
    return ret;
}

#if 0
static int imx415_read_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_table_data.addr = sensor_context->work_info.i2c_addr;
    reg_table_data.reg_len = imx415_reg_addr_byte;
    reg_table_data.val_len = imx415_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_read_burst_register(sensor_context->devId, &reg_table_data);

    return ret;
}
#endif

/*******************************************************************/
/*isp sensor function*/
static int imx415_sensor_write_reg(void* snsHandle, uint32_t regAddr, uint32_t value)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = imx415_write_register(snsHandle, regAddr, value);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int imx415_sensor_group_reg_start(void* snsHandle)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    imx415_write_register(snsHandle, IMX415_GROUP_ACCESS, 1);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int imx415_sensor_group_reg_done(void* snsHandle)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    imx415_write_register(snsHandle, IMX415_GROUP_ACCESS, 0x00);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int imx415_sensor_get_isp_default(void* snsHandle, uint32_t u32ChanelId, uint32_t camScene,
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

static int imx415_sensor_get_isp_black_level(void* snsHandle, uint32_t u32ChanelId,
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

static int imx415_sensor_get_reg_info(void* snsHandle, ISP_SENSOR_REGS_INFO_S* pstSensorRegsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t i = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorRegsInfo);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (false == sensor_context->syncInit) {
        sensor_context->sensorRegs[0].u8CfgDelayMax = 1;
        sensor_context->sensorRegs[0].u32RegNum = 8;
        sensor_context->sensorRegs[0].stSensorComBus.s8I2cDev = sensor_context->twsi_no;

        for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
            sensor_context->sensorRegs[0].astI2cData[i].bUpdate = true;
            sensor_context->sensorRegs[0].astI2cData[i].u8DevAddr = sensor_context->i2c_addr;
            sensor_context->sensorRegs[0].astI2cData[i].u32AddrWidth = imx415_reg_addr_byte;
            sensor_context->sensorRegs[0].astI2cData[i].u32DataWidth = imx415_reg_data_byte;
        }
        sensor_context->sensorRegs[0].astI2cData[0].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[0].u32RegAddr = IMX415_EXPO_L;  // exposure time
        sensor_context->sensorRegs[0].astI2cData[1].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[1].u32RegAddr = IMX415_EXPO_H;  // exposure time
        sensor_context->sensorRegs[0].astI2cData[2].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[2].u32RegAddr = IMX415_EXPO_H16;  // exposure time
        sensor_context->sensorRegs[0].astI2cData[3].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[3].u32RegAddr = IMX415_AGAIN_L;  // analog gain
        sensor_context->sensorRegs[0].astI2cData[4].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[4].u32RegAddr = IMX415_AGAIN_H;  // analog gain
        sensor_context->sensorRegs[0].astI2cData[5].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[5].u32RegAddr = IMX415_VTS_ADDR_L;  // VTS
        sensor_context->sensorRegs[0].astI2cData[6].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[6].u32RegAddr = IMX415_VTS_ADDR_H;  // VTS
        sensor_context->sensorRegs[0].astI2cData[7].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[7].u32RegAddr = IMX415_VTS_ADDR_H16;  // VTS
        sensor_context->syncInit = true;
    } else {
        for (i = 0; i < (sensor_context->sensorRegs[0].u32RegNum); i++) {
            if (sensor_context->sensorRegs[0].astI2cData[i].u32Data
                == sensor_context->sensorRegs[1].astI2cData[i].u32Data) {
                sensor_context->sensorRegs[0].astI2cData[i].bUpdate = false;
            } else {
                sensor_context->sensorRegs[0].astI2cData[i].bUpdate = true;
            }
        }
    }

    memcpy(pstSensorRegsInfo, &sensor_context->sensorRegs[0], sizeof(ISP_SENSOR_REGS_INFO_S));
    memcpy(&sensor_context->sensorRegs[1], &sensor_context->sensorRegs[0], sizeof(ISP_SENSOR_REGS_INFO_S));
    sensor_context->vts[1] = sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int imx415_sensor_dump_info(void* snsHandle)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t vts = 0, exp_time = 0;
    uint32_t again = 0, dgain = 0;
    uint16_t reg_val_h, reg_val_l;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    imx415_read_register(snsHandle, IMX415_VTS_ADDR_H, &reg_val_h);
    imx415_read_register(snsHandle, IMX415_VTS_ADDR_L, &reg_val_l);
    vts = (reg_val_h << 8) | reg_val_l;
    imx415_read_register(snsHandle, IMX415_EXPO_H, &reg_val_h);
    imx415_read_register(snsHandle, IMX415_EXPO_L, &reg_val_l);
    exp_time = (reg_val_h << 8) | reg_val_l;
    imx415_read_register(snsHandle, IMX415_AGAIN_L, &reg_val_h);
    again = reg_val_h;
    // imx415_read_register(snsHandle, IMX415_DGAIN_GR_H, &reg_val_h);
    // imx415_read_register(snsHandle, IMX415_DGAIN_GR_L, &reg_val_l);
    // dgain = ((reg_val_h & 0x3) << 8) | reg_val_l;
    pthread_mutex_lock(&sensor_context->apiLock);
    CLOG_INFO("imx415 regs(vts=%d,exptime=%d,again=0x%x,dain =0x%x),struct(initVTS=%d,initFps=%f,vts=%d,expline=%d)",
        vts, exp_time, again, dgain, sensor_context->initVTS, sensor_context->initFps, sensor_context->vts[0],
        sensor_context->hdrIntTime[0] * 1000 / sensor_context->lineTime);
    pthread_mutex_unlock(&sensor_context->apiLock);

    return ret;
}

/*ae function*/
static int imx415_sensor_get_ae_default(void* snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AE_DEFAULT_S* pstSensorAeDft)
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

    // pstSensorAeDft->maxAnaGain = 0xF80;  // 15.5x Q8 format
    // pstSensorAeDft->minAnaGain = 0x100;  // 1x Q8 format
    // pstSensorAeDft->maxTGain = 0x3f8 * pstSensorAeDft->maxAnaGain / 0x100;  // Q8 format; maxDgain * maxAnaGain
    // pstSensorAeDft->minTGain = 0x100 * pstSensorAeDft->minAnaGain / 0x100;  // Q8 format; minDgain * minAnaGain

    pstSensorAeDft->initSceneLuma = sensor_context->init_3a_attr.initSceneLuma[u32ChanelId];
    pstSensorAeDft->initSceneLux = sensor_context->init_3a_attr.initSceneLux[u32ChanelId];

    pstSensorAeDft->initExpTime = exp_time;
    pstSensorAeDft->initAnaGain = again;
    pstSensorAeDft->initDGain = dgain;
    pstSensorAeDft->initTGain = pstSensorAeDft->initAnaGain * pstSensorAeDft->initDGain / 0x1000;

    pstSensorAeDft->maxDelayCfg = 1;
    pstSensorAeDft->minDelayCfg = 0;

    /* uint : us */
    // pstSensorAeDft->maxExpTime = (pstSensorState->initVTS - IMX415_VTS_ADJUST) * sensor_context->lineTime / 1000;
    // pstSensorAeDft->minExpTime = IMX415_EXPO_LINES_MIN * sensor_context->lineTime / 1000;

    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int imx415_sensor_get_expotime_by_fps(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t max_expotime = 0;
    uint32_t vts = 0;
    float minFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / IMX415_VTS_LINES_MAX;
    if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps))
        vts = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    else {
        CLOG_ERROR("Not support Fps: %f, minFps, sensor_context->maxFps: (%f, %f), sensor_context->minVTS: %x", f32Fps, minFps, sensor_context->maxFps, sensor_context->minVTS);
        max_expotime = -EINVAL;
        goto out;
    }

    max_expotime = (vts - IMX415_VTS_ADJUST) * sensor_context->lineTime / 1000;  // us
out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return max_expotime;
}

static int imx415_sensor_fps_set(void* snsHandle, float f32Fps)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t lines;
    float minFps;
    uint32_t expLine = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / IMX415_VTS_LINES_MAX;
    if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps))
        lines = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    else {
        CLOG_ERROR("Not support Fps: %f, minFps, sensor_context->maxFps: (%f, %f), sensor_context->minVTS: %x", f32Fps, minFps, sensor_context->maxFps, sensor_context->minVTS);
        ret = -1;
        goto out;
    }
    sensor_context->initVTS = lines;
    sensor_context->initFps = f32Fps;
    sensor_context->vts[0] = sensor_context->initVTS;
    sensor_context->sensorRegs[0].astI2cData[5].u32Data = LOW_8BITS(sensor_context->vts[0]);
    sensor_context->sensorRegs[0].astI2cData[6].u32Data = HIGH_8BITS(sensor_context->vts[0]);
    sensor_context->sensorRegs[0].astI2cData[7].u32Data = HIGH_8BITS(HIGH_8BITS(sensor_context->vts[0]));
out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

#define VMAX 2250
static int imx415_sensor_expotime_update(void* snsHandle, uint32_t u32ChanelId, uint32_t u32ExpoTime,
                                          ISP_SENSOR_VTS_INFO_S* pstSensorVtsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t shutter = 0;
    uint32_t integration_time = 0;
    uint32_t SHR0 = 0, SHR0_tmp;
    float Toffset = 1.79;  // us

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);

    shutter = u32ExpoTime * 1000 / sensor_context->lineTime;  // u32ExpoTime unit: us

    // SHR0 = sensor_context->initVTS - (integration_time * 1000 - 2680) * sensor_context->lineTime; //bit 12: Toffset=2.68us

// u32ExpoTime = sensor_context->vts[0]*sensor_context->lineTime / 1000  - SHR0 * sensor_context->lineTime / 1000 + Toffset;

    SHR0 = (sensor_context->vts[0]*sensor_context->lineTime / 1000 + Toffset - u32ExpoTime ) * 1000 / sensor_context->lineTime;

    SHR0_tmp = SHR0;

    if (SHR0 < 8)
        SHR0 = 8;
    else if (SHR0 >= VMAX - 4)
        SHR0 = VMAX - 4;

    sensor_context->hdrIntTime[u32ChanelId] = integration_time * sensor_context->lineTime / 1000;

    // if (SHR0_tmp < 8)
    //     sensor_context->vts[0] = expLine + IMX415_VTS_ADJUST;
    // else
    //     sensor_context->vts[0] = sensor_context->initVTS;

    // sensor_context->sensorRegs[0].astI2cData[3].u32Data = LOW_8BITS(sensor_context->vts[0]);
    // sensor_context->sensorRegs[0].astI2cData[4].u32Data = HIGH_8BITS(sensor_context->vts[0]);
// SHR0=90;
    sensor_context->sensorRegs[0].astI2cData[0].u32Data = LOW_8BITS(SHR0);
    sensor_context->sensorRegs[0].astI2cData[1].u32Data = HIGH_8BITS(SHR0);
    sensor_context->sensorRegs[0].astI2cData[2].u32Data = (SHR0>>16 & 0xf);

    pstSensorVtsInfo->snsLineTime = sensor_context->lineTime;
    pstSensorVtsInfo->snsVts = sensor_context->vts[0];
    pstSensorVtsInfo->snsFps = sensor_context->initFps * sensor_context->initVTS / sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);
//	printf("exp ttime: %d us, L:%d, SHR0_tmp:%d, SHR0:%d\n", u32ExpoTime, integration_time, SHR0_tmp, SHR0);

    return 0;
}
// static int imx415_sensor_gain_update(void* snsHandle, uint32_t u32ChanelId, uint32_t* pAgainVal, uint32_t* pDgainVal)
// {
//     SENSOR_CONTEXT_S* sensor_context = NULL;
//     int ret = 0;
//     uint32_t AGain_Reg, DGain_Reg = 0;

//     SENSORS_CHECK_PARA_POINTER(snsHandle);
//     SENSORS_CHECK_PARA_POINTER(pAgainVal);
//     SENSORS_CHECK_PARA_POINTER(pDgainVal);
//     sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
//     SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

// 	uint32_t tval = *pAgainVal;
//     // float gain_db;
//     //从isp发下来的gain转成db = 20 log(*pAgainVal/256)

//     pthread_mutex_lock(&sensor_context->apiLock);
//     // gain_db = 20 * log(*pAgainVal/256);

//     // if (gain_db > 30.0) //30x (256x16 is 16x)
//     //     gain_db = 30.0;

//     // AGain_Reg = (uint32_t)(gain_db * 10 / 3);
//     if (*pAgainVal > 0x1E00) //30x (256x16 is 16x)
//         AGain_Reg = 0x0064;
//     else //AGain_Reg = 20log(*pAgainVal/256) * 10 / 3 , reg: 0~240
//         AGain_Reg = *pAgainVal * 10 / 768;// AGain_Reg = 200 * log(*pAgainVal/256) / 3;

//     sensor_context->sensorRegs[0].astI2cData[3].u32Data = LOW_8BITS(AGain_Reg);     // bit[7:0] = Again[7:0]
//     sensor_context->sensorRegs[0].astI2cData[4].u32Data = 0;             // bit[15:8] = Again[8]

//     *pAgainVal = LOW_8BITS(AGain_Reg) * 768 / 10;  // Q8

//     // *pAgainVal = pow(10, ((AGain_Reg) * 3 / 200)) * 256;  // Q8
//     *pDgainVal = 4096;  // Q8 -> Q12
//     pthread_mutex_unlock(&sensor_context->apiLock);
// printf("again: %x (%x), AGain_Reg: %x\n", *pAgainVal, tval, AGain_Reg);
//     return ret;
//     pthread_mutex_lock(&sensor_context->apiLock);
//     gain_db = 20 * log10(*pAgainVal/256);

//     if (gain_db > 30.0) //30x (256x16 is 16x)
//         gain_db = 30.0;

//     AGain_Reg = (uint32_t)(gain_db * 10 / 3);

//     sensor_context->sensorRegs[0].astI2cData[3].u32Data = LOW_8BITS(AGain_Reg);     // bit[7:0] = Again[7:0]
//     sensor_context->sensorRegs[0].astI2cData[4].u32Data = 0;             // bit[15:8] = Again[8]

//     *pAgainVal = pow(10, ((AGain_Reg + 160) * 3 / 200));  // Q8
//     *pDgainVal = 4096;  // Q8 -> Q12
//     pthread_mutex_unlock(&sensor_context->apiLock);
// printf("again: %x (%x), AGain_Reg: %x, db:%f\n", *pAgainVal, tval, AGain_Reg, gain_db);
// }

//only 30.0db
static int gain_table[101] = {
     256,     264,     274,     283,     293,     304,     314,     326,
     337,     349,     361,     374,     387,     401,     415,     429,
     444,     460,     476,     493,     510,     528,     547,     566,
     586,     607,     628,     650,     673,     697,     721,     746,
     773,     800,     828,     857,     887,     918,     951,     984,
    1019,    1054,    1092,    1130,    1170,    1211,    1253,    1297,
    1343,    1390,    1439,    1490,    1542,    1596,    1652,    1710,
    1771,    1833,    1897,    1964,    2033,    2104,    2178,    2255,
    2334,    2416,    2501,    2589,    2680,    2774,    2872,    2973,
    3077,    3185,    3297,    3413,    3533,    3657,    3786,    3919,
    4057,    4199,    4347,    4500,    4658,    4822,    4991,    5167,
    5348,    5536,    5731,    5932,    6141,    6356,    6580,    6811,
    7050,    7298,    7555,    7820,    8095
};
static int imx415_sensor_gain_update(void* snsHandle, uint32_t u32ChanelId, uint32_t* pAgainVal, uint32_t* pDgainVal)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int i, ret = 0;
    uint32_t AGain_Reg, DGain_Reg = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pAgainVal);
    SENSORS_CHECK_PARA_POINTER(pDgainVal);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	uint32_t tval = *pAgainVal;
    double gain_db;
    uint32_t gain_val;
    //从isp发下来的gain转成db = 20 log(*pAgainVal/256)

    pthread_mutex_lock(&sensor_context->apiLock);

    for (i = 0; i < ARRAY_SIZE(gain_table); i++) {
        if (tval < gain_table[i])
            break;
    }

    if (i >= ARRAY_SIZE(gain_table)) {
        gain_val = gain_table[ARRAY_SIZE(gain_table) - 1];
    } else if (i == 0) {
        gain_val = gain_table[i];
    } else {
        gain_val = gain_table[i - 1];
    }

    gain_db = 20 * log10(gain_val/256.0);
    AGain_Reg = (uint32_t)(gain_db * 10 / 3);

    sensor_context->sensorRegs[0].astI2cData[3].u32Data = LOW_8BITS(AGain_Reg);     // bit[7:0] = Again[7:0]
    sensor_context->sensorRegs[0].astI2cData[4].u32Data = 0;             // bit[15:8] = Again[8]

    *pAgainVal = gain_val;  // Q8
    *pDgainVal = 4096;  // Q8 -> Q12
    pthread_mutex_unlock(&sensor_context->apiLock);
//printf("again: %x (%x), AGain_Reg: %x, db:%f, i:%d\n", *pAgainVal, tval, AGain_Reg, gain_db, i);
    return ret;
}

static int imx415_get_aelib_default_settings(void* snsHandle, uint32_t u32ChanelId,
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
static int imx415_sensor_get_awb_default(void* snsHandle, uint32_t u32ChanelId,
                                          ISP_SENSOR_AWB_DEFAULT_S* pstSensorAwbDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSORS_CHECK_PARA_POINTER(pstSensorAwbDft);
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId] != 0 && sensor_context->init_3a_attr.initTint[u32ChanelId] != 0) {
        pstSensorAwbDft->initCorrelationCT = sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId];
        pstSensorAwbDft->initTint = sensor_context->init_3a_attr.initTint[u32ChanelId];
    }
    pthread_mutex_unlock(&sensor_context->apiLock);

    return ret;
}

static int imx415_get_awblib_default_settings(void* snsHandle, uint32_t u32ChanelId,
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

static int imx415_power_on(SENSOR_CONTEXT_S* sensor_context)
{
    SENSORS_CHECK_PARA_POINTER(sensor_context);

    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);

    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1800000);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1);
    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DVDD, 1200000);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DVDD, 1);
    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_AFVDD, 2800000);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AFVDD, 1);
    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_AVDD, 2800000);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 1);

    usleep(100);

    sensor_set_mclk_enable(sensor_context->devId, 1);

    sensor_set_mclk_rate(sensor_context->devId, 37125000);

    usleep(100);

    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 1);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 1);
    usleep(1000);

    CLOG_INFO("finish power on 22222222");

    return 0;
}

/*******************************************************************/
static int imx415_init(void** pHandle, SENSOR_CUSTOM_S snr_custom)
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
    sensor_context->name = IMX415_NAME;
    sensor_context->devId = sns_id;
    sensor_context->i2c_addr = sns_addr;
    sensor_context->magic = SENSOR_MAGIC;
    pthread_mutex_init(&sensor_context->apiLock, NULL);

    sensor_hw_init(sensor_context->devId);
    imx415_power_on(sensor_context);
    sensor_get_hw_info(sensor_context->devId, &sensor_hw_info);
    sensor_context->twsi_no = sensor_hw_info.twsi_no;

    *pHandle = sensor_context;
    return 0;
}

static int imx415_deinit(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    sensor_context->magic = 0;
    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        imx415_write_burst_register(handle, stream_off_regs, ARRAY_SIZE(stream_off_regs));
        sensor_context->stream_on_flag = 0;
    }

    sensor_hw_reset(sensor_context->devId);
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

static int imx415_global_config(void* handle, SENSOR_WORK_INFO_S* work_info)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(work_info);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        CLOG_ERROR("sensor global config must be done before stream on");
        ret = -EPERM;
        goto out;
    }
    memcpy(&sensor_context->work_info, work_info, sizeof(SENSOR_WORK_INFO_S));
    memset(&sensor_context->init_3a_attr, 0x00, sizeof(SENSOR_INIT_ATTR_S));

    if (sensor_context->work_info.mclk == 27000000 || sensor_context->work_info.mclk == 24000000) {
        sensor_set_mclk_rate(sensor_context->devId, sensor_context->work_info.mclk);
        CLOG_INFO("change imx415 mclk to %dhz", sensor_context->work_info.mclk);
    }

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

    ret = imx415_write_burst_register(handle, sensor_context->work_info.setting_table,
                                       sensor_context->work_info.setting_table_size);
    if (ret) {
        goto out;
    }

out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int imx415_set_param(void* handle, const SENSOR_INIT_ATTR_S* init_attr)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    memcpy(&sensor_context->init_3a_attr, init_attr, sizeof(SENSOR_INIT_ATTR_S));

    return 0;
}

static int imx415_stream_on(void* handle)
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
        imx415_write_register(handle, sensor_context->sensorRegs[0].astI2cData[i].u32RegAddr,
                               sensor_context->sensorRegs[0].astI2cData[i].u32Data);
    }
#if 0	//read sensor reg setting

    fprintf(stderr, "-----------------start read sensor reg----------------\n");
    int iii;
    struct regval_tab* sensor_table = NULL;
    sensor_table = (struct regval_tab*)calloc(sensor_context->sensorRegs[0].u32RegNum, sizeof(struct regval_tab));
    if (NULL == sensor_table) {
        CLOG_ERROR("sensor_table malloc memory failed!");
        ret = -ENOMEM;
    }
    for (iii = 0; iii < sensor_context->sensorRegs[0].u32RegNum; iii++) {
        sensor_table[iii].reg = sensor_context->sensorRegs[0].astI2cData[iii].u32RegAddr;
        sensor_table[iii].val = 0;
    }

    struct cam_burst_i2c_data reg_table_data;
    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = imx415_reg_addr_byte;
    reg_table_data.val_len = imx415_reg_data_byte;
    reg_table_data.tab = sensor_table;
    reg_table_data.num = sensor_context->sensorRegs[0].u32RegNum;
    ret = sensor_read_burst_register(sensor_context->devId, &reg_table_data);
    if (ret) {
        CLOG_INFO("read sensor_table register failed: %s\n", strerror(errno));
    }

    for (iii = 0; iii < sensor_context->sensorRegs[0].u32RegNum; iii++) {
        if ((sensor_table[iii].reg != sensor_context->sensorRegs[0].astI2cData[iii].u32RegAddr)
            || (sensor_table[iii].val != sensor_context->sensorRegs[0].astI2cData[iii].u32Data)) {
            fprintf(stderr, "read sensor (0x%04x, 0x%04x) != (0x%04x, 0x%04x)\n", 
            sensor_table[iii].reg, sensor_table[iii].val,sensor_context->sensorRegs[0].astI2cData[iii].u32RegAddr,sensor_context->sensorRegs[0].astI2cData[iii].u32Data);
        } else if ((sensor_table[iii].reg == sensor_context->work_info.setting_table[iii].reg)
            || (sensor_table[iii].val == sensor_context->work_info.setting_table[iii].val)) {
            fprintf(stderr, "read sensor (0x%04x, 0x%04x) == (0x%04x, 0x%04x)\n", 
            sensor_table[iii].reg, sensor_table[iii].val,sensor_context->sensorRegs[0].astI2cData[iii].u32RegAddr,sensor_context->sensorRegs[0].astI2cData[iii].u32Data);
        } else {
            fprintf(stderr, "read sensor (0x%04x, 0x%04x) ?? (0x%04x, 0x%04x)\n", 
            sensor_table[iii].reg, sensor_table[iii].val,sensor_context->sensorRegs[0].astI2cData[iii].u32RegAddr,sensor_context->sensorRegs[0].astI2cData[iii].u32Data);
        }
    }
    free(sensor_table);
    fprintf(stderr, "-----------------finish read sensor reg----------------\n");

#endif

    ret = imx415_write_burst_register(handle, stream_on_regs, ARRAY_SIZE(stream_on_regs));

    sensor_context->stream_on_flag = 1;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int imx415_stream_off(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = imx415_write_burst_register(handle, stream_off_regs, ARRAY_SIZE(stream_off_regs));

    sensor_context->stream_on_flag = 0;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int imx415_get_ops(void* handle, ISP_SENSOR_REGISTER_S* pSensorFuncOps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(pSensorFuncOps);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pSensorFuncOps->snsHandle = handle;
    pSensorFuncOps->stSensorFunc.pfn_sensor_write_reg = imx415_sensor_write_reg;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_default = imx415_sensor_get_isp_default;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_black_level = imx415_sensor_get_isp_black_level;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_reg_info = imx415_sensor_get_reg_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_dump_info = imx415_sensor_dump_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_start = imx415_sensor_group_reg_start;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_done = imx415_sensor_group_reg_done;

    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_ae_default = imx415_sensor_get_ae_default;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_fps_set = imx415_sensor_fps_set;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_expotime_by_fps = imx415_sensor_get_expotime_by_fps;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_expotime_update = imx415_sensor_expotime_update;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_gain_update = imx415_sensor_gain_update;
    pSensorFuncOps->stSensorAeFunc.pfn_get_aelib_default_settings = imx415_get_aelib_default_settings;

    pSensorFuncOps->stSensorAwbFunc.pfn_sensor_get_awb_default = imx415_sensor_get_awb_default;
    pSensorFuncOps->stSensorAwbFunc.pfn_get_awblib_default_settings = imx415_get_awblib_default_settings;
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int imx415_detect_sensor(void* handle, SENSOR_VENDOR_ID_S* vendor_id)
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
    reg_table_data.reg_len = imx415_reg_addr_byte;
    reg_table_data.val_len = imx415_reg_data_byte;
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

SENSOR_OBJ_S imx415Obj = {
    .name = IMX415_NAME,
    .pfnInit = imx415_init,
    .pfnDeinit = imx415_deinit,
    .pfnGloablConfig = imx415_global_config,
    .pfnSetParam = imx415_set_param,
    .pfnStreamOn = imx415_stream_on,
    .pfnStreamOff = imx415_stream_off,
    .pfnGetSensorOps = imx415_get_ops,
    .pfnDetectSns = imx415_detect_sensor,
    .pfnWriteReg = imx415_write_register,
    .pfnReadReg = imx415_read_register,
};
