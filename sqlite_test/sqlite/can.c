/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：can.c
*文件功能描述:can相关到初始化
*创建标识   ：xuchangwei 20170819
//
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <getopt.h>


int can_fd;

/**************************************************
* name      :   can_init
* funtion   :   can 初始化
* parameter :   void
* return    :   void
***************************************************/
void can_init(void)
{
    int ret;
    struct ifreq ifr;
    struct sockaddr_can addr;

    /* create a socket */
    can_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if(can_fd < 0 )
    {
            return;
    }

    int loopback = 0; /* 0 = disabled, 1 = enabled (default) */
  // setsockopt(can_fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
  // setsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&so_timeo,sizeof(so_timeo));  //将udp发送超时设置为1秒

    strcpy(ifr.ifr_name, "can0");   //设备名can0对应板上的CAN2接口 设备名can1对应板上的can1接口
    ret = ioctl(can_fd, SIOCGIFINDEX, &ifr);
    if( ret < 0 )
    {
            return;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(can_fd, (struct sockaddr *)&addr, sizeof(addr));
    setsockopt(can_fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
}

/***************end file****************************/
