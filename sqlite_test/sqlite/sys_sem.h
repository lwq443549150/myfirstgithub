/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ： sys_sem.h
*文件功能描述: 信号量头文件
*创建标识   ：xuchangwei 20170819
*修改标识   : 20170819 初次创建
*修改描述   : 20170819 初次创建
***************************************************/

#ifndef __SYS_SEM_H_
#define __SYS_SEM_H_
#include <semaphore.h>  //信号量

#define NSECTOSEC    1000000000

//外部变量的相关 声明
extern  int setsem_wait(sem_t *sem,int nano_sec,int sec);
#endif

