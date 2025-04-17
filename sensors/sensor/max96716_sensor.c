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
#include "i2c_common.h"

#include "spm_comm_cam.h"
#include "cam_sensor.h"

#define SENSOR_MAGIC 0x415
#define MAX96716_NAME "max96716_max96717_imx556_spm"
#define USE_12BIT 0
static int i2c_fd;

static const unsigned int max96716_reg_addr_byte = I2C_16BIT; /*byte width of the sensor register address*/
static const unsigned int max96716_reg_data_byte = I2C_8BIT;  /*byte width of sensor register data*/

static struct regval_tab stream_on_regs[] = {
    {0x0313, 0x02}, 	//  CSI output enable
};

static struct regval_tab stream_off_regs[] = {
    {0x0313, 0x00}, 	//  CSI output enable
};

static struct regval_tab stream_soft_reset_regs[] = {
    // {0x0103, 0x01},
};

static struct regval_tab color_bar_regs[] = {
    // {0x0601, 0x02},
};
#define MAX96716_I2C_ADDR 0x28
#define MAX96717F_I2C_ADDR 0x40
#define IM556_I2C_ADDR 0x57

static struct regval_tab max96716_autolink_cfg_regs[] = {
    {0x0313, 0x00}, //  CSI output disable

    {0x0001, 0x01}, //  Link A receiver rate set to 3Gbps
    {0x0004, 0x01}, //  Link B receiver rate set to 3Gbps
    {0xFFFF, 0x0A}, //

    {0x0473, 0x10}, //
    {0x04b3, 0x10}, //
    {0x0112, 0x23}, //  Disable sequence miss and packet detect on pipe Y
    {0x0124, 0x23}, //  Disable sequence miss and packet detect on pipe Z

    {0x040A, 0x50}, //  PHY0 set to 2 lanes mode
    {0x044a, 0x50}, //  PHY1 set to 2 lanes mode
    {0x048a, 0x50}, //  PHY2 set to 2 lanes mode
    {0x04ca, 0x50}, //  PHY3 set to 2 lanes mode

    {0x1C00, 0xF4}, //
    {0x1D00, 0xF4}, //
    {0x1E00, 0xF4}, //
    {0x1F00, 0xF4}, //

    {0x031D, 0x2f}, //  CSI PHY0 rate set to 1000Mbps
    {0x0320, 0x2f}, //  CSI PHY1 rate set to 1000Mbps
    {0x0323, 0x2f}, //  CSI PHY2 rate set to 1000Mbps
    {0x0326, 0x2f}, //  CSI PHY3 rate set to 1000Mbps

    {0x1C00, 0xF5}, //
    {0x1D00, 0xF5}, //
    {0x1E00, 0xF5}, //
    {0x1F00, 0xF5}, //

    {0x0050, 0x00}, //  Controls which video packets sent from serializer enter the deserializer’s Pipe X
    {0x0051, 0x01}, //  Controls which video packets sent from serializer enter the deserializer’s Pipe Y
    {0x0052, 0x02}, //  Controls which video packets sent from serializer enter the deserializer’s Pipe Z
    {0x0053, 0x03}, //  Controls which video packets sent from serializer enter the deserializer’s Pipe U

    // {0x46d, 0xAA}, //  Map to DPHY1
    // {0x44B, 0x0F}, //  enable MAP_SRC_0~3
    // {0x44D, 0x2c}, //  mapping MAP_SRC_0
    // {0x44E, 0x2c}, //  mapping MAP_DST_0
    // {0x44F, 0x12}, //  mapping MAP_SRC_1
    // {0x450, 0x12}, //  mapping MAP_DST_1
    // {0x451, 0x01}, //  mapping MAP_SRC_2
    // {0x452, 0x01}, //  mapping MAP_DST_2
    // {0x453, 0x00}, //  mapping MAP_SRC_3
    // {0x454, 0x00}, //  mapping MAP_DST_3

    {0x4AD, 0xAA}, //  Map to DPHY2
    {0x48B, 0x0F}, //  enable MAP_SRC_0~3
    {0x48D, 0x2c}, //  mapping MAP_SRC_0
    {0x48E, 0x2c}, //  mapping MAP_DST_0
    {0x48F, 0x12}, //  mapping MAP_SRC_1
    {0x490, 0x12}, //  mapping MAP_DST_1
    {0x491, 0x01}, //  mapping MAP_SRC_2
    {0x492, 0x01}, //  mapping MAP_DST_2
    {0x493, 0x00}, //  mapping MAP_SRC_3
    {0x494, 0x00}, //  mapping MAP_DST_3

