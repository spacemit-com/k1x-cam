/*
 * Copyright (C) 2026 Spacemit Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "spm_comm_cam.h"
#include "cam_sensor.h"
#include "../module/hik_ir_spm/hik_ir_spm.h"

#define SENSOR_MAGIC 0x36400340
#define HIK_IR_NAME "hik_ir_uart"
#define HIK_IR_UART_DEVICE "/dev/ttyS8"
#define HIK_IR_DEVICE_ADDR 0x36

#ifndef I2C_SLAVE
#define I2C_SLAVE 0x0703
#endif

static const unsigned int hik_ir_reg_addr_byte = I2C_16BIT;
static const unsigned int hik_ir_reg_data_byte = I2C_8BIT;

#define HIK_IR_I2C_BUFFER_RW 0x1D00
#define HIK_IR_STREAM_ON_CMD_SIZE 20
#define HIK_IR_STREAM_OFF_CMD_SIZE 20
#define HIK_IR_DETECT_CMD_SIZE 20
#define HIK_IR_STATUS_REG_SIZE 2
#define HIK_IR_STATUS_SIZE 1
#define HIK_IR_VID_REG_SIZE 2
#define HIK_IR_VID_SIZE 2

static uint8_t hik_ir_stream_on_30hz_cmd[HIK_IR_STREAM_ON_CMD_SIZE] = {
    /* Matches: i2ctransfer -y 0 w20@0x3c ... */
    0x1d, 0x00,
    0x10, 0x10, 0x46, 0x00,
    0x01, 0x03, 0x1e, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0xb5, 0x9b,
};

static uint8_t hik_ir_stream_on_60hz_cmd[HIK_IR_STREAM_ON_CMD_SIZE] = {
    /* Matches: i2ctransfer -y 0 w20@0x3c ... */
    0x1d, 0x00,
    0x10, 0x10, 0x46, 0x00,
    0x01, 0x03, 0x3c, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x32, 0x7c,
};

static uint8_t hik_ir_stream_off_cmd[HIK_IR_STREAM_OFF_CMD_SIZE] = {
    /* Matches: i2ctransfer -y 0 w20@0x3c ... */
    0x1d, 0x00,
    0x10, 0x10, 0x46, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x58, 0x4d,
};

static uint8_t hik_ir_detect_vid_cmd[HIK_IR_DETECT_CMD_SIZE] = {
    /* Matches: i2ctransfer -y 0 w20@0x3c 0x1d 0x00 ... */
    0x1d, 0x00,
    0x01, 0x01, 0x81, 0x00,
    0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00,
    0x7b, 0xca,
};

static uint8_t hik_ir_status_reg[HIK_IR_STATUS_REG_SIZE] = {
    /* Matches: i2ctransfer -y 0 w2@0x3c 0x02 0x00 r1 */
    0x02, 0x00,
};

static uint8_t hik_ir_vid_reg[HIK_IR_VID_REG_SIZE] = {
    /* Matches: i2ctransfer -y 0 w2@0x3c 0x1d 0x12 r2 */
    0x1d, 0x12,
};

