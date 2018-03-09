/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：uart.c
*文件功能描述:串口初始化
*创建标识   ：xuchangwei 20170819
//
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
#include <signal.h>
#include <features.h>

#include "sys_cfg.h"
#include "main.h"
#include "uart.h"


/*********************参数定义****************************/
tsUART_DEV  lcd_uart_dev;                       //连接lcd串口设备参数结构体


/**************************************************
* name     :uart_configution
* funtion  : 串口初始化, 8位数据位,1位停止位,无校验
* parameter：
            fd:需要配置串口的文件描述符
            dev:串口设备文件名指针,
            baud: 波特率,设置为:2400,4800,9600,38400,57600,115200其中的一种
            read_signal_handler:接收软中断函数
* return   :NULL
***************************************************/
static void uart_configution(s32 *fd,s8 *dev,u32 baud,void(*read_signal_handler)(s32 status,siginfo_t *info,void *myact))
{

    struct termios newtio;
    struct sigaction saio; /*definition of signal axtion */


   // printf("uart_init\r\n");
    *fd = open(dev, O_RDWR | O_NOCTTY);//串口

       /* install the signal handle before making the device asynchronous*/
     // saio.sa_handler = read_signal_handler;
      //sigemptyset(&saio.sa_mask);
      //saio.sa_mask = 0; 必须用sigemptyset函数初始话act结构的sa_mask成员

     // saio.sa_flags = SA_SIGINFO;
     // saio.sa_restorer = NULL;
    //  sigaction(SIGIO,&saio,NULL);
   //   sigaction(SIGPOLL,&saio,NULL);
      /* allow the process to recevie SIGIO*/
    //  fcntl(*fd,F_SETOWN,getpid());
      /* Make the file descriptor asynchronous*/
    //  fcntl(*fd,F_SETFL,FASYNC);

     // fcntl(*fd,F_SETSIG,SIGPOLL);

    /* set serial port */
	memset(&newtio, 0, sizeof(newtio));

	/* ignore modem control lines and enable receiver */
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	/* set character size */
    newtio.c_cflag |= CS8;
	/* set the parity */
    newtio.c_cflag &= ~PARENB;
	/* set the stop bits */
    newtio.c_cflag &= ~CSTOPB;
    /* set output and input baud rate */

    switch(baud)
    {

    case 2400:
        cfsetospeed(&newtio, B2400);
        cfsetispeed(&newtio, B2400);
     break;

    case 4800:
        cfsetospeed(&newtio, B4800);
        cfsetispeed(&newtio, B4800);
     break;

     case 9600:
        cfsetospeed(&newtio, B9600);
        cfsetispeed(&newtio, B9600);
     break;

     case 38400:
        cfsetospeed(&newtio, B38400);
        cfsetispeed(&newtio, B38400);
     break;

     case 57600:
        cfsetospeed(&newtio, B57600);
        cfsetispeed(&newtio, B57600);
     break;

     case 115200:
        cfsetospeed(&newtio, B115200);
        cfsetispeed(&newtio, B115200);
     break;

     default:
        cfsetospeed(&newtio, B9600);
        cfsetispeed(&newtio, B9600);
     break;
    }
   newtio.c_cc[VTIME] =0;
   newtio.c_cc[VMIN] =7;

	/* flushes data received but not read */
	tcflush(*fd, TCIFLUSH);
	/* set the parameters associated with the terminal from
		the termios structure and the change occurs immediately */
	if((tcsetattr(*fd, TCSANOW, &newtio))!=0)
	{
		perror("set_port/tcsetattr");
	}
}



/**************************************************
* name     : uart_init
* funtion  : 串口初始化, 8位数据位,1位停止位,无校验
* parameter：NULL
* return   :NULL
***************************************************/
void uart_init(void)
{
    lcd_uart_dev.baud = 9600;
    //memcpy(lcd_uart_dev.dev_str,"/dev/ttyO1",sizeof("/dev/ttyO1"));
    memcpy(lcd_uart_dev.dev_str,"/dev/rs485_1", sizeof("/dev/rs485_1"));
    uart_configution(&lcd_uart_dev.fd,lcd_uart_dev.dev_str,lcd_uart_dev.baud,lcd_uart_signal_handler_IO);
}

/**************************************************
* name     : lcd_uart_send
* funtion  : 连接lcd的串口发送函数
* parameter：
            buff：需要发送数据的数据指针,
            len: 需要发送数据的数据长度
* return   :NULL
***************************************************/
/*
void lcd_uart_send(u8 *buff,u32 len)
{
     write(lcd_uart_dev.fd,buff, len);
}
*/
/*************end file**************************/
