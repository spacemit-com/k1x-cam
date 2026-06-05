/*
 * Copyright (C) 2026 Spacemit Micro Limited
 * SC285SL sensor driver (based on sc533hai driver)
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "spm_comm_cam.h"
#include "cam_sensor.h"

#define SENSOR_MAGIC 0x285 /* unique magic */
#define SC285SL_NAME "sc285sl"

static const unsigned int sc285sl_reg_addr_byte = I2C_16BIT; /* sensor register address byte width */
static const unsigned int sc285sl_reg_data_byte = I2C_8BIT;  /* sensor register data byte width */

static struct regval_tab stream_on_regs[] = {
	{0x0100, 0x01},
};

static struct regval_tab stream_off_regs[] = {
	{0x0100, 0x00},
};

static struct regval_tab stream_soft_reset_regs[] = {
	{0x0103, 0x01},
};

#define SC285SL_VTS_ADDR_H   (0x320e)
#define SC285SL_VTS_ADDR_L   (0x320f)
#define SC285SL_EXPO_H       (0x3e00)
#define SC285SL_EXPO_M       (0x3e01)
#define SC285SL_EXPO_L       (0x3e02)

#define SC285SL_GROUP_ACCESS (0x3812)
#define SC285SL_GROUP_DELAY  (0x3802)

#define SC285SL_VTS_ADJUST     (8) /* vts - max_exposure*/
#define SC285SL_VTS_LINES_MAX (0x3ffff0)
#define SC285SL_EXPO_LINES_MIN (2)

static int sc285sl_write_register(void* handle, uint16_t regAddr, uint16_t value)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	struct cam_i2c_data reg_data;
	int ret = 0;

	SENSORS_CHECK_PARA_POINTER(handle);
	sensor_context = (SENSOR_CONTEXT_S*)handle;

	reg_data.addr = sensor_context->i2c_addr;
	reg_data.reg_len = sc285sl_reg_addr_byte;
	reg_data.val_len = sc285sl_reg_data_byte;
	reg_data.tab.reg = regAddr;
	reg_data.tab.val = value;
	ret = sensor_write_register(sensor_context->devId, &reg_data);

	return ret;
}

static int sc285sl_read_register(void* handle, uint16_t regAddr, uint16_t* value)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	struct cam_i2c_data reg_data;
	int ret = 0;

	SENSORS_CHECK_PARA_POINTER(handle);
	sensor_context = (SENSOR_CONTEXT_S*)handle;

	reg_data.addr = sensor_context->i2c_addr;
	reg_data.reg_len = sc285sl_reg_addr_byte;
	reg_data.val_len = sc285sl_reg_data_byte;
	reg_data.tab.reg = regAddr;
	reg_data.tab.val = 0;
	ret = sensor_read_register(sensor_context->devId, &reg_data);
	if (!ret) {
		*value = reg_data.tab.val;
	}

	return ret;
}

static int sc285sl_write_burst_register(void* handle, struct regval_tab* reg_table, int reg_table_num)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	struct cam_burst_i2c_data reg_table_data;
	int ret = 0;

	SENSORS_CHECK_PARA_POINTER(handle);
	SENSORS_CHECK_PARA_POINTER(reg_table);
	sensor_context = (SENSOR_CONTEXT_S*)handle;

	reg_table_data.addr = sensor_context->i2c_addr;
	reg_table_data.reg_len = sc285sl_reg_addr_byte;
	reg_table_data.val_len = sc285sl_reg_data_byte;
	reg_table_data.tab = reg_table;
	reg_table_data.num = reg_table_num;
	ret = sensor_write_burst_register(sensor_context->devId, &reg_table_data);

	return ret;
}

/* sensor-level write used by ISP */
static int sc285sl_sensor_write_reg(void* snsHandle, uint32_t regAddr, uint32_t value)
{
	int ret = 0;
	SENSOR_CONTEXT_S* sensor_context = NULL;
	SENSORS_CHECK_PARA_POINTER(snsHandle);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	ret = sc285sl_write_register(snsHandle, regAddr, value);
	pthread_mutex_unlock(&sensor_context->apiLock);
	return ret;
}

