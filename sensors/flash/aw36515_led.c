/*
 * Copyright (C) 2022 ASR Micro Limited
 * All Rights Reserved.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "cam_led.h"

#define LED_NAME "aw36515"

static int aw36515_flash_init(void** pHandle)
{
    FLASH_CONTEXT_S* flash_context = NULL;

    SENSORS_CHECK_PARA_POINTER(pHandle);

    flash_context = (FLASH_CONTEXT_S*)calloc(1, sizeof(FLASH_CONTEXT_S));
    if (NULL == flash_context) {
        CLOG_ERROR("%s: led_context malloc memory failed!", __FUNCTION__);
        return -ENOMEM;
    }
    flash_context->name = LED_NAME;
    flash_context->twsi_no = 2;
    *pHandle = flash_context;
    return 0;
}

static int aw36515_flash_deinit(void* handle)
{
    FLASH_CONTEXT_S* flash_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    flash_context = (FLASH_CONTEXT_S*)handle;
    free(flash_context);
    flash_context = NULL;
    return 0;
}

static int aw36515_flash_set_mode(void* handle, int mode)
{
    FLASH_CONTEXT_S* flash_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    flash_context = (FLASH_CONTEXT_S*)handle;
    int dev_fd;
    int ret = 0;
    switch (mode) {
        case 0: {
            char devName[128] = "/sys/class/leds/torch/brightness";
            dev_fd = open(devName, O_RDWR | O_NONBLOCK);
            if (dev_fd < 0) {
                CLOG_ERROR("open as36515 torch fail \n");
                return -1;
            }
            char* ptrBrightness = "0";
            ret = write(dev_fd, ptrBrightness, 2);
            if (ret == -1)
                CLOG_ERROR("aw36515_flash_set_mode %d write fail!", mode);
            close(dev_fd);
        } break;
        case 1: {
            char devName[128] = "/sys/class/leds/torch/brightness";
            dev_fd = open(devName, O_RDWR | O_NONBLOCK);
            if (dev_fd < 0) {
                CLOG_ERROR("open as36515 torch fail \n");
                return -1;
            }
            char* ptrBrightness = "20";
            ret = write(dev_fd, ptrBrightness, 3);
            if (ret == -1)
                CLOG_ERROR("aw36515_flash_set_mode %d write fail!", mode);

            close(dev_fd);
        } break;
        case 2: {
            char devName[128] = "/sys/class/leds/flash/brightness";
            dev_fd = open(devName, O_RDWR | O_NONBLOCK);
            if (dev_fd < 0) {
                CLOG_ERROR("open as36515 flash fail \n");
                return -1;
            }
            char* ptrBrightness = "80";
            ret = write(dev_fd, ptrBrightness, 3);
            if (ret == -1)
                CLOG_ERROR("aw36515_flash_set_mode %d write fail!", mode);
            close(dev_fd);
        } break;
        default:
            break;
    }
    return ret;
}

FLASH_OBJ_S aw36515FlashObj = {
    .name = LED_NAME,
    .pfnInit = aw36515_flash_init,
    .pfnDeinit = aw36515_flash_deinit,
    .pfnSetMode = aw36515_flash_set_mode,
};