/* The module UART packet is F0, SIZE, address, class, sub-class, rw, DATA..., checksum, FF. */
static int hik_ir_uart_read_version(uint8_t version[3])
{
    const uint8_t request[] = {0xf0, 0x04, 0x36, 0x74, 0x03, 0x01, 0xae, 0xff};
    uint8_t response[32] = {0};
    struct termios tio;
    struct pollfd pfd;
    const char *dev_name = getenv("HIK_IR_UART_DEVICE");
    int fd, len, expected = 0, pos = 0;
    uint8_t checksum = 0;

    if (!dev_name || !dev_name[0])
        dev_name = HIK_IR_UART_DEVICE;
    fd = open(dev_name, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
        return -errno;
    if (tcgetattr(fd, &tio) || cfsetispeed(&tio, B115200) || cfsetospeed(&tio, B115200)) {
        close(fd);
        return -errno;
    }
    tio.c_cflag = (tio.c_cflag & ~CSIZE) | CS8 | CLOCAL | CREAD;
    tio.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 10;
    if (tcsetattr(fd, TCSANOW, &tio) || tcflush(fd, TCIOFLUSH) ||
        write(fd, request, sizeof(request)) != (ssize_t)sizeof(request)) {
        close(fd);
        return -errno;
    }
    pfd.fd = fd;
    pfd.events = POLLIN;
    while (pos < 2 || (expected && pos < expected)) {
        if (poll(&pfd, 1, 1000) <= 0) {
            close(fd);
            return -ETIMEDOUT;
        }
        len = read(fd, response + pos, sizeof(response) - pos);
        if (len <= 0) {
            close(fd);
            return -EIO;
        }
        pos += len;
        if (pos >= 2) {
            expected = response[1] + 4;
            if (response[0] != 0xf0 || expected > (int)sizeof(response)) {
                close(fd);
                return -EPROTO;
            }
        }
    }
    close(fd);
    /* The module echoes the query as a read response (0x03), not request flag 0x01. */
    if (response[pos - 1] != 0xff || response[2] != HIK_IR_DEVICE_ADDR ||
        response[3] != 0x74 || response[4] != 0x03 || response[5] != 0x03 || pos < 11)
        return -EPROTO;
    for (len = 2; len < pos - 2; len++)
        checksum += response[len];
    if (checksum != response[pos - 2])
        return -EBADMSG;
    version[0] = response[6];
    version[1] = response[7];
    version[2] = response[8];
    return 0;
}

static int hik_ir_i2c_write_raw(void* handle, uint8_t* data, uint32_t size)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    char dev_name[32];
    int fd;
    int ret;
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msg;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(data);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    snprintf(dev_name, sizeof(dev_name), "/dev/i2c-%u", sensor_context->twsi_no);
    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        CLOG_ERROR("open %s failed: %s", dev_name, strerror(errno));
        return -errno;
    }

    msg.addr = sensor_context->i2c_addr;
    msg.flags = 0;
    msg.len = size;
    msg.buf = data;
    ioctl_data.nmsgs = 1;
    ioctl_data.msgs = &msg;

    ret = ioctl(fd, I2C_RDWR, &ioctl_data);
    if (ret < 0) {
        CLOG_ERROR("I2C_RDWR write %s addr 0x%x failed: %s", dev_name, sensor_context->i2c_addr, strerror(errno));
        ret = -errno;
        close(fd);
        return ret;
    }

    close(fd);
    return 0;
}

static int hik_ir_i2c_write_read_raw(void* handle, uint8_t* wdata, uint32_t wsize, uint8_t* rdata, uint32_t rsize)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    char dev_name[32];
    int fd;
    int ret;
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msg[2];

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(wdata);
    SENSORS_CHECK_PARA_POINTER(rdata);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    snprintf(dev_name, sizeof(dev_name), "/dev/i2c-%u", sensor_context->twsi_no);
    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        CLOG_ERROR("open %s failed: %s", dev_name, strerror(errno));
        return -errno;
    }

    msg[0].addr = sensor_context->i2c_addr;
    msg[0].flags = 0;
    msg[0].len = wsize;
    msg[0].buf = wdata;

    msg[1].addr = sensor_context->i2c_addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = rsize;
    msg[1].buf = rdata;

    ioctl_data.nmsgs = 2;
    ioctl_data.msgs = msg;

    ret = ioctl(fd, I2C_RDWR, &ioctl_data);
    if (ret < 0) {
        CLOG_ERROR("I2C_RDWR read %s addr 0x%x failed: %s", dev_name, sensor_context->i2c_addr, strerror(errno));
        ret = -errno;
        close(fd);
        return ret;
    }

    close(fd);
    return 0;
}

static int hik_ir_write_register(void* handle, uint16_t regAddr, uint16_t value)
{
    (void)handle;
    (void)regAddr;
    (void)value;
    return -EOPNOTSUPP;
}

static int hik_ir_read_register(void* handle, uint16_t regAddr, uint16_t* value)
{
    (void)handle;
    (void)regAddr;
    (void)value;
    return -EOPNOTSUPP;
}

static int hik_ir_write_packet(void* handle, uint16_t regAddr, uint8_t* packet, uint32_t size)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct regval_tab* table = NULL;
    struct cam_burst_i2c_data reg_table_data;
    int ret = 0;
    uint32_t i;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(packet);
    sensor_context = (SENSOR_CONTEXT_S*)handle;

    table = (struct regval_tab*)calloc(size + 1, sizeof(struct regval_tab));
    if (!table) {
        return -ENOMEM;
    }

    for (i = 0; i < size; i++) {
        table[i].reg = regAddr + i;
        table[i].val = packet[i];
    }

    reg_table_data.addr = sensor_context->i2c_addr;
    reg_table_data.reg_len = hik_ir_reg_addr_byte;
    reg_table_data.val_len = hik_ir_reg_data_byte;
    reg_table_data.tab = table;
    reg_table_data.num = size;
    ret = sensor_write_burst_register(sensor_context->devId, &reg_table_data);

    free(table);
    return ret;
}

