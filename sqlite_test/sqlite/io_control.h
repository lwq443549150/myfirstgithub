/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ： io.h
*文件功能描述: IO口控制头文件
*创建标识   ：xuchangwei 20170819
*修改标识   : 20170819 初次创建
*修改描述   : 20170819 初次创建
***************************************************/

#ifndef _IO_CONTROL_H
#define _IO_CONTROL_H

#define DEBUG 1

#ifdef DEBUG
	#define DBG_MSG(fmt, arg...)	fprintf(stdout, fmt, ##arg)
#else
	#define DBG_MSG(fmt, arg...)
#endif

#define ERR_MSG(fmt, arg...)    fprintf(stderr, fmt, ##arg)
#define INF_MSG(fmt, arg...)    fprintf(stdout, fmt, ##arg)
#define ARRAY_SIZE(a)   (sizeof(a)/sizeof(a[0]))


extern void io_air_pump1_on();
extern void io_air_pump1_off();
extern void io_air_pump2_on();
extern void io_air_pump2_off();
extern void io_leds_run_on();
extern void io_leds_run_off();
#endif
