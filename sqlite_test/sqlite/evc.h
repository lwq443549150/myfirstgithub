/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：evc.h
*文件功能描述: CAN模块通讯头文件
*创建标识   ：xuchangwei 20170819
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

//充放电控制模块
#ifndef __EVC_H_
#define __EVC_H_

#include "sys_logic_control.h"

/**************节点控制指令****************************/
#define	SETID		0x0103	//设置节点号
#define	SETLIM	    0x0104	//设置硬件限制最大电压电流 v

#define	CLIG		0x0201	//控制灯点亮   v
#define	SETV		0x0202	//设置输出电压 v
#define	SETC		0x0205	//设置输出电流 v
#define	RNV			0x0341	//读节点电压   v
#define	RNC			0x0342	//读节点电流   v
#define	RSETM		0x0301	//读硬件设置电压   v
#define	RSTC		0x0303	//读节点状态和故障 v
#define	C_S_S		0x0106	//控制节点启动停止和充电放电 v
#define	DEBUG_MODE	0x0122	//调试模式
#define	READ_GET_VERSION			0x0306	//读取软件版本号

//模块状态
#define MODULE_LINK_OFF 0
#define MODULE_LINK_ON  1


//系统控制事件宏定义
#define LED_EVT_CTR_ON  			(1 << 0) 	//led 控制事件开
#define LED_EVT_CTR_OFF  			(1 << 1) 	//led 控制事件关
#define LED_EVT_CTR_CHARGE			(1 << 2) 	//整组充电
#define LED_EVT_CTR_DISCHARGE		(1 << 3) 	//整组放电
#define LED_EVT_CTR_RESET 			(1 << 4) 	//整组搁置

#define MODULE_EVT_CTR_SINGLE 	    (1 << 5) 	//模块单板控制
#define MODULE_EVT_DIR_CNV          (1 << 6) 	//充电放电模式直接切换事件 即充电直接切换到放电 或者放电直接切换到充电

//#define GROUP_MODULE_EVT_DEBUG  Bit(7) 	//将某一组设置调试模式

//外部变量
extern u8 div_cap_led[MAX_NODE][4];	//分容时led灯指示

//***外部函数****
extern void update_divide_capacity_led(u8 *buff,u16 len);
extern void led_group_on_evt(u8 group_id);
extern void led_group_off_evt(u8 group_id);
extern void evc_group_charge_evt(u8 group_id);
extern void evc_group_discharge_evt(u8 group_id);
extern void evc_group_reset_evt(u8 group_id);
extern void evc_module_ctr_evt(u8 module,u8 ch,u16 set_v,u16 set_i,u8 mode);
extern void evc_module_mode_dir_cnv_evt(u8 module,u8 ch,u16 set_v,u16 set_i,u8 mode);
extern void evc_module_ctr_clr(u8 module);
extern void evc_pthread_task(void);
extern u8   get_single_node_status(u8 module,u8 ch);
extern u8   module_single_ctr(u8 module, u8 *buff,u16 cmd);

#endif
/******************end file***************************/