static int sc285sl_sensor_group_reg_start(void* snsHandle)
{
	int ret = 0;
	SENSOR_CONTEXT_S* sensor_context = NULL;
	SENSORS_CHECK_PARA_POINTER(snsHandle);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	sc285sl_write_register(snsHandle, SC285SL_GROUP_ACCESS, 0);
	pthread_mutex_unlock(&sensor_context->apiLock);
	return ret;
}

static int sc285sl_sensor_group_reg_done(void* snsHandle)
{
	int ret = 0;
	SENSOR_CONTEXT_S* sensor_context = NULL;
	SENSORS_CHECK_PARA_POINTER(snsHandle);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	sc285sl_write_register(snsHandle, SC285SL_GROUP_ACCESS, 0x30);
	pthread_mutex_unlock(&sensor_context->apiLock);
	return ret;
}

static int sc285sl_sensor_get_isp_default(void* snsHandle, uint32_t u32ChanelId, uint32_t camScene,
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

static int sc285sl_sensor_get_isp_black_level(void* snsHandle, uint32_t u32ChanelId,
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

static int sc285sl_sensor_get_reg_info(void* snsHandle, ISP_SENSOR_REGS_INFO_S* pstSensorRegsInfo)
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
		sensor_context->sensorRegs[0].u32RegNum = 7;
		sensor_context->sensorRegs[0].stSensorComBus.s8I2cDev = sensor_context->twsi_no;

		for (i = 0; i < sensor_context->sensorRegs[0].u32RegNum; i++) {
			sensor_context->sensorRegs[0].astI2cData[i].bUpdate = true;
			sensor_context->sensorRegs[0].astI2cData[i].u8DevAddr = sensor_context->i2c_addr;
			sensor_context->sensorRegs[0].astI2cData[i].u32AddrWidth = sc285sl_reg_addr_byte;
			sensor_context->sensorRegs[0].astI2cData[i].u32DataWidth = sc285sl_reg_data_byte;
		}

		sensor_context->sensorRegs[0].astI2cData[0].u8DelayFrmNum = 2;
		sensor_context->sensorRegs[0].astI2cData[0].u32RegAddr = SC285SL_EXPO_L;  // exposure time low
		sensor_context->sensorRegs[0].astI2cData[1].u8DelayFrmNum = 2;
		sensor_context->sensorRegs[0].astI2cData[1].u32RegAddr = SC285SL_EXPO_M;  // exposure time mid
		sensor_context->sensorRegs[0].astI2cData[2].u8DelayFrmNum = 2;
		sensor_context->sensorRegs[0].astI2cData[2].u32RegAddr = SC285SL_EXPO_H;  // exposure time high
		sensor_context->sensorRegs[0].astI2cData[3].u8DelayFrmNum = 2;
		sensor_context->sensorRegs[0].astI2cData[3].u32RegAddr = 0x3e08;  // analog gain (example)
		sensor_context->sensorRegs[0].astI2cData[4].u8DelayFrmNum = 2;
		sensor_context->sensorRegs[0].astI2cData[4].u32RegAddr = 0x3e09;  // fine analog gain (example)
		sensor_context->sensorRegs[0].astI2cData[5].u8DelayFrmNum = 2;
		sensor_context->sensorRegs[0].astI2cData[5].u32RegAddr = SC285SL_VTS_ADDR_L;  // VTS low
		sensor_context->sensorRegs[0].astI2cData[6].u8DelayFrmNum = 2;
		sensor_context->sensorRegs[0].astI2cData[6].u32RegAddr = SC285SL_VTS_ADDR_H;  // VTS high

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

static int sc285sl_sensor_dump_info(void* snsHandle)
{
	int ret = 0;
	SENSOR_CONTEXT_S* sensor_context = NULL;
	uint32_t vts = 0, exp_time = 0;
	uint16_t reg_val_h, reg_val_l;

	SENSORS_CHECK_PARA_POINTER(snsHandle);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	sc285sl_read_register(snsHandle, SC285SL_VTS_ADDR_H, &reg_val_h);
	sc285sl_read_register(snsHandle, SC285SL_VTS_ADDR_L, &reg_val_l);
	vts = (reg_val_h << 8) | reg_val_l;
	sc285sl_read_register(snsHandle, SC285SL_EXPO_H, &reg_val_h);
	sc285sl_read_register(snsHandle, SC285SL_EXPO_L, &reg_val_l);
	exp_time = (reg_val_h << 8) | reg_val_l;

	pthread_mutex_lock(&sensor_context->apiLock);
	CLOG_INFO("sc285sl regs(vts=%d,exptime=%d), struct(initVTS=%d,initFps=%f,vts=%d)",
		vts, exp_time, sensor_context->initVTS, sensor_context->initFps, sensor_context->vts[0]);
	pthread_mutex_unlock(&sensor_context->apiLock);

	return ret;
}

/* AE placeholders: mirror sc533hai behavior where compatible */
static int sc285sl_sensor_get_ae_default(void* snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AE_DEFAULT_S* pstSensorAeDft)
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

	pstSensorAeDft->initSceneLuma = sensor_context->init_3a_attr.initSceneLuma[u32ChanelId];
	pstSensorAeDft->initSceneLux = sensor_context->init_3a_attr.initSceneLux[u32ChanelId];
	pstSensorAeDft->initExpTime = exp_time;
	pstSensorAeDft->initAnaGain = again;
	pstSensorAeDft->initDGain = dgain;
	pstSensorAeDft->initTGain = pstSensorAeDft->initAnaGain * pstSensorAeDft->initDGain / 0x1000;

	pstSensorAeDft->maxDelayCfg = 2;
	pstSensorAeDft->minDelayCfg = 2;
	pthread_mutex_unlock(&sensor_context->apiLock);
	return 0;
}

static int sc285sl_sensor_get_expotime_by_fps(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t max_expotime = 0;
    uint32_t vts = 0;
    float minFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / SC285SL_VTS_LINES_MAX;
    if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps))
        vts = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    else {
        CLOG_ERROR("Not support Fps: %f", f32Fps);
        max_expotime = -EINVAL;
        goto out;
    }

    max_expotime = (vts - SC285SL_VTS_ADJUST) * sensor_context->lineTime / 1000;  // us
