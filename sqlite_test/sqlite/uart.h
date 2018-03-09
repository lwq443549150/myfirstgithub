/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ： uart.h
*文件功能描述: 串口相关配置头文件
*创建标识   ：xuchangwei 20170819
*修改标识   : 20170819 初次创建
*修改描述   : 20170819 初次创建
***************************************************/

#ifndef __UART_H_
#define __UART_H_

#include"sys_cfg.h"

/*串口设备使用到定义的结构体*/
typedef struct{
 s32 fd;            //设备文件名
 u32 baud;          //波特率
 s8  dev_str[15];   //串口设备字符串
}tsUART_DEV;

extern tsUART_DEV   lcd_uart_dev;                       //连接lcd串口设备参数结构体
extern void         uart_init(void);
//extern int          lcd_usart_send(u8* buf, u8 lenth);
#endif