static int hik_ir_sensor_write_reg(void* snsHandle, uint32_t regAddr, uint32_t value)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = hik_ir_write_register(snsHandle, regAddr, value);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int hik_ir_sensor_group_reg_start(void* snsHandle)
{
    (void)snsHandle;
    return 0;
}

static int hik_ir_sensor_group_reg_done(void* snsHandle)
{
    (void)snsHandle;
    return 0;
}

static int hik_ir_sensor_get_isp_default(void* snsHandle, uint32_t u32ChanelId, uint32_t camScene,
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
        pstDef->pstIspDefaultSetting = NULL;
    } else {
        pstDef->pstIspDefaultSetting = sensor_context->work_info.pstIspDefaultSettings[camScene];
    }
    pthread_mutex_unlock(&sensor_context->apiLock);
    CLOG_DEBUG("%s: isp get sensor default, u32ChanelId %d", __FUNCTION__, u32ChanelId);
    return 0;
}

static int hik_ir_sensor_get_isp_black_level(void* snsHandle, uint32_t u32ChanelId,
                                            ISP_SENSOR_BLACK_LEVEL_S* pstBlackLevel)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int i;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstBlackLevel);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pstBlackLevel->bUpdate = false;
    if (SENSOR_LINEAR_MODE == sensor_context->work_info.image_mode) {
        for (i = 0; i < 4; i++) {
            pstBlackLevel->sensorBlackLevel[i] = 0;
            pstBlackLevel->bitDepth = 8;
        }
    }
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_sensor_get_reg_info(void* snsHandle, ISP_SENSOR_REGS_INFO_S* pstSensorRegsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorRegsInfo);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (false == sensor_context->syncInit) {
        sensor_context->sensorRegs[0].u8CfgDelayMax = 2;
        sensor_context->sensorRegs[0].u32RegNum = 0;
        sensor_context->sensorRegs[0].stSensorComBus.s8I2cDev = sensor_context->twsi_no;
        sensor_context->syncInit = true;
    }

    memcpy(pstSensorRegsInfo, &sensor_context->sensorRegs[0], sizeof(ISP_SENSOR_REGS_INFO_S));
    memcpy(&sensor_context->sensorRegs[1], &sensor_context->sensorRegs[0], sizeof(ISP_SENSOR_REGS_INFO_S));
    sensor_context->vts[1] = sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_sensor_dump_info(void* snsHandle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    CLOG_INFO("hik_ir struct(initVTS=%d,initFps=%f,vts=%d)", sensor_context->initVTS, sensor_context->initFps,
              sensor_context->vts[0]);
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_sensor_get_ae_default(void* snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AE_DEFAULT_S* pstSensorAeDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorAeDft);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pstSensorAeDft->initSceneLuma = sensor_context->init_3a_attr.initSceneLuma[u32ChanelId];
    pstSensorAeDft->initSceneLux = sensor_context->init_3a_attr.initSceneLux[u32ChanelId];
    pstSensorAeDft->initExpTime = sensor_context->work_info.exp_time[u32ChanelId];
    pstSensorAeDft->initAnaGain = sensor_context->work_info.again[u32ChanelId];
    pstSensorAeDft->initDGain = sensor_context->work_info.dgain[u32ChanelId];
    pstSensorAeDft->initTGain = pstSensorAeDft->initAnaGain * pstSensorAeDft->initDGain / 0x1000;
    pstSensorAeDft->maxDelayCfg = 2;
    pstSensorAeDft->minDelayCfg = 2;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_sensor_get_expotime_by_fps(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t vts = 0;
    float minFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / 0xffff;
    if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps)) {
        vts = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    } else {
        pthread_mutex_unlock(&sensor_context->apiLock);
        return -EINVAL;
    }
    pthread_mutex_unlock(&sensor_context->apiLock);
    return vts * sensor_context->lineTime / 1000;
}