out:
    pthread_mutex_unlock(&sensor_context->apiLock);
    return max_expotime;
}

static int sc285sl_sensor_fps_set(void* snsHandle, float f32Fps)
{
	int ret = 0;
	SENSOR_CONTEXT_S* sensor_context = NULL;
	uint32_t lines;
	float minFps;

	SENSORS_CHECK_PARA_POINTER(snsHandle);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	minFps = (sensor_context->minVTS * sensor_context->maxFps) / 0x1fff0;
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
	sensor_context->sensorRegs[0].astI2cData[5].u32Data = LOW_8BITS(sensor_context->vts[0]);
	sensor_context->sensorRegs[0].astI2cData[6].u32Data = HIGH_8BITS(sensor_context->vts[0]);
out:
	pthread_mutex_unlock(&sensor_context->apiLock);
	return ret;
}

static int sc285sl_sensor_expotime_update(void* snsHandle, uint32_t u32ChanelId, uint32_t u32ExpoTime,
		ISP_SENSOR_VTS_INFO_S* pstSensorVtsInfo)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	uint32_t expLine = 0;

	SENSORS_CHECK_PARA_POINTER(snsHandle);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	expLine = u32ExpoTime * 1000 / sensor_context->lineTime;  // us -> lines
	if (expLine < 2) expLine = 2;
	expLine = (expLine < SC285SL_EXPO_LINES_MIN) ? SC285SL_EXPO_LINES_MIN : expLine;
	expLine = (expLine > (SC285SL_VTS_LINES_MAX - SC285SL_VTS_ADJUST)) ? (SC285SL_VTS_LINES_MAX - SC285SL_VTS_ADJUST)
                                                                       : expLine;

	sensor_context->hdrIntTime[u32ChanelId] = expLine * sensor_context->lineTime / 1000;

	sensor_context->sensorRegs[0].astI2cData[5].u32Data = LOW_8BITS(sensor_context->vts[0]);
	sensor_context->sensorRegs[0].astI2cData[6].u32Data = HIGH_8BITS(sensor_context->vts[0]);
	sensor_context->sensorRegs[0].astI2cData[0].u32Data = (((expLine) << 4) & 0xF0);
	sensor_context->sensorRegs[0].astI2cData[1].u32Data = (((expLine) >> 4) & 0xFF);
	sensor_context->sensorRegs[0].astI2cData[2].u32Data = (((expLine) >> 12) & 0xFF);

	pstSensorVtsInfo->snsLineTime = sensor_context->lineTime;
	pstSensorVtsInfo->snsVts = sensor_context->vts[0];
	pstSensorVtsInfo->snsFps = sensor_context->initFps * sensor_context->initVTS / sensor_context->vts[0];
	pthread_mutex_unlock(&sensor_context->apiLock);
	//  printf("exp ttime: %d us, L:%d, vts:%d\n", u32ExpoTime, expLine, sensor_context->vts[0]);
	return 0;
}

