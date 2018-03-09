/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：hw.c
*文件功能描述:硬件相关到初始化
*创建标识   ：xuchangwei 20170819

*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include"uart.h"

/**************************************************
* name      :   peripherals_init
* funtion   :   初始化系统使用到的外设初始化
* parameter :   NULL
* return    :   NUL
***************************************************/
void peripherals_init(void)
{
   uart_init(); //串口初始化
}

/**************end file*********************************/
