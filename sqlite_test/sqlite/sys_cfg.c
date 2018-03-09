/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：sys_cfg.c
*文件功能描述:系统参数配置
*创建标识   ：xuchangwei 20170819
//
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

#include "sys_cfg.h"
#include <string.h>
#include <stdio.h>
#include "ini_file.h"
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

//系统配置参数
sys_cfg_def sys_cfg;
const sys_cfg_def sys_cfg_default =
{
    0xAAAA,
    192,  //本机IP
    168,
    1,
    3,

    192,  //网关IP
    168,
    1,
    1,

    255,    //子网掩码
    255,
    255,
    0,

    192,  //远程IP
    168,
    1,
    1,

    8080,  //远程端口

    1,         	//亮灯模式
    3315,		//合格低电压    单位 1mv
    3360, 		//合格高电压    单位 1mv
    0, 			//最低保护电压   单位 1mv
    3700, 		//最高保护电压   单位 1mv
    50,         //最低保护电流   单位 1ma
    9500,       //最高保护电流   单位 1ma
    0
};

/******************end file*******************************/