// static int sc285sl_sensor_gain_update(void* snsHandle, uint32_t u32ChanelId, uint32_t* pAgainVal, uint32_t* pDgainVal)
// {
// 	SENSOR_CONTEXT_S* sensor_context = NULL;
// 	int ret = 0;
// 	uint32_t AGain_Reg = 0, AGain_Reg_Fine = 0;
// 	uint32_t AGain_Val, step;

// 	SENSORS_CHECK_PARA_POINTER(snsHandle);
// 	SENSORS_CHECK_PARA_POINTER(pAgainVal);
// 	SENSORS_CHECK_PARA_POINTER(pDgainVal);
// 	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
// 	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

// 	pthread_mutex_lock(&sensor_context->apiLock);
// 	/* pAgainVal from caller is Q8 (256 = 1x). Convert to Q10 for mapping (<<2). */
// 	AGain_Val = (*pAgainVal >> 2);  // Q8 -> Q10

// 	/* Mapping adapted from sc533hai: map Q10 AGain_Val to coarse/fine registers.
// 	 * Ranges and register encodings follow the same piecewise scheme.
// 	 */
// 	if (AGain_Val < 0x80) { /* 1.000~2.000x (ANA GAIN=0x00) */
// 		step = (AGain_Val - 0x40) * 32 / 0x40; /* 0x40(Q10)=1.0x,0x80(Q10)=2.0x -> 0x20~0x3F */
// 		AGain_Reg = 0x00;
// 		AGain_Reg_Fine = 0x20 + step;
// 	} else if (AGain_Val < 0x100) { /* 2.660~5.237x (ANA GAIN=0x80, DCG enabled) */
// 		step = (AGain_Val - 0x80) * 32 / 0x80; /* 0x80(Q10)=2.66x,0x100(Q10)=5.237x */
// 		AGain_Reg = 0x01;
// 		AGain_Reg_Fine = 0x20 + step;
// 	} else if (AGain_Val < 0x159) { /* 5.320~8.313x (ANA GAIN=0x81) */
// 		step = (AGain_Val - 0x100) * 12 / 0x100; /* 0x100(Q10)=5.32x,0x180(Q10)=8.313x */
// 		AGain_Reg = 0x03;
// 		AGain_Reg_Fine = 0x20 + step;
// 	} else if (AGain_Val < 0x2B3) { /* 8.567~18.567x (ANA GAIN=0x83) */
// 		step = (AGain_Val - 0x159) * 32 / 0x159; /* 0x180(Q10)=8.313x,0x280(Q10)=18.567x */
// 		AGain_Reg = 0x81;
// 		AGain_Reg_Fine = 0x20 + step;
// 	} else if (AGain_Val < 0x566) { /* 18.567~41.895x (ANA GAIN=0x87) */
// 		step = (AGain_Val - 0x2B3) * 32 / 0x2B3; /* 0x280(Q10)=18.567x,0x480(Q10)=41.895x */
// 		AGain_Reg = 0x87;
// 		AGain_Reg_Fine = 0x20 + step;
// 	} else { /* 42.560~max (ANA GAIN=0x8f, max analog gain) */
// 		step = (AGain_Val - 0x566) * 32 / 0x566; /* 0x480(Q10)=42.56x,0x880(Q10)=~83.79x */
// 		AGain_Reg = 0x8f;
// 		AGain_Reg_Fine = 0x20 + (step > 31 ? 31 : step);
// 	}