static int hik_ir_sensor_fps_set(void* snsHandle, float f32Fps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t lines;
    float minFps;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    minFps = (sensor_context->minVTS * sensor_context->maxFps) / 0xffff;
    if ((f32Fps <= sensor_context->maxFps) && (f32Fps >= minFps)) {
        lines = sensor_context->minVTS * sensor_context->maxFps / f32Fps;
    } else {
        pthread_mutex_unlock(&sensor_context->apiLock);
        return -EINVAL;
    }
    sensor_context->initVTS = lines;
    sensor_context->initFps = f32Fps;
    sensor_context->vts[0] = sensor_context->initVTS;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_sensor_expotime_update(void* snsHandle, uint32_t u32ChanelId, uint32_t u32ExpoTime,
                                        ISP_SENSOR_VTS_INFO_S* pstSensorVtsInfo)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint32_t expLine = 0;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorVtsInfo);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    expLine = u32ExpoTime * 1000 / sensor_context->lineTime;
    sensor_context->hdrIntTime[u32ChanelId] = expLine * sensor_context->lineTime / 1000;
    sensor_context->vts[0] = sensor_context->initVTS;
    pstSensorVtsInfo->snsLineTime = sensor_context->lineTime;
    pstSensorVtsInfo->snsVts = sensor_context->vts[0];
    pstSensorVtsInfo->snsFps = sensor_context->initFps * sensor_context->initVTS / sensor_context->vts[0];
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_sensor_gain_update(void* snsHandle, uint32_t u32ChanelId, uint32_t* pAgainVal, uint32_t* pDgainVal)
{
    (void)snsHandle;
    (void)u32ChanelId;
    (void)pAgainVal;
    (void)pDgainVal;
    return 0;
}

static int hik_ir_get_aelib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                            AE_LIB_DEFAULT_SETTING_S** ppstAeLibDefault)
{
    (void)snsHandle;
    (void)u32ChanelId;
    SENSORS_CHECK_PARA_POINTER(ppstAeLibDefault);
    *ppstAeLibDefault = NULL;
    return 0;
}

static int hik_ir_sensor_get_awb_default(void* snsHandle, uint32_t u32ChanelId, ISP_SENSOR_AWB_DEFAULT_S* pstSensorAwbDft)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(snsHandle);
    SENSORS_CHECK_PARA_POINTER(pstSensorAwbDft);
    sensor_context = (SENSOR_CONTEXT_S*)snsHandle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId] != 0 &&
        sensor_context->init_3a_attr.initTint[u32ChanelId] != 0) {
        pstSensorAwbDft->initCorrelationCT = sensor_context->init_3a_attr.initCorrelationCT[u32ChanelId];
        pstSensorAwbDft->initTint = sensor_context->init_3a_attr.initTint[u32ChanelId];
    }
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_get_awblib_default_settings(void* snsHandle, uint32_t u32ChanelId,
                                             AWB_LIB_DEFAULT_SETTING_S** ppstAwbLibDefault)
{
    (void)snsHandle;
    (void)u32ChanelId;
    SENSORS_CHECK_PARA_POINTER(ppstAwbLibDefault);
    *ppstAwbLibDefault = NULL;
    return 0;
}

static int hik_ir_power_on(SENSOR_CONTEXT_S* sensor_context)
{
    SENSORS_CHECK_PARA_POINTER(sensor_context);

    sensor_set_power_voltage(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1800000);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 1);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 1);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DVDD, 1);
    sensor_set_mclk_rate(sensor_context->devId, 24000000);
    sensor_set_mclk_enable(sensor_context->devId, 1);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 0);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 0);
    usleep(10000);
    return 0;
}

static int hik_ir_init(void** pHandle, SENSOR_CUSTOM_S snr_custom)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    struct cam_sensor_info sensor_hw_info;
    int sns_id = snr_custom.dev_id;
    uint8_t sns_addr = snr_custom.i2c_addr;

    SENSORS_CHECK_PARA_POINTER(pHandle);

    sensor_context = (SENSOR_CONTEXT_S*)calloc(1, sizeof(SENSOR_CONTEXT_S));
    if (NULL == sensor_context) {
        return -ENOMEM;
    }
    sensor_context->name = HIK_IR_NAME;
    sensor_context->devId = sns_id;
    sensor_context->i2c_addr = sns_addr;
    sensor_context->magic = SENSOR_MAGIC;
    pthread_mutex_init(&sensor_context->apiLock, NULL);

    sensor_hw_init(sensor_context->devId);
    sensor_get_hw_info(sensor_context->devId, &sensor_hw_info);
    sensor_context->twsi_no = sensor_hw_info.twsi_no;

    *pHandle = sensor_context;
    return 0;
}

static int hik_ir_power_off(SENSOR_CONTEXT_S* sensor_context)
{
    SENSORS_CHECK_PARA_POINTER(sensor_context);

    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_RST, 1);
    sensor_set_gpio_enable(sensor_context->devId, SENSOR_GPIO_PWDN, 1);
    sensor_set_mclk_enable(sensor_context->devId, 0);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DVDD, 0);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_AVDD, 0);
    sensor_set_power_on(sensor_context->devId, SENSOR_REGULATOR_DOVDD, 0);
    return 0;
}

