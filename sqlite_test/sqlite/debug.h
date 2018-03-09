/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：debug.h
*文件功能描述: 调试开关头文件
*创建标识   ：xuchangwei 20170819
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

#ifndef _DEBUG_H
#define _DEBUG_H

#define DEBUG_SWITCH_ON 1

#if DEBUG_SWITCH_ON > 0

#define LCD_DEBUG_CONFIG 1
#define BMS_DEBUG_CONFIG 1
#define NET_DEBUG_CONFIG 1
#define BAT_DEBUG_CONFIG 1
#define SQLITE_DEBUG_CONFIG 1

#else

#define LCD_DEBUG_CONFIG 0
#define BMS_DEBUG_CONFIG 0
#define NET_DEBUG_CONFIG 0
#define BAT_DEBUG_CONFIG 0
#define SQLITE_DEBUG_CONFIG 0

#endif

#endif