    // {0x0313, 0x02}, //  CSI output enable
};
static struct regval_tab max96717_autolink_cfg_regs[] = {
    {0x0383, 0x00}, // Disable tunneling mode
    {0x0331, 0x10}, // 2 lane mode
    {0x0312, 0x04}, // Double EMB8 on pipe Z
    {0x031E, 0x2C}, // Min BPP = 0x12 on pipe Z
    {0x0111, 0x50}, // Max BPP = 0x16
    {0x0110, 0x60}, // Disble auto BPP
    // {0x0112, 0x0C}, // Limit heartbeat

};
#define MAX96716_VTS_ADJUST     (0) /* vts - max_exposure*/
// #define MAX96716_VTS_LINES_MAX  (0x08CA)
#define MAX96716_VTS_LINES_MAX  (0xffff)
#define MAX96716_EXPO_LINES_MIN (0x0008)

#define MAX96716_VTS_ADDR_H16 (0x3026)
#define MAX96716_VTS_ADDR_H   (0x3025)
#define MAX96716_VTS_ADDR_L   (0x3024)
#define MAX96716_EXPO_H16     (0x3052)
#define MAX96716_EXPO_H       (0x3051)
#define MAX96716_EXPO_L       (0x3050)
#define MAX96716_AGAIN_L      (0x3090)
#define MAX96716_AGAIN_H      (0x3091)

#define MAX96716_GROUP_ACCESS (0x3001)

static int i2c_write_register(int devId, uint8_t i2c_addr, uint16_t regAddr, uint16_t value)
{
    struct cam_i2c_data reg_data;
    int ret = 0;

    reg_data.addr = i2c_addr;
    reg_data.reg_len = 16;
    reg_data.val_len = 8;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = value;
    ret = sensor_write_register(devId, &reg_data);

    return ret;
}

static int i2c_read_register(int devId, uint8_t i2c_addr, uint16_t regAddr, uint16_t* value)
{
    struct cam_i2c_data reg_data;
    int ret = 0;

    reg_data.addr = i2c_addr;
    reg_data.reg_len = 16;
    reg_data.val_len = 8;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = 0;
    ret = sensor_read_register(devId, &reg_data);
    if (!ret) {
        *value = reg_data.tab.val;
    }

    return ret;
}

static int i2c_write_burst_register(int devId, uint8_t i2c_addr, struct regval_tab* reg_table, int reg_table_num)
{
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(reg_table);

#if 1
    reg_table_data.addr = i2c_addr;
    reg_table_data.reg_len = 16;
    reg_table_data.val_len = 8;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_write_burst_register(devId, &reg_table_data);
#else
    {
        int i;
        for (i = 0; i < reg_table_num; i++) {
            max96716_write_register(handle, reg_table[i].reg, reg_table[i].val);
        }
    }
#endif
    return ret;
}

/*******************************************************************/
static int max96716_write_register(void* handle, uint16_t regAddr, uint16_t value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = max96716_reg_addr_byte;
    reg_data.val_len = max96716_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = value;
    ret = sensor_write_register(sensor_context->devId, &reg_data);

    return ret;
}

static int max96716_read_register(void* handle, uint16_t regAddr, uint16_t* value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_i2c_data reg_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_data.addr = sensor_context->i2c_addr;
    reg_data.reg_len = max96716_reg_addr_byte;
    reg_data.val_len = max96716_reg_data_byte;
    reg_data.tab.reg = regAddr;
    reg_data.tab.val = 0;
    ret = sensor_read_register(sensor_context->devId, &reg_data);
    if (!ret) {
        *value = reg_data.tab.val;
    }

    return ret;
}

static int max96716_write_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

#if 1
    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = max96716_reg_addr_byte;
    reg_table_data.val_len = max96716_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_write_burst_register(sensor_context->devId, &reg_table_data);
#else
    {
        int i;
        for (i = 0; i < reg_table_num; i++) {
            max96716_write_register(handle, reg_table[i].reg, reg_table[i].val);
        }
    }
#endif
    return ret;
}