static int hik_ir_deinit(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    sensor_context->magic = 0;
    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        sensor_context->stream_on_flag = 0;
    }
    sensor_hw_exit(sensor_context->devId);
    pthread_mutex_unlock(&sensor_context->apiLock);

    pthread_mutex_destroy(&sensor_context->apiLock);
    memset(sensor_context, 0, sizeof(SENSOR_CONTEXT_S));
    free(sensor_context);
    return 0;
}

static int hik_ir_global_config(void* handle, SENSOR_WORK_INFO_S* work_info)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(work_info);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    if (sensor_context->stream_on_flag == 1) {
        pthread_mutex_unlock(&sensor_context->apiLock);
        return -EPERM;
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
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_set_param(void* handle, const SENSOR_INIT_ATTR_S* init_attr)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(init_attr);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    memcpy(&sensor_context->init_3a_attr, init_attr, sizeof(SENSOR_INIT_ATTR_S));
    return 0;
}

static int hik_ir_stream_on(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = sensor_mipi_clock_set(sensor_context->devId, sensor_context->work_info.mipi_clock);
    CLOG_INFO("hik_ir_uart stream on: CSI-2 output is autonomous; no I2C stream command is issued");
    if (!ret) {
        sensor_context->stream_on_flag = 1;
    }
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int hik_ir_stream_off(void* handle)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    int ret;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    CLOG_INFO("hik_ir_uart stream off: no UART stream command is defined by the supplied protocol");
    ret = 0;
    sensor_context->stream_on_flag = 0;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

static int hik_ir_get_ops(void* handle, ISP_SENSOR_REGISTER_S* pSensorFuncOps)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    SENSORS_CHECK_PARA_POINTER(pSensorFuncOps);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    pSensorFuncOps->snsHandle = handle;
    pSensorFuncOps->stSensorFunc.pfn_sensor_write_reg = hik_ir_sensor_write_reg;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_default = hik_ir_sensor_get_isp_default;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_isp_black_level = hik_ir_sensor_get_isp_black_level;
    pSensorFuncOps->stSensorFunc.pfn_sensor_get_reg_info = hik_ir_sensor_get_reg_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_dump_info = hik_ir_sensor_dump_info;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_start = hik_ir_sensor_group_reg_start;
    pSensorFuncOps->stSensorFunc.pfn_sensor_group_regs_done = hik_ir_sensor_group_reg_done;

    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_ae_default = hik_ir_sensor_get_ae_default;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_fps_set = hik_ir_sensor_fps_set;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_get_expotime_by_fps = hik_ir_sensor_get_expotime_by_fps;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_expotime_update = hik_ir_sensor_expotime_update;
    pSensorFuncOps->stSensorAeFunc.pfn_sensor_gain_update = hik_ir_sensor_gain_update;
    pSensorFuncOps->stSensorAeFunc.pfn_get_aelib_default_settings = hik_ir_get_aelib_default_settings;

    pSensorFuncOps->stSensorAwbFunc.pfn_sensor_get_awb_default = hik_ir_sensor_get_awb_default;
    pSensorFuncOps->stSensorAwbFunc.pfn_get_awblib_default_settings = hik_ir_get_awblib_default_settings;
    pthread_mutex_unlock(&sensor_context->apiLock);
    return 0;
}

static int hik_ir_detect_sensor(void* handle, SENSOR_VENDOR_ID_S* vendor_id)
{
    SENSOR_CONTEXT_S* sensor_context = NULL;
    uint8_t version[3] = {0};
    int ret;

    SENSORS_CHECK_PARA_POINTER(handle);
    sensor_context = (SENSOR_CONTEXT_S*)handle;
    SENSOR_CHECK_HANDLE_IS_ERR(sensor_context);

    pthread_mutex_lock(&sensor_context->apiLock);
    ret = hik_ir_uart_read_version(version);
    if (!ret) {
        CLOG_INFO("hik_ir_uart detected FPGA version %u.%u.%u", version[0], version[1], version[2]);
    } else {
        CLOG_ERROR("hik_ir_uart FPGA version query failed: %d", ret);
    }
    pthread_mutex_unlock(&sensor_context->apiLock);
    return ret;
}

SENSOR_OBJ_S hik_irObj = {
    .name = HIK_IR_NAME,
    .pfnInit = hik_ir_init,
    .pfnDeinit = hik_ir_deinit,
    .pfnGloablConfig = hik_ir_global_config,
    .pfnSetParam = hik_ir_set_param,
    .pfnStreamOn = hik_ir_stream_on,
    .pfnStreamOff = hik_ir_stream_off,
    .pfnGetSensorOps = hik_ir_get_ops,
    .pfnDetectSns = hik_ir_detect_sensor,
    .pfnWriteReg = hik_ir_write_register,
    .pfnReadReg = hik_ir_read_register,
};
