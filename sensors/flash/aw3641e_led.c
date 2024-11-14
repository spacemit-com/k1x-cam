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
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/time.h>
#include "cam_led.h"
//#include "cam_sensor.h"

#define LED_NAME "aw3641e"
static int gpio_flag = 0;

void sigalrm_handler(int sig)
{
    if (sig == SIGALRM) {
        system ("echo 0 > /sys/class/gpio/gpio10/value ");
        system ("echo 0 > /sys/class/gpio/gpio9/value ");
    }
}

void set_timer()
{
    struct itimerval itv;

    memset(&itv,0,sizeof(itv));
    itv.it_interval.tv_sec = 0;//不定时
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 0;//第一次250微秒
    itv.it_value.tv_usec = 250;
 
    int ret = setitimer(ITIMER_REAL, &itv, NULL);
    if (ret) {
        CLOG_ERROR("setitimer failed!/n");
    }
}

static int aw3641e_flash_init(void** pHandle)
{
    FLASH_CONTEXT_S* flash_context = NULL;
    struct stat s;
    FILE *fp = NULL;
    char buffer[30];

    SENSORS_CHECK_PARA_POINTER(pHandle);

    flash_context = (FLASH_CONTEXT_S*)calloc(1, sizeof(FLASH_CONTEXT_S));
    if (NULL == flash_context) {
        CLOG_ERROR("%s: led_context malloc memory failed!", __FUNCTION__);
        return -ENOMEM;
    }
    flash_context->name = LED_NAME;

    if (stat("/sys/class/gpio", &s) == 0 && stat("/sys/class/gpio/export", &s) == 0) {
        //config gpio
        system ("echo 10 > /sys/class/gpio/export ");
        system ("echo out > /sys/class/gpio/gpio10/direction ");
        system ("echo 0 > /sys/class/gpio/gpio10/value ");
        system ("echo 9 > /sys/class/gpio/export ");
        system ("echo out > /sys/class/gpio/gpio9/direction ");
        system ("echo 0 > /sys/class/gpio/gpio9/value ");

        //verify gpio
        fp = popen ("cat /sys/class/gpio/gpio10/direction ", "r");
        if (fp == NULL) {
            CLOG_ERROR("popen fail! Invaild gpio cmd!");
            return -1;
        }
        while (fgets(buffer, 30, fp) != NULL) {
            CLOG_INFO("gpio10_direction: %s", buffer);
        }
        pclose (fp);
        fp = popen ("cat /sys/class/gpio/gpio10/value", "r");
        if (fp == NULL) {
            CLOG_ERROR("popen fail! Invaild gpio cmd!");
            return -1;
        }
        while (fgets(buffer, 30, fp) != NULL) {
            CLOG_INFO("gpio10_value: %s", buffer);
        }
		pclose (fp);

        fp = popen ("cat /sys/class/gpio/gpio9/direction ", "r");
        if (fp == NULL) {
            CLOG_ERROR("popen fail! Invaild gpio cmd!");
            return -1;
        }
        while (fgets(buffer, 30, fp) != NULL) {
            CLOG_INFO("gpio9_direction: %s", buffer);
        }
        pclose (fp);
        fp = popen ("cat /sys/class/gpio/gpio9/value", "r");
        if (fp == NULL) {
            CLOG_ERROR("popen fail! Invaild gpio cmd!");
            return -1;
        }
        while (fgets(buffer, 30, fp) != NULL) {
            CLOG_INFO("gpio9_value: %s", buffer);
        }
		pclose (fp);

        gpio_flag = 1;

    } else {
        CLOG_ERROR("stat fail! Invaild gpio node!");
        return -1;
	}

    *pHandle = flash_context;
    return 0;
}

static int aw3641e_flash_deinit(void* handle)
{
    FLASH_CONTEXT_S* flash_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    flash_context = (FLASH_CONTEXT_S*)handle;
    free(flash_context);
    flash_context = NULL;

    if (gpio_flag == 1) {
        system ("echo 10 > /sys/class/gpio/unexport ");
        system ("echo 9 > /sys/class/gpio/unexport ");
    }

    return 0;
}
static int aw3641e_flash_set_mode(void* handle, int mode)
{
    FLASH_CONTEXT_S* flash_context = NULL;

    SENSORS_CHECK_PARA_POINTER(handle);
    flash_context = (FLASH_CONTEXT_S*)handle;
    int ret = 0;

    if (gpio_flag != 1) {
        CLOG_ERROR("Invaild gpio node!");
        return -1;
    }

    switch (mode) {
        case 0: {   //flash mode
            system ("echo 1 > /sys/class/gpio/gpio9/value ");
            system ("echo 1 > /sys/class/gpio/gpio10/value ");
            // usleep(250 * 1000);
            signal(SIGALRM, sigalrm_handler);
            set_timer();
            // system ("echo 0 > /sys/class/gpio/gpio10/value ");
            // system ("echo 0 > /sys/class/gpio/gpio9/value ");
        } break;
        case 1: {   // open torch mode
            system ("echo 1 > /sys/class/gpio/gpio10/value ");
        } break;
        case 2: {   //close torch mode
            system ("echo 0 > /sys/class/gpio/gpio10/value ");
        } break;
        default:
            break;
    }
    return ret;
}

FLASH_OBJ_S aw3641eFlashObj = {
    .name = LED_NAME,
    .pfnInit = aw3641e_flash_init,
    .pfnDeinit = aw3641e_flash_deinit,
    .pfnSetMode = aw3641e_flash_set_mode,
};