#if 0
static int max96716_read_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(reg_table);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    reg_table_data.addr = sensor_context->work_info.i2c_addr;
    reg_table_data.reg_len = max96716_reg_addr_byte;
    reg_table_data.val_len = max96716_reg_data_byte;
    reg_table_data.tab = reg_table;
    reg_table_data.num = reg_table_num;
    ret = sensor_read_burst_register(sensor_context->devId, &reg_table_data);

    return ret;
}
#endif

/*******************************************************************/
/*isp sensor function*/
static int max96716_sensor_write_reg(void* snsHandle, uint32_t regAddr, uint32_t value)
{
    int ret = 0;
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = max96716_write_register(snsHandle, regAddr, value);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int max96716_sensor_group_reg_start(void* snsHandle)
{
    int ret = 0;
    // SENSOR_CONTEXT_S* sensor_context = NULL;

    // SENSORS_CHECK_PARA_POINTER(snsHandle);
    // sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    // SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    // pthread_mutex_lock(&sensor_context->apiLock);
    // max96716_write_register(snsHandle, MAX96716_GROUP_ACCESS, 1);
    // pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int max96716_sensor_group_reg_done(void* snsHandle)
{
    int ret = 0;
    // SENSOR_CONTEXT_S* sensor_context = NULL;

    // SENSORS_CHECK_PARA_POINTER(snsHandle);
    // sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    // SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    // pthread_mutex_lock(&sensor_context->apiLock);
    // max96716_write_register(snsHandle, MAX96716_GROUP_ACCESS, 0x00);
    // pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int max96716_sensor_get_isp_default(void* snsHandle, uint32_t u32ChanelId, uint32_t camScene,
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

    pthread_mutex_unlock(&sensor_context->apiLock);
    CLOG_DEBUG("%s: isp get sensor default, u32ChanelId %d", __FUNCTION__, u32ChanelId);

    return 0;
}

static int max96716_sensor_get_isp_black_level(void* snsHandle, uint32_t u32ChanelId,
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

static int max96716_sensor_get_reg_info(void* snsHandle, ISP_SENSOR_REGS_INFO_S* pstSensorRegsInfo)
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
            sensor_context->sensorRegs[0].astI2cData[i].u32AddrWidth = max96716_reg_addr_byte;
            sensor_context->sensorRegs[0].astI2cData[i].u32DataWidth = max96716_reg_data_byte;
        }
        sensor_context->sensorRegs[0].astI2cData[0].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[0].u32RegAddr = MAX96716_EXPO_L;  // exposure time
        sensor_context->sensorRegs[0].astI2cData[1].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[1].u32RegAddr = MAX96716_EXPO_H;  // exposure time
        sensor_context->sensorRegs[0].astI2cData[2].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[2].u32RegAddr = MAX96716_EXPO_H16;  // exposure time
        sensor_context->sensorRegs[0].astI2cData[3].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[3].u32RegAddr = MAX96716_AGAIN_L;  // analog gain
        sensor_context->sensorRegs[0].astI2cData[4].u8DelayFrmNum = 1;
        sensor_context->sensorRegs[0].astI2cData[4].u32RegAddr = MAX96716_AGAIN_H;  // analog gain
        sensor_context->sensorRegs[0].astI2cData[5].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[5].u32RegAddr = MAX96716_VTS_ADDR_L;  // VTS
        sensor_context->sensorRegs[0].astI2cData[6].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[6].u32RegAddr = MAX96716_VTS_ADDR_H;  // VTS
        sensor_context->sensorRegs[0].astI2cData[7].u8DelayFrmNum = 0;
        sensor_context->sensorRegs[0].astI2cData[7].u32RegAddr = MAX96716_VTS_ADDR_H16;  // VTS
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

static int max96716_sensor_dump_info(void* snsHandle)
{
    int ret = 0;
    // SENSOR_CONTEXT_S* sensor_context = NULL;
    // uint32_t vts = 0, exp_time = 0;
    // uint32_t again = 0, dgain = 0;
    // uint16_t reg_val_h, reg_val_l;

    // SENSORS_CHECK_PARA_POINTER(snsHandle);
    // sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    // SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    // max96716_read_register(snsHandle, MAX96716_VTS_ADDR_H, &reg_val_h);
    // max96716_read_register(snsHandle, MAX96716_VTS_ADDR_L, &reg_val_l);
    // vts = (reg_val_h << 8) | reg_val_l;
    // max96716_read_register(snsHandle, MAX96716_EXPO_H, &reg_val_h);
    // max96716_read_register(snsHandle, MAX96716_EXPO_L, &reg_val_l);
    // exp_time = (reg_val_h << 8) | reg_val_l;
    // max96716_read_register(snsHandle, MAX96716_AGAIN_L, &reg_val_h);
    // again = reg_val_h;
    // max96716_read_register(snsHandle, MAX96716_DGAIN_GR_H, &reg_val_h);
    // max96716_read_register(snsHandle, MAX96716_DGAIN_GR_L, &reg_val_l);
    // dgain = ((reg_val_h & 0x3) << 8) | reg_val_l;
    // pthread_mutex_lock(&sensor_context->apiLock);
    // CLOG_INFO("max96716 regs(vts=%d,exptime=%d,again=0x%x,dain =0x%x),struct(initVTS=%d,initFps=%f,vts=%d,expline=%d)",
    //     vts, exp_time, again, dgain, sensor_context->initVTS, sensor_context->initFps, sensor_context->vts[0],
    //     sensor_context->hdrIntTime[0] * 1000 / sensor_context->lineTime);
    // pthread_mutex_unlock(&sensor_context->apiLock);

    return ret;
}

/*ae function*/
static int max96716_sensor_get_ae_default(void* snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AE_DEFAULT_S* pstSensorAeDft)
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
    // pstSensorAeDft->maxExpTime = (pstSensorState->initVTS - MAX96716_VTS_ADJUST) * sensor_context->lineTime / 1000;
    // pstSensorAeDft->minExpTime = MAX96716_EXPO_LINES_MIN * sensor_context->lineTime / 1000;

    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int max96716_sensor_get_expotime_by_fps(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t max_expotime = 0;
//     uint32_t vts = 0;
//     float minFps;

//     SENSORS_CHECK_PARA_POINTER(snsHandle);
//     sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
//     SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

//     pthread_mutex_lock(&sensor_context->apiLock);
//     minFps = (sensor_context->minVTS * sensor_context->maxFps) / MAX96716_VTS_LINES_MAX;
//     if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps))
//         vts = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
//     else {
//         CLOG_ERROR("Not support Fps: %f, minFps, sensor_context->maxFps: (%f, %f), sensor_context->minVTS: %x", f32Fps, minFps, sensor_context->maxFps, sensor_context->minVTS);
//         max_expotime = -EINVAL;
//         goto out;
//     }

//     max_expotime = (vts - MAX96716_VTS_ADJUST) * sensor_context->lineTime / 1000;  // us
// out:
//     pthread_mutex_unlock(&sensor_context->apiLock);
    return max_expotime;
}

static int max96716_sensor_fps_set(void* snsHandle, float f32Fps)
{
    int ret = 0;
//     SENSOR_CONTEXT_S* sensor_context = NULL;
//     uint32_t lines;
//     float minFps;
//     uint32_t expLine = 0;

//     SENSORS_CHECK_PARA_POINTER(snsHandle);
//     sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
//     SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

//     pthread_mutex_lock(&sensor_context->apiLock);
//     minFps = (sensor_context->minVTS * sensor_context->maxFps) / MAX96716_VTS_LINES_MAX;
//     if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps))
//         lines = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
//     else {
//         CLOG_ERROR("Not support Fps: %f, minFps, sensor_context->maxFps: (%f, %f), sensor_context->minVTS: %x", f32Fps, minFps, sensor_context->maxFps, sensor_context->minVTS);
//         ret = -1;
//         goto out;
//     }
//     sensor_context->initVTS = lines;
//     sensor_context->initFps = f32Fps;
//     sensor_context->vts[0] = sensor_context->initVTS;
//     sensor_context->sensorRegs[0].astI2cData[5].u32Data = LOW_8BITS(sensor_context->vts[0]);
//     sensor_context->sensorRegs[0].astI2cData[6].u32Data = HIGH_8BITS(sensor_context->vts[0]);
//     sensor_context->sensorRegs[0].astI2cData[7].u32Data = HIGH_8BITS(HIGH_8BITS(sensor_context->vts[0]));
// out:
//     pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

#define VMAX 2250
static int max96716_sensor_expotime_update(void* snsHandle, uint32_t u32ChanelId, uint32_t u32ExpoTime,
                                          ISP_SENSOR_VTS_INFO_S* pstSensorVtsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
//     uint32_t shutter = 0;
//     uint32_t integration_time = 0;
//     uint32_t SHR0 = 0, SHR0_tmp;
//     float Toffset = 1.79;  // us

//     SENSORS_CHECK_PARA_POINTER(snsHandle);
//     sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
//     SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

//     pthread_mutex_lock(&sensor_context->apiLock);

//     shutter = u32ExpoTime * 1000 / sensor_context->lineTime;  // u32ExpoTime unit: us

//     // SHR0 = sensor_context->initVTS - (integration_time * 1000 - 2680) * sensor_context->lineTime; //bit 12: Toffset=2.68us

// // u32ExpoTime = sensor_context->vts[0]*sensor_context->lineTime / 1000  - SHR0 * sensor_context->lineTime / 1000 + Toffset;

//     SHR0 = (sensor_context->vts[0]*sensor_context->lineTime / 1000 + Toffset - u32ExpoTime ) * 1000 / sensor_context->lineTime;

//     SHR0_tmp = SHR0;

//     if (SHR0 < 8)
//         SHR0 = 8;
//     else if (SHR0 >= VMAX - 4)
//         SHR0 = VMAX - 4;

//     sensor_context->hdrIntTime[u32ChanelId] = integration_time * sensor_context->lineTime / 1000;

//     // if (SHR0_tmp < 8)
//     //     sensor_context->vts[0] = expLine + MAX96716_VTS_ADJUST;
//     // else
//     //     sensor_context->vts[0] = sensor_context->initVTS;

//     // sensor_context->sensorRegs[0].astI2cData[3].u32Data = LOW_8BITS(sensor_context->vts[0]);
//     // sensor_context->sensorRegs[0].astI2cData[4].u32Data = HIGH_8BITS(sensor_context->vts[0]);
// // SHR0=90;
//     sensor_context->sensorRegs[0].astI2cData[0].u32Data = LOW_8BITS(SHR0);
//     sensor_context->sensorRegs[0].astI2cData[1].u32Data = HIGH_8BITS(SHR0);
//     sensor_context->sensorRegs[0].astI2cData[2].u32Data = (SHR0>>16 & 0xf);

//     pstSensorVtsInfo->snsLineTime = sensor_context->lineTime;
//     pstSensorVtsInfo->snsVts = sensor_context->vts[0];
//     pstSensorVtsInfo->snsFps = sensor_context->initFps * sensor_context->initVTS / sensor_context->vts[0];
//     pthread_mutex_unlock(&sensor_context->apiLock);
//	printf("exp ttime: %d us, L:%d, SHR0_tmp:%d, SHR0:%d\n", u32ExpoTime, integration_time, SHR0_tmp, SHR0);

    return 0;
}

static int max96716_sensor_gain_update(void* snsHandle, uint32_t u32ChanelId, uint32_t* pAgainVal, uint32_t* pDgainVal)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int i, ret = 0;
    // uint32_t AGain_Reg, DGain_Reg = 0;

    // SENSORS_CHECK_PARA_POINTER(snsHandle);
    // SENSORS_CHECK_PARA_POINTER(pAgainVal);
    // SENSORS_CHECK_PARA_POINTER(pDgainVal);
    // sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    // SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	// uint32_t tval = *pAgainVal;
    // double gain_db;
    // uint32_t gain_val;
    // //从isp发下来的gain转成db = 20 log(*pAgainVal/256)

    // pthread_mutex_lock(&sensor_context->apiLock);

    // for (i = 0; i < ARRAY_SIZE(gain_table); i++) {
    //     if (tval < gain_table[i])
    //         break;
    // }

    // if (i >= ARRAY_SIZE(gain_table)) {
    //     gain_val = gain_table[ARRAY_SIZE(gain_table) - 1];
    // } else if (i == 0) {
    //     gain_val = gain_table[i];
    // } else {
    //     gain_val = gain_table[i - 1];
    // }

    // gain_db = 20 * log10(gain_val/256.0);
    // AGain_Reg = (uint32_t)(gain_db * 10 / 3);

    // sensor_context->sensorRegs[0].astI2cData[3].u32Data = LOW_8BITS(AGain_Reg);     // bit[7:0] = Again[7:0]
    // sensor_context->sensorRegs[0].astI2cData[4].u32Data = 0;             // bit[15:8] = Again[8]

    // *pAgainVal = gain_val;  // Q8
    // *pDgainVal = 4096;  // Q8 -> Q12
    // pthread_mutex_unlock(&sensor_context->apiLock);
//printf("again: %x (%x), AGain_Reg: %x, db:%f, i:%d\n", *pAgainVal, tval, AGain_Reg, gain_db, i);
    return ret;
}

static int max96716_get_aelib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                              AE_LIB_DEFAULT_SETTING_S** ppstAeLibDefault)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSORS_CHECK_PARA_POINTER(ppstAeLibDefault);
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    *ppstAeLibDefault = NULL;

// #if 1
//     // Transfer ae lib tunning file to struct like this if you need.
//     if (SENSOR_HDR_MODE == sensor_context->work_info.image_mode) {
//         if (0 == u32ChanelId)
//             *ppstAeLibDefault = NULL;

//         /* spm aelib needn't set setting for pipe1(short exposure),because setting of short exposure has already existed
//          * in pipe0, perhaps you can set the same value of pipe0 to pipe1.
//          */
//     } else {
//         if (0 == u32ChanelId)
//             *ppstAeLibDefault = NULL;
//         else if (1 == u32ChanelId)
//             *ppstAeLibDefault = NULL;
//     }
// #endif
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

/*awb function*/
static int max96716_sensor_get_awb_default(void* snsHandle, uint32_t u32ChanelId,
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

static int max96716_get_awblib_default_settings(void* snsHandle, uint32_t u32ChanelId,
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

static int max96716_power_on(SENSOR_CONTEXT_S* sensor_context)
{
    SENSORS_CHECK_PARA_POINTER(sensor_context);

    CLOG_INFO("finish power on");

    return 0;
}

/*******************************************************************/
static int max96716_init(void** pHandle, SENSOR_CUSTOM_S snr_custom)
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
    sensor_context->name = MAX96716_NAME;
    sensor_context->devId = sns_id;
    sensor_context->i2c_addr = sns_addr;
    sensor_context->magic = SENSOR_MAGIC;
    pthread_mutex_init(&sensor_context->apiLock, NULL);

    sensor_hw_init(sensor_context->devId);
    max96716_power_on(sensor_context);
    sensor_get_hw_info(sensor_context->devId, &sensor_hw_info);
    sensor_context->twsi_no = sensor_hw_info.twsi_no;
    i2c_fd = i2_ctrl_open(sensor_context->twsi_no);
    if (i2c_fd < 0) {
        CLOG_ERROR("open i2c%d failed %d, reason: %s", sensor_context->twsi_no, i2c_fd, strerror(errno));
        return i2c_fd;
    }

    *pHandle = sensor_context;
    return 0;
}

static int max96716_power_off(SENSOR_CONTEXT_S* sensor_context)
{
    SENSORS_CHECK_PARA_POINTER(sensor_context);

    CLOG_INFO("finish power off");

    return 0;
}
static int max96716_deinit(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int i, ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    sensor_context->magic = 0;
    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        for (i = 0; i < ARRAY_SIZE(stream_off_regs); i++) {
            ret = i2c_write_r16v8(i2c_fd, MAX96716_I2C_ADDR, stream_off_regs[i].reg, stream_off_regs[i].val);
            if (ret < 0)
                CLOG_ERROR("write register failed: %s\n", strerror(errno));
        }

        ret = i2c_write_r16v8(i2c_fd, IM556_I2C_ADDR, 0x1001, 0x00);
        if (ret)
            CLOG_INFO("write register failed: %s\n", strerror(errno));
        sensor_context->stream_on_flag = 0;
    }

    max96716_power_off(sensor_context);
    // sensor_hw_reset(sensor_context->devId);
    sensor_hw_exit(sensor_context->devId);
    pthread_mutex_unlock(&sensor_context->apiLock);
    i2_ctrl_close(i2c_fd);
    pthread_mutex_destroy(&sensor_context->apiLock);
    if (sensor_context) {
        memset(sensor_context, 0, sizeof(SENSOR_CONTEXT_S));
        free(sensor_context);
        sensor_context = NULL;
    }
    return ret;
}

static int max96716_global_config(void* handle, SENSOR_WORK_INFO_S* work_info)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0, i, j;
    uint16_t reg_val;
    uint8_t u8val;

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

    for (i = 0; i < sensor_context->work_info.setting_table_size; i++) {
        ret = i2c_write_r16v8(i2c_fd, IM556_I2C_ADDR, sensor_context->work_info.setting_table[i].reg, sensor_context->work_info.setting_table[i].val);
        if (ret < 0) {
            CLOG_ERROR("write register failed: %s", strerror(errno));
            goto out;
        }
    }

    for (i = 0; i < ARRAY_SIZE(max96716_autolink_cfg_regs); i++) {
        if (max96716_autolink_cfg_regs[i].reg == 0xFFFF) {
            usleep (1000 * max96716_autolink_cfg_regs[i].val);
            continue;
        }
        for (j = 0; j < 5; j++) {
            ret = i2c_write_r16v8(i2c_fd, MAX96716_I2C_ADDR, max96716_autolink_cfg_regs[i].reg, max96716_autolink_cfg_regs[i].val);
            if (ret < 0 && j > 4) {
                CLOG_ERROR("write  %dth %x,%x failed: %s", j, max96716_autolink_cfg_regs[i].reg, max96716_autolink_cfg_regs[i].val, strerror(errno));
                goto out;
            } else if (ret < 0 && j < 4) {
                j++;
                CLOG_ERROR("write %dth %x,%x failed: %s", j, max96716_autolink_cfg_regs[i].reg, max96716_autolink_cfg_regs[i].val, strerror(errno));
                usleep(100*1000);
            } else {
                break;
            }
        }
    }

#if 0
    CLOG_INFO("start MAX96716 read reg ");

    for (i = 0, j = 0; i < ARRAY_SIZE(max96716_autolink_cfg_regs); i++) {
        ret = i2c_read_r16v8(i2c_fd, MAX96716_I2C_ADDR, max96716_autolink_cfg_regs[i].reg, &u8val);
        if (ret < 0) {
            CLOG_ERROR("read register failed: %s\n", strerror(errno));
            goto out;
        } else {
            if (u8val != max96716_autolink_cfg_regs[i].val) {
                CLOG_ERROR("read %dth varify %x failed: %x != %x", i, max96716_autolink_cfg_regs[i].reg, max96716_autolink_cfg_regs[i].val, u8val);
            }
        }
    }

#endif

    ret = i2c_read_r16v8(i2c_fd, MAX96717F_I2C_ADDR, 0x000d, &u8val);
    if (ret < 0) {
        CLOG_ERROR("read MAX96717F ID register 0x000d failed: %s\n", strerror(errno));
        goto out;
    }
    CLOG_INFO("read max96717 reg 0x000d, reg_val = 0x%x, addr:%x, ret:%d", u8val, MAX96717F_I2C_ADDR, ret);

    ret = i2c_write_r16v8(i2c_fd, MAX96717F_I2C_ADDR, 0x0010, 0x21);
    if (ret < 0) {
        CLOG_ERROR("write 0x0010, reset max96717 failed: %s\n", strerror(errno));
        goto out;
    }
    usleep(200*1000);

    for (i = 0; i < ARRAY_SIZE(max96717_autolink_cfg_regs); i++) {
        for (j = 0; j < 5; j++) {
            ret = i2c_write_r16v8(i2c_fd, MAX96717F_I2C_ADDR, max96717_autolink_cfg_regs[i].reg, max96717_autolink_cfg_regs[i].val);
            if (ret < 0 && j > 4) {
                CLOG_ERROR("write  %dth %x,%x failed: %s", j, max96717_autolink_cfg_regs[i].reg, max96717_autolink_cfg_regs[i].val, strerror(errno));
                goto out;
            } else if (ret < 0 && j < 4) {
                j++;
                CLOG_ERROR("write %dth %x,%x failed: %s", j, max96717_autolink_cfg_regs[i].reg, max96717_autolink_cfg_regs[i].val, strerror(errno));
                usleep(100*1000);
            } else {
                break;
            }
        }
    }

    CLOG_INFO("init SerDes finish");

out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int max96716_set_param(void* handle, const SENSOR_INIT_ATTR_S* init_attr)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    memcpy(&sensor_context->init_3a_attr, init_attr, sizeof(SENSOR_INIT_ATTR_S));

    return 0;
}

