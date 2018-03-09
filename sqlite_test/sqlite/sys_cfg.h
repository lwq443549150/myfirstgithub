
#ifndef __SYS_CFG_H_
#define __SYS_CFG_H_
#include <arpa/inet.h>
#define  TRUE               1
#define  FALSE              0

#define u64 unsigned long long
#define u32 unsigned int
#define u16 unsigned short
#define u8  unsigned char

#define s64 signed long long
#define s32 int
#define s16 short int
#define s8  char


//系统参数设置
typedef struct
{
	u16 valid;  //标志是否有效  == 0xAAAA 有效 用于第一次上电

	u8 lip1;  //本机IP
	u8 lip2;
	u8 lip3;
	u8 lip4;

	u8 drip1;  //网关IP
	u8 drip2;
	u8 drip3;
	u8 drip4;

	u8 netmask1;  //子网掩码
	u8 netmask2;
	u8 netmask3;
	u8 netmask4;

	u8 rip1;  //远程IP
	u8 rip2;
	u8 rip3;
	u8 rip4;
	u16 rport;  //远程端口

	u8 led_lighting_mode;			//低压检测亮灯 模式 0灭  1亮灯
	u16 low_volt;					//合格低电压
	u16 high_volt;					//合格高电压

	u16 protect_low_v;		//最低保护电压
	u16 protect_high_v;		//最高保护电压
	u16 protect_low_i;		//最低保护电流
	u16 protect_high_i;		//最高保护电流
    u8 capacityMode;                //容量模式:0x00小于60AH，0x01大于65AH
}sys_cfg_def;

//外部变量的相关 声明
extern sys_cfg_def sys_cfg;
extern const sys_cfg_def sys_cfg_default;

extern  pthread_t lcd_thread;   //

/**********************声明外部函数*******************/
extern s32 sys_cfg_read(void);             //读取设备配置参数
#endif

/********************end file*******************************/