//         if (AGain_Reg_Fine > 0x3F) AGain_Reg_Fine = 0x3F; /* Cap fine gain to max */
// 	sensor_context->sensorRegs[0].astI2cData[3].u32Data = AGain_Reg;
// 	sensor_context->sensorRegs[0].astI2cData[4].u32Data = AGain_Reg_Fine;

// 	/* return AGain in Q12 (same convention as sc533hai) and set dgain to 1x (Q12) */
// 	*pAgainVal = AGain_Val << 2; /* Q10 -> Q12 */
// 	*pDgainVal = 4096;

//         printf("pAgainVal: %d  AGain_Reg: %x, AGain_Reg_Fine: %x, step: %d\n", *pAgainVal,
//                AGain_Reg, AGain_Reg_Fine, step);

//         pthread_mutex_unlock(&sensor_context->apiLock);
// 	return ret;
// }

static int sc285sl_sensor_gain_update(void *snsHandle, uint32_t u32ChanelId,
                                      uint32_t *pAgainVal,
                                      uint32_t *pDgainVal) {
	SENSOR_CONTEXT_S *sensor_context = NULL;
	int ret = 0;
	uint32_t AGain_Reg = 0, AGain_Reg_Fine = 0;
	uint32_t AGain_Val;
	float gain_multiplier;
	const uint8_t fine_gain_step = 32;

	SENSORS_CHECK_PARA_POINTER(snsHandle);
	SENSORS_CHECK_PARA_POINTER(pAgainVal);
	SENSORS_CHECK_PARA_POINTER(pDgainVal);
	sensor_context = (SENSOR_CONTEXT_S *)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);

	AGain_Val = *pAgainVal;
	gain_multiplier = (float)AGain_Val / 256.0f;


	if (gain_multiplier < 2.000f) {
		/* 1.000x ~ 1.969x → AGain_Reg=0x00，Fine=0x20~0x3F */
		AGain_Reg = 0x00;
		float range = 1.969f - 1.000f;
		float offset = gain_multiplier - 1.000f;
		AGain_Reg_Fine = 0x20 + (uint32_t)((offset / range) * fine_gain_step);
	} else if (gain_multiplier < 4.000f) {
		/* 2.000x ~ 3.938x → AGain_Reg=0x01，Fine=0x20~0x3F */
		AGain_Reg = 0x01;
		float range = 3.938f - 2.000f;
		float offset = gain_multiplier - 2.000f;
		AGain_Reg_Fine = 0x20 + (uint32_t)((offset / range) * fine_gain_step);
	} else if (gain_multiplier < 8.000f) {
		/* 4.000x ~ 7.931x → AGain_Reg=0x03，Fine=0x20~0x3F */
		AGain_Reg = 0x03;
		float range = 7.931f - 4.000f;
		float offset = gain_multiplier - 4.000f;
		AGain_Reg_Fine = 0x20 + (uint32_t)((offset / range) * fine_gain_step);
	} else if (gain_multiplier < 16.000f) {
		/* 档位 4：8.100x ~ 15.938x → AGain_Reg=0x80，Fine=0x20~0x3F */
		AGain_Reg = 0x80;
		float range = 15.938f - 8.100f;
		float offset = gain_multiplier - 8.100f;
		AGain_Reg_Fine = 0x20 + (uint32_t)((offset / range) * fine_gain_step);
	} else if (gain_multiplier < 32.000f) {
		/* 16.200x ~ 31.938x → AGain_Reg=0x81，Fine=0x20~0x3F */
		AGain_Reg = 0x81;
		float range = 31.938f - 16.200f;
		float offset = gain_multiplier - 16.200f;
		AGain_Reg_Fine = 0x20 + (uint32_t)((offset / range) * fine_gain_step);
	} else if (gain_multiplier < 64.000f) {
		/* 32.400x ~ 63.450x → AGain_Reg=0x83，Fine=0x20~0x3F */
		AGain_Reg = 0x83;
		float range = 63.450f - 32.400f;
		float offset = gain_multiplier - 32.400f;
		AGain_Reg_Fine = 0x20 + (uint32_t)((offset / range) * fine_gain_step);
	} else if (gain_multiplier < 128.000f) {
		/* 64.800x ~ 126.900x → AGain_Reg=0x87，Fine=0x20~0x3F */
		AGain_Reg = 0x87;
		float range = 126.900f - 64.800f;
		float offset = gain_multiplier - 64.800f;
		AGain_Reg_Fine = 0x20 + (uint32_t)((offset / range) * fine_gain_step);
	} else {
		/* 129.600x ~ 170.100x → AGain_Reg=0x8F，Fine=0x20~0x3F
		*/
		AGain_Reg = 0x8F;
		float range = 170.100f - 129.600f;
		float offset = gain_multiplier - 129.600f;
		AGain_Reg_Fine = 0x20 + (uint32_t)((offset / range) * fine_gain_step);
	}

	if (AGain_Reg_Fine > 0x3F) {
		AGain_Reg_Fine = 0x3F;
	}


	sensor_context->sensorRegs[0].astI2cData[3].u32Data =
	AGain_Reg;
	sensor_context->sensorRegs[0].astI2cData[4].u32Data =
	AGain_Reg_Fine;

	*pAgainVal = (uint32_t)(gain_multiplier * 4096.0f);
	*pDgainVal = 4096;

	CLOG_DEBUG("sc285sl gain update: pAgainVal(Q8)=%d, gain=%.3fx, "
		"AGain_Reg=0x%02x, AGain_Fine=0x%02x",
		AGain_Val, gain_multiplier, AGain_Reg, AGain_Reg_Fine);

	pthread_mutex_unlock(&sensor_context->apiLock);
	return ret;

}
static int sc285sl_get_aelib_default_settings(void* snsHandle, uint32_t u32ChanelId,
		AE_LIB_DEFAULT_SETTING_S** ppstAeLibDefault)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	SENSORS_CHECK_PARA_POINTER(snsHandle);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSORS_CHECK_PARA_POINTER(ppstAeLibDefault);
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	*ppstAeLibDefault = NULL;
	pthread_mutex_unlock(&sensor_context->apiLock);
	return 0;
}