static int max96716_stream_on(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret = 0;
    uint8_t u8val;
    uint32_t i = 0;
    CLOG_INFO("max96716_stream_on start");

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sensor_mipi_clock_set(sensor_context->devId, sensor_context->work_info.mipi_clock);
    if (ret)
        return ret;
    // for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
    //     max96716_write_register(handle, sensor_context->sensorRegs[0].astI2cData[i].u32RegAddr,
    //                            sensor_context->sensorRegs[0].astI2cData[i].u32Data);
    // }
    for (i = 0; i < ARRAY_SIZE(stream_on_regs); i++) {
        ret = i2c_write_r16v8(i2c_fd, MAX96716_I2C_ADDR, stream_on_regs[i].reg, stream_on_regs[i].val);
        if (ret < 0) {
            CLOG_ERROR("write register failed: %s\n", strerror(errno));
            goto out;
        }
    }

    ret = i2c_write_r16v8(i2c_fd, IM556_I2C_ADDR, 0x1001, 0x01);
    if (ret) {
        CLOG_INFO("write register failed: %s\n", strerror(errno));
        goto out;
    }
    usleep(210 * 1000);

    CLOG_INFO("max96716_stream_on finish, %d", sensor_context->work_info.mipi_clock);

    sensor_context->stream_on_flag = 1;
out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int max96716_stream_off(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int i, ret = 0;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    for (i = 0; i < ARRAY_SIZE(stream_off_regs); i++) {
        ret = i2c_write_r16v8(i2c_fd, MAX96716_I2C_ADDR, stream_off_regs[i].reg, stream_off_regs[i].val);
        if (ret < 0)
            CLOG_ERROR("write register failed: %s\n", strerror(errno));
    }

    ret = i2c_write_r16v8(i2c_fd, IM556_I2C_ADDR, 0x1001, 0x00);
    if (ret)
        CLOG_INFO("write register failed: %s\n", strerror(errno));

    sensor_context->stream_on_flag = 0;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int max96716_get_ops(void* handle, ISP_SENSOR_REGISTER_S* pSensorFuncOps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(pSensorFuncOps);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pSensorFuncOps->snsHandle = handle;
    pSensorFuncOps->stSensorFunc.pfn_sensor_write_reg = max96716_sensor_write_reg;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_default = max96716_sensor_get_isp_default;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_black_level = max96716_sensor_get_isp_black_level;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_reg_info = max96716_sensor_get_reg_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_dump_info = max96716_sensor_dump_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_start = max96716_sensor_group_reg_start;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_done = max96716_sensor_group_reg_done;

    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_ae_default = max96716_sensor_get_ae_default;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_fps_set = max96716_sensor_fps_set;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_expotime_by_fps = max96716_sensor_get_expotime_by_fps;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_expotime_update = max96716_sensor_expotime_update;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_gain_update = max96716_sensor_gain_update;
    pSensorFuncOps->stSensorAeFunc.pfn_get_aelib_default_settings = max96716_get_aelib_default_settings;

    pSensorFuncOps->stSensorAwbFunc.pfn_sensor_get_awb_default = max96716_sensor_get_awb_default;
    pSensorFuncOps->stSensorAwbFunc.pfn_get_awblib_default_settings = max96716_get_awblib_default_settings;
    pthread_mutex_unlock(&sensor_context->apiLock);

    return 0;
}

static int max96716_detect_sensor(void* handle, SENSOR_VENDOR_ID_S* vendor_id)
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
    reg_table_data.reg_len = max96716_reg_addr_byte;
    reg_table_data.val_len = max96716_reg_data_byte;
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

SENSOR_OBJ_S max96716Obj = {
    .name = MAX96716_NAME,
    .pfnInit = max96716_init,
    .pfnDeinit = max96716_deinit,
    .pfnGloablConfig = max96716_global_config,
    .pfnSetParam = max96716_set_param,
    .pfnStreamOn = max96716_stream_on,
    .pfnStreamOff = max96716_stream_off,
    .pfnGetSensorOps = max96716_get_ops,
    .pfnDetectSns = max96716_detect_sensor,
    .pfnWriteReg = max96716_write_register,
    .pfnReadReg = max96716_read_register,
};
