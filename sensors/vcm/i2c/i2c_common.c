/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */

#include "i2c_common.h"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include "cam_log.h"
/*********************************************************************/
int i2_ctrl_open(int twsi_no)
{
    int i2c_ctrl_fp = 0;
    char file_name[100] = {0};

    sprintf(file_name, "/dev/i2c-%d", twsi_no);

    i2c_ctrl_fp = open(file_name, O_RDWR);
    if (i2c_ctrl_fp < 0)
        CLOG_ERROR("open i2c device %s fail", file_name);
    else
        CLOG_DEBUG("open i2c device %s success", file_name);

    return i2c_ctrl_fp;
}

int i2_ctrl_close(int i2c_ctrl_fp)
{
    if (i2c_ctrl_fp < 0) {
        CLOG_ERROR("invalid i2c ctrl file, i2c_ctrl_fp %d", i2c_ctrl_fp);
        return -1;
    }

    close(i2c_ctrl_fp);
    return 0;
}

int i2_ctrl_write(int i2c_ctrl_fp, struct cmd_i2c_data* i2c_data)
{
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msg;
    uint8_t val[8];
    int i, j = 0, ret = 0;

    if (i2c_ctrl_fp < 0) {
        CLOG_ERROR("invalid i2c ctrl file, i2c_ctrl_fp %d", i2c_ctrl_fp);
        return -1;
    }

    msg.addr = i2c_data->addr;
    msg.flags = 0;
    msg.buf = val;
    msg.len = i2c_data->reg_len + i2c_data->val_len;

    for (i = 0; i < i2c_data->reg_len; i++) val[j++] = ((uint8_t*)(&i2c_data->reg))[i];
    for (i = 0; i < i2c_data->val_len; i++) val[j++] = ((uint8_t*)(&i2c_data->val))[i];

    ioctl_data.nmsgs = 1;
    ioctl_data.msgs = &msg;
    ret = ioctl(i2c_ctrl_fp, I2C_RDWR, &ioctl_data);
    if (ret < 0)
        CLOG_ERROR("i2_ctrl_write failed %d(%s)", ret, strerror(errno));

    return ret;
}

int i2_ctrl_read(int i2c_ctrl_fp, struct cmd_i2c_data* i2c_data)
{
    struct i2c_rdwr_ioctl_data ioctl_data;
    struct i2c_msg msg[2];
    uint8_t reg[4], val[4];
    int ret = 0;

    if (i2c_ctrl_fp < 0) {
        CLOG_ERROR("invalid i2c ctrl file, i2c_ctrl_fp %d", i2c_ctrl_fp);
        return -1;
    }

    msg[0].addr = i2c_data->addr;
    msg[0].flags = 0;
    msg[0].buf = reg;
    msg[0].len = i2c_data->reg_len;
    if (i2c_data->reg_len == I2C_8BIT) {
        reg[0] = i2c_data->reg & 0xff;
    } else if (i2c_data->reg_len == I2C_16BIT) {
        reg[0] = (i2c_data->reg >> 8) & 0xff;
        reg[1] = i2c_data->reg & 0xff;
    }

    msg[1].addr = i2c_data->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = i2c_data->val_len;

    ioctl_data.nmsgs = 2;
    ioctl_data.msgs = msg;
    ret = ioctl(i2c_ctrl_fp, I2C_RDWR, &ioctl_data);
    if (ret < 0) {
        CLOG_ERROR("i2_ctrl_read: read i2c_addr 0x%x reg 0x%x 0x%x failed %d(%s)", i2c_data->addr, reg[0], reg[1], ret, strerror(errno));
        return ret;
    }

    if (i2c_data->val_len == I2C_8BIT)
        i2c_data->val = val[0];
    else if (i2c_data->val_len == I2C_16BIT)
        i2c_data->val = (val[0] << 8) + val[1];
    else if (i2c_data->val_len == I2C_32BIT)
        i2c_data->val = (val[3] << 24) + (val[2] << 16) + (val[1] << 8) + val[0];

    return ret;
}