static int sc285sl_sensor_get_awb_default(void* snsHandle, uint32_t u32ChanelId,
		ISP_SENSOR_AWB_DEFAULT_S* pstSensorAwbDft)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	SENSORS_CHECK_PARA_POINTER(snsHandle);
	SENSORS_CHECK_PARA_POINTER(pstSensorAwbDft);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	if (sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId] != 0 && sensor_context->init_3a_attr.initTint[u32ChanelId] != 0) {
		pstSensorAwbDft->initCorrelationCT = sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId];
		pstSensorAwbDft->initTint = sensor_context->init_3a_attr.initTint[u32ChanelId];
	}
	pthread_mutex_unlock(&sensor_context->apiLock);
	return 0;
}

static int sc285sl_get_awblib_default_settings(void* snsHandle, uint32_t u32ChanelId,
		AWB_LIB_DEFAULT_SETTING_S** ppstAwbLibDefault)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	SENSORS_CHECK_PARA_POINTER(snsHandle);
	SENSORS_CHECK_PARA_POINTER(ppstAwbLibDefault);
	sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	*ppstAwbLibDefault = NULL;
	pthread_mutex_unlock(&sensor_context->apiLock);
	return 0;
}

static int sc285sl_power_on(SENSOR_CONTEXT_S* sensor_context)
{
	SENSORS_CHECK_PARA_POINTER(sensor_context);

	sensor_set_mclk_enable(sensor_context->devId, 1);
	sensor_set_mclk_rate(sensor_context->devId, 27000000);

	sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);
	sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);

	sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1800000);
	sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1);
	sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DVDD, 1200000);
	sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DVDD, 1);
	sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_AVDD, 2800000);
	sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 1);

	sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 1);
	sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 1);
	usleep(10000);
	CLOG_INFO("sc285sl: finish power on");
	return 0;
}

