/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：io_control.c
*文件功能描述:IO口相关操作
*创建标识   ：xuchangwei 20170819
//
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

#include "io_control.h"
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define PUMP1                "/sys/class/gpio/ry1/value"
#define PUMP2                "/sys/class/gpio/ry2/value"
#define LED_RUN              "/sys/class/leds/run/brightness"

/******************************************************************
* 函数名: io_control
* 功能:   控制IO口的输入输出
* 参数：
* 返回值:
******************************************************************/
void io_control(char *path, int value)
{
    int fd = -1;
	char strval[32] = {0};

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		ERR_MSG("open %s failed: %s\n", path, strerror(errno));
		goto err_exit;
	}

	sprintf(strval, "%d", value);

	if (write(fd, strval, strlen(strval)) <= 0) {
		ERR_MSG("Write to %s failed!\n", path);
		goto err_exit;
	}

	close(fd);
	return ;

err_exit:
	if (fd > 0) {
		close(fd);
	}
	return ;
}


/******************************************************************
* 函数名: io_air_pump1_on
* 功能:   气泵1打开
* 参数：
* 返回值:
******************************************************************/
void io_air_pump1_on()
{
    io_control(PUMP1, 1);
}

/******************************************************************
* 函数名: io_air_pump1_off
* 功能:   气泵1关闭
* 参数：
* 返回值:
******************************************************************/
void io_air_pump1_off()
{
    io_control(PUMP1, 0);
}

/******************************************************************
* 函数名: io_air_pump2_on
* 功能:   气泵1打开
* 参数：
* 返回值:
******************************************************************/
void io_air_pump2_on()
{
    io_control(PUMP2, 1);
}

/******************************************************************
* 函数名: io_air_pump2_off
* 功能:   气泵2关闭
* 参数：
* 返回值:
******************************************************************/
void io_air_pump2_off()
{
    io_control(PUMP2, 0);
}

/******************************************************************
* 函数名: io_leds_run_on
* 功能:   运行指示灯打开
* 参数：
* 返回值:
******************************************************************/
void io_leds_run_on()
{
    io_control(LED_RUN, 1);
}

/******************************************************************
* 函数名: io_leds_run_off
* 功能:   运行指示灯关闭
* 参数：
* 返回值:
******************************************************************/
void io_leds_run_off()
{
    io_control(LED_RUN, 0);
}

/********************end file*********************/
