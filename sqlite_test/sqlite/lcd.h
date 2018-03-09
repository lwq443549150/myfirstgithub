/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ： lcd.h
*文件功能描述: lcd通讯头文件
*创建标识   ：xuchangwei 20170819
*修改标识   : 20170819 初次创建
*修改描述   : 20170819 初次创建
***************************************************/

#ifndef __LCD_H_
#define __LCD_H_
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>  //信号量
#include "sys_cfg.h"


typedef struct PAGE
{
	u8 (*poll_handler)(u32 poll_cnt); //页面轮询处理函数
	void (*vchange_handler)(u8 offset, u32 data);  //
	u8 (*button_handler)(u8 offset);
	void (*page_init)();  //页面初始化页面
	void* wo;              //只写变量
	void* ro;              //只读变量
	u16 poll_time;   //轮询时间间隔 单位ms
	u8  wo_len;		 //只写变量长度
	u8  ro_len;		 //只读变量长度
	u8 prev_id;   //前一级页面 用于从当前页面返回前一级
}page_t;


//DGUS 时间
typedef struct
{
	u8 YY;  //年 2000 + YY
	u8 MM;  //月
	u8 DD;  //日
	u8 WW;  //周
	u8 h;   //时
	u8 m;   //min
    u8 s;   //second
	}dgus_time_t;

//无效页面
#define PAGE_ID_INVALID 0xff

#define PAGE_PREV 0xfe


extern  void    lcd_uart_signal_handler_IO(s32 status,siginfo_t *info,void *myact);     //连接lcd串口接收中断函数
extern  void    lcd_pthread_task(void);
extern  int     dgus_show_text(u16 addr, char *str);
extern  int     dgus_write_var(u16 addr, u8 len);
extern  void    dgus_fill_rec(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
extern  void    dgus_copy_rec(u16 page_id,u16 addr, u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3);

#endif

/***************end file*******************************/