static int sc285sl_init(void** pHandle, SENSOR_CUSTOM_S snr_custom)
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
	sensor_context->name = SC285SL_NAME;
	sensor_context->devId = sns_id;
	sensor_context->i2c_addr = sns_addr;
	sensor_context->magic = SENSOR_MAGIC;
	pthread_mutex_init(&sensor_context->apiLock, NULL);
	sensor_hw_init(sensor_context->devId);
	sc285sl_power_on(sensor_context);
	sensor_get_hw_info(sensor_context->devId, &sensor_hw_info);
	sensor_context->twsi_no = sensor_hw_info.twsi_no;

	*pHandle = sensor_context;
	return 0;
}

static int sc285sl_power_off(SENSOR_CONTEXT_S* sensor_context)
{
	SENSORS_CHECK_PARA_POINTER(sensor_context);

	sensor_set_mclk_enable(sensor_context->devId, 0);
	sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);
	sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);
	sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1800000);
	sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 0);
	sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DVDD, 1200000);
	sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DVDD, 0);
	sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_AVDD, 2800000);
	sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 0);
	usleep(2100);
	CLOG_INFO("sc285sl: finish power off");
	return 0;
}

static int sc285sl_deinit(void* handle)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;

	SENSORS_CHECK_PARA_POINTER(handle);
	sensor_context = (SENSOR_CONTEXT_S*)handle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	sensor_context->magic = 0;
	pthread_mutex_lock(&sensor_context->apiLock);
	if (sensor_context->stream_on_flag == 1) {
		sc285sl_write_burst_register(handle, stream_off_regs, ARRAY_SIZE(stream_off_regs));
		sensor_context->stream_on_flag = 0;
	}

	sc285sl_power_off(sensor_context);
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

static int sc285sl_global_config(void* handle, SENSOR_WORK_INFO_S* work_info)
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

	ret = sc285sl_write_burst_register(handle, stream_soft_reset_regs, ARRAY_SIZE(stream_soft_reset_regs));
	if (ret) {
		CLOG_ERROR("sensor soft reset fail");
		goto out;
	}
	usleep(1000);

        ret = sc285sl_write_burst_register(handle, sensor_context->work_info.setting_table,
					sensor_context->work_info.setting_table_size);
        if (ret) {
		CLOG_ERROR("sensor write reg fail");
		goto out;
	}

out:
	pthread_mutex_unlock(&sensor_context->apiLock);
	return ret;
}

static int sc285sl_set_param(void* handle, const SENSOR_INIT_ATTR_S* init_attr)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;

	SENSORS_CHECK_PARA_POINTER(handle);
	sensor_context = (SENSOR_CONTEXT_S*)handle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	memcpy(&sensor_context->init_3a_attr, init_attr, sizeof(SENSOR_INIT_ATTR_S));
	return 0;
}

