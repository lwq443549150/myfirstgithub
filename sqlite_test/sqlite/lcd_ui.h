/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ： lcd_ui.h
*文件功能描述: lcd界面显示头文件
*创建标识   ：xuchangwei 20170819
*修改标识   : 20170819 初次创建
*修改描述   : 20170819 初次创建
***************************************************/

#ifndef __LCD_UI_H_
#define __LCD_UI_H_
#include"lcd.h"

/********************屏幕页面定义*************************************************/
//电池处在的状态
#define LOGE_PAGE								0  //logo页
//监控界面
#define ALL_MONNTORING_PAGE 				    1  //主监控页 对8组电池进行总的监控（状态查看/开始操作/停止操作）
#define SINGLE_MONNTORING_PAGE 					2  //电池监控页 对16组电池进行监控（状态查看/分段操作/分段结果查看）
#define OTHER_PASSWORD_PAGE						3  //密码页
#define SYS_OPTION_PAGE                         4   //系统功能选项
#define NET_CONFIG_PAGE                         5   //网络配置
#define STEP_CONFIG_PAGE                        6   //工步组选择
#define MODULE_INFO_PAGE                        7   //模块信息查询
#define SYS_CONFIG_PAGE                         8   //系统配置
#define STEP_INFO_PAGE                          9   //工步详情
#define NODE_REAL_DATA                          10  //节点电池实时数据
#define CHECK_PASSWAD_PAGE                      14  //密码校正页面
#define OPEROTOR_PAGE                           15  //操作人员操作页面


#define	CHACK_PAGE								04  //巡检页面
#define	STEP_CHECK_PAGE							05  //工步查询
#define	STEP_CHECK_GROUP_PAGE					06  //工步查询组选择
#define	SYS_PARAMETER_SET_PAGE	   				07  //工步查询组选择
#define	NODE_RUN_STATUS_PAGE	   				8  //工步查询组选择
#define SINGNAL_STEP_STATUS                     23  //工步运行状态

//其他辅助界面
#define OTHER_FUTUAL_KEYBOARD 				    13  //全键盘
#define OTHER_NUMBER_KEYBOARD 				    14  //数字键盘
#define OTHER_WARNING_PAGE 						15  //报警页

//维护界面
#define MAINTENANCE_PAGE 						16  //设置主页面
#define MAINTENANCE_NETWORK_PAGE 				17  //网络设置页面
//电池显示页面
#define BAT_PAGE 							    18  //设置主页面



//组处在的状态
#define STAGE_OFFLINE 		0x00		//掉线
#define STAGE_DISABLE 		0x01		//禁用
#define STAGE_CFG			0x02		//工步还未配置完成
#define STAGE_IDLE		    0x03		//闲置
#define STAGE_WORKING 		0X04		//正在测试
#define STAGE_PAUSE		 	0X05		//暂停测试
#define STAGE_ALLDONE 		0X06		//完成测试，待点击分段按键
#define STAGE_ALARM 		0x07		//有故障
#define STAGE_OTHER 		0x08		//其他工步类型 老的屏幕程序会显示"段1"

#define STAGE_DIVCAP 	    0X0C		//分容中		"老的屏幕程序会显示段4"
#define STAGE_CHARGE 	    0X0D		//充电中
#define STAGE_DISCHARGE		0x0E		//放电中
#define STAGE_RESET			0x0F		//静置

#define STAGE_HARD          0x10        //硬件故障
#define STAGE_SAMPLE        0x11        //采样故障

#define STAGE_DIVCAPCITY	0x1F		//分容阶段

extern page_t*  UI_data[];
extern u8       read_stage_state(u16 group);
#endif

/**************************end file*********************************/


