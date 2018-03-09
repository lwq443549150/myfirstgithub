/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：main.c
*文件功能描述: 系统初始化,创建系统线程
*创建标识   ：xuchangwei 20170819
//
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/
#include <pthread.h>
#include <stdio.h>
#include"hw.h"
#include"sys_cfg.h"
#include <unistd.h>
#include "lcd.h"
#include "server.h"
#include "evc.h"
#include "sys_logic_control.h"
#include "ini_file.h"
#include "sqlite3.h"
#include "sqlite_opera.h"


//变量的相关定义
pthread_t lcd_thread;                  //线程指示符指针
pthread_t server_thread;               //后台服务线程指示符指针
pthread_t evc_thread;                  //DCDC充放电模块线程指示符指针
pthread_t sys_logic_control_thread;    //分容柜逻辑控制线程
pthread_t sys_time_thread;             //定时器处理线程
pthread_t save_data_thread;            //保存数据处理线程

sys_cfg_def sys_temp;
#define SQLITE3_STATIC

volatile int save_data_flag = 0;    //保存数据标志位

//主函数
int main(void)
{
    int ret;

    creat_data_db_and_table();      //创建数据库相关表格
    ret = init_sysconfig_opera();   //恢复系统参数 如果第一次，则进行初始化
    if(ret == 0){
        #if DEBUG_SWITCH_ON > 0
        printf("default parament\r\n");
        #endif
        memcpy(&sys_cfg, &sys_cfg_default, sizeof(sys_cfg_def));  //设置为默认参数
        set_sys_config_values(sys_cfg_default);
    }

    read_step_run_status();     //读工步
    recovery_gloabal_info();    //恢复全局变量参数
    recover_stepcfg_fun();      //恢复工步全局变量
    peripherals_init();         //初始化外设
    usleep(10*1000);

    pthread_create(&lcd_thread,NULL,(void *(*)(void *))lcd_pthread_task,NULL);         //lcd 通信线程
    pthread_create(&server_thread,NULL,(void *(*)(void *))server_pthread_task,NULL);   //以太网后台通信线程
    pthread_create(&evc_thread,NULL,(void *(*)(void *))evc_pthread_task,NULL);         //充电模块
    pthread_create(&sys_logic_control_thread,NULL,(void *(*)(void *))sys_logic_control_pthread_task,NULL);  //电源管理模块
    pthread_create(&sys_time_thread, NULL, (void *(*)(void *))sys_time_thread_task, NULL);
    pthread_create(&save_data_thread, NULL, (void *(*)(void *))save_data_thread_task, NULL);

    while(1)
    {
        //添加脚本，判断是否插入的TF卡中有需要更新的程序，若有，进行程序的替换
        system("./copy.sh");
        usleep(1000);
    }

   return 0;
}

/*********************end file***************************/

