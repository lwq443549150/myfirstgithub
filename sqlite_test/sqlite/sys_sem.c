/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名        sys_sem.c
*文件功能描述:信号量处理函数
*创建标识   ：xuchangwei 20170819
//
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <unistd.h>
#include <semaphore.h>  //信号量
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "sys_sem.h"

/**************************************************
* name     : setsem_wait
* funtion  : 信号等待函数
* parameter：*sem 信号量 nano_sec 设置等待时间 sec返回
* return   :
***************************************************/
int setsem_wait(sem_t *sem,int nano_sec,int sec)
{
    struct timespec ts;
    if ( clock_gettime( CLOCK_REALTIME,&ts ) < 0 )
       return -1;
    ts.tv_sec  += sec;
    ts.tv_nsec += nano_sec;
    ts.tv_sec += ts.tv_nsec/NSECTOSEC; //Nanoseconds [0 .. 999999999]
    ts.tv_nsec = ts.tv_nsec%NSECTOSEC;
    return sem_timedwait(sem,&ts );
}

/*******************end file*************************/