static int sc285sl_stream_on(void* handle)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	int ret = 0;

	SENSORS_CHECK_PARA_POINTER(handle);
	sensor_context = (SENSOR_CONTEXT_S*)handle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	ret = sensor_mipi_clock_set(sensor_context->devId, sensor_context->work_info.mipi_clock);
	if (ret)
		return ret;

	ret = sc285sl_write_burst_register(handle, stream_on_regs, ARRAY_SIZE(stream_on_regs));
	usleep(1000);
	sensor_context->stream_on_flag = 1;
	pthread_mutex_unlock(&sensor_context->apiLock);
	return ret;
}

static int sc285sl_stream_off(void* handle)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;
	int ret = 0;

	SENSORS_CHECK_PARA_POINTER(handle);
	sensor_context = (SENSOR_CONTEXT_S*)handle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	ret = sc285sl_write_burst_register(handle, stream_off_regs, ARRAY_SIZE(stream_off_regs));
	sensor_context->stream_on_flag = 0;
	pthread_mutex_unlock(&sensor_context->apiLock);
	return ret;
}

static int sc285sl_get_ops(void* handle, ISP_SENSOR_REGISTER_S* pSensorFuncOps)
{
	SENSOR_CONTEXT_S* sensor_context = NULL;

	SENSORS_CHECK_PARA_POINTER(handle);
	SENSORS_CHECK_PARA_POINTER(pSensorFuncOps);
	sensor_context = (SENSOR_CONTEXT_S*)handle;
	SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

	pthread_mutex_lock(&sensor_context->apiLock);
	pSensorFuncOps->snsHandle = handle;
	pSensorFuncOps->stSensorFunc.pfn_sensor_write_reg = sc285sl_sensor_write_reg;
	pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_default = sc285sl_sensor_get_isp_default;
        pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_black_level = sc285sl_sensor_get_isp_black_level;
        pSensorFuncOps->stSensorFunc.pfn_sensor_get_reg_info = sc285sl_sensor_get_reg_info;
	pSensorFuncOps->stSensorFunc.pfn_sensor_dump_info = sc285sl_sensor_dump_info;
	pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_start = sc285sl_sensor_group_reg_start;
	pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_done = sc285sl_sensor_group_reg_done;

	pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_ae_default = sc285sl_sensor_get_ae_default;
	pSensorFuncOps->stSensorAeFunc.pfn_sensor_fps_set = sc285sl_sensor_fps_set;
	pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_expotime_by_fps = sc285sl_sensor_get_expotime_by_fps;
	pSensorFuncOps->stSensorAeFunc.pfn_sensor_expotime_update = sc285sl_sensor_expotime_update;
	pSensorFuncOps->stSensorAeFunc.pfn_sensor_gain_update = sc285sl_sensor_gain_update;
	pSensorFuncOps->stSensorAeFunc.pfn_get_aelib_default_settings = sc285sl_get_aelib_default_settings;

	pSensorFuncOps->stSensorAwbFunc.pfn_sensor_get_awb_default = sc285sl_sensor_get_awb_default;
	pSensorFuncOps->stSensorAwbFunc.pfn_get_awblib_default_settings = sc285sl_get_awblib_default_settings;
	pthread_mutex_unlock(&sensor_context->apiLock);

	return 0;
}

static int sc285sl_detect_sensor(void* handle, SENSOR_VENDOR_ID_S* vendor_id)
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
	reg_table_data.reg_len = sc285sl_reg_addr_byte;
	reg_table_data.val_len = sc285sl_reg_data_byte;
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

SENSOR_OBJ_S sc285slObj = {
	.name = SC285SL_NAME,
	.pfnInit = sc285sl_init,
	.pfnDeinit = sc285sl_deinit,
	.pfnGloablConfig = sc285sl_global_config,
	.pfnSetParam = sc285sl_set_param,
	.pfnStreamOn = sc285sl_stream_on,
	.pfnStreamOff = sc285sl_stream_off,
	.pfnGetSensorOps = sc285sl_get_ops,
	.pfnDetectSns = sc285sl_detect_sensor,
	.pfnWriteReg = sc285sl_write_register,
	.pfnReadReg = sc285sl_read_register,
};


