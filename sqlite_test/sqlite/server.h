/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ： server.h
*文件功能描述: 网络通讯头文件
*创建标识   ：xuchangwei 20170819
*修改标识   : 20170819 初次创建
*修改描述   : 20170819 初次创建
***************************************************/

#ifndef __SERVER_H_
#define __SERVER_H_
#include "sys_cfg.h"

//后台通信组通道数量
#define SRV_GROUP_NUM          8     //后台通信协议定义共8组
#define SRV_GROUP_CH_NUM       16    //后台通信协议定义每组16通道
#define SRV_GROUP_MODULE_NUM   4     //每组模块数量
#define SRV_MODULE_CH_NUM      4     //每组模块通道数量

//校验码字节长度
#define CRC_CODE_LEN    2
//
#define SVR_START           0xAA
#define	OFF	0X65	//off node
#define	ON	0X66

#define	RC	 0x61		//read Current
#define	RT	 0x62		//read realtime nowstep_____
#define	C_V	 0x63		//read CC capacty % all capacty
#define	RBT	 0x64		//read read all bat test time in step work
#define	RV	 0x71		//read Vlotage
#define	REN	 0x81		//____读使能电池
#define	LEC	 0x82		//????
#define	LEC2 0x84		//128节点整组点亮
#define	ST	 0x85		//start charge
#define	STP	 0x86		//????
#define	CAP	 0x91		//read capacty
#define	DCA	 0x92		//_____读放电容量
#define	ENE	 0x93		//read energy
#define	STA	 0xa1		//read status
#define	CST	 0xb5		//clear status and flag
#define	SSR	 0xb6		//????
#define	DSS	 0xb7		//????
#define	RSP	 0xb2		//read work_step
#define	RCY	 0xb3		//_____读当前循环次数
#define	SWB	 0x80		//control bat SW
#define	SCC	 0xc1		//control screen for capacty
#define	SCD	 0xc2		//control screen for discapacty

//配置工步参数命令
#define CFG_STEP            0x0A      //配置工步
#define SYS_PARM            0X0B      //配置系统参数
#define CLR_OR_READ_STEP    0x0F      //清除或者读取工步
#define SIGNAL_BAT_CAP      0x11      //下发单点容量
#define SET_STEP_GLOBAL     0x13      //工步全局配置下发
#define READ_STEP_GLOBAL    0x14      //工步全局配置读取


//后台控制通信命令定义
#define NODE_ENABLE         0x12      //节点使能
#define CONTROL_LED         0x0E      //亮灯控制

#define READ_GROUP_RUN_DATA 0x10      //读取组节点运行数据
#define READ_SINGAL_BAT_CAP 0x15      //读取单点容量
#define SEARCH_HISTORY_DATA 0x16      //查询离线数据
#define READ_NODE_HIS_DATA  0x17      //读取历史数据

#define READ_STEP_STATUS    0x67      //读取组工步状态
#define READ_HISTORY_STATUS 0x68      //读取各组历史数据状态
#define READ_HISTORY_DATA   0x69      //读取各组历史数据
#define CTR_PUMP            0x0D      //气缸控制
#define READ_BAT_EN         0x81      //读电池使能情况
#define CTR_LED             0x84      //控制指示灯
#define START_STOP_CMD      0x0C      //启动/停止控制
#define CMP_DIVIDE_CAP      0x88      //完成分容
#define READ_BAT_EM         0x93      //读电池组储能
#define CLR_BAT_ERR         0xB5      //清除故障还原节点状态

#define NET_LEFT	   		0x02	    //网络离线
#define NET_ONLINE	   		0x01        //网络在线


/*
//配置工步参数命令
#define CFG_STEP            0xBB      //配置工步
#define CLR_OR_READ_STEP    0xcc      //清除或者读取工步

//后台控制通信命令定义
#define READ_GROUP_RUN_DATA 0x66      //读取组节点运行数据
#define READ_STEP_STATUS    0x67      //读取组工步状态
#define READ_HISTORY_STATUS 0x68      //读取各组历史数据状态
#define READ_HISTORY_DATA   0x69      //读取各组历史数据
#define CTR_PUMP            0x70      //气缸控制
#define READ_BAT_EN         0x81      //读电池使能情况
#define CTR_LED             0x84      //控制指示灯
#define START_STOP_CMD      0x85      //启动/停止控制
#define CMP_DIVIDE_CAP      0x88      //完成分容
#define READ_BAT_EM         0x93      //读电池组储能
#define CLR_BAT_ERR         0xB5      //清除故障还原节点状态
*/
//
#define TEST_STOP           0       //启动
#define TEST_START          1       //停止
#define ALRDY_START         0xFF    //已经启动

//设置字节对齐
#pragma pack(push)   //保存对齐状态
#pragma pack(1)
//后台通信结构体定义

typedef struct
{
    u8 start;      //起始位固定为0xAA
    u8 res;        //保留字
    u8 cmd;        //命令字
    u8 group;      //组号
    u8 data;       //数据
}tsSVR_CTR_STD_FH;     //控制通信协议标准协议头

typedef struct
{
    u8 start;    //起始位固定为0xAA
    u8 group;    //保留字
    u8 cmd;      //命令字
    u8 num;      //工步号
    u8 code;     //工步功能码
    u8 data;
}tsSVR_CFG_STD_FH;     // 配置通信协议标准协议头


//读取节点运行数据上行数据结构体
typedef struct
{
    u16 v[SRV_GROUP_CH_NUM];          //电压
    u16 i[SRV_GROUP_CH_NUM];          //电流
    u32 capacity[SRV_GROUP_CH_NUM];         //充电容量单位 mah
    u32 discapacity[SRV_GROUP_CH_NUM];      //放电容量 单位mah
    u16 Btime[SRV_GROUP_CH_NUM];            //工步运行时间 单位 s
    u8  en_sta[SRV_GROUP_CH_NUM];        //节点电池使能状态: 0 不使能, 1 使能
    u16 work_sta[SRV_GROUP_CH_NUM];      //节点工作状态
    u16 cc_ratio[SRV_GROUP_CH_NUM];         //恒流比 : 0 ～ 100
    u16 platform_time[SRV_GROUP_CH_NUM];    //放电平台时间
}tsRUN_DATA;

//每组工步状态
typedef struct
{
    u8  num;                     //工步号
    u8  type;                 //工步类型
    u16 cycle;                //循环次数
    u32 time;                 //工步时间 单位s
    u8  recharge_flag;        //返充状态： 0分容, 1返充

 /********返充********/
    u8  recharge_ratio;       //返充比例
}tsSTEP_STA;

//每组历史数据状态
typedef struct
{
    u8  sta;                     //历史数据状态 false 不存在, ture 存在
    u8  run_sta;                 //设备运行状态 false 停止状态, ture 运行中
}tsHISTORY_STA;


//配置工步数据结构体
typedef struct
{
    u8 type;        //0x00：充（恒流恒压)0x01：搁0x02：放0x03：充（恒流)
    u16 parm1;      //上限电压
    u16 parm2;      //下限电压
    u32 cond1;      //电流
    u32 cond2;      //终止电流
    u32 cond3;      //终止时间
    u32 cond4;      //终止容量
    u8  cond5;      //循环开始行 跳转工步号
    u8  cond6;      //内循环次数 子循环次数
}tsCFG_STEP_DATA;

// 工步全局配置下发结构体
typedef struct
{
    u16  cyc;           //主循环次数
    u16 platform_v;     //平台电压
    u8  con1;           //测试类型(1分容,2返充,3分容加返充)
    u8  con2;           //分容起始工步(1~254,只返充时为0xFF)
    u8  con3;           //分容结束工步(1~254,只返充时为0xFF)
    u8  con4;           //返充起始工步(1~254,只分容时为0xFF)
    u8  con5;           //返充结束工步(1~254,只分容时为0xFF)
    u8  con6;           //返充比例(0~100分容时为0xFF)
}tsCFG_STEP_CONFIG;

//配置系统设置参数结构体
typedef struct{
    u8  ligth_type;     //亮灯模式 0x00灭灯 0x01亮灯
    u16 parm1;          //合格低电压
    u16 parm2;          //合格高电压
    u16 parm3;          //最低保护电压
    u16 parm4;          //最高保护电压
    u16 parm5;          //最低保护电流
    u16 parm6;          //最高保护电流
    u8  parm7;          //容量模式
}tsCFG_SYS_PARM;

//上位机发送指定的格式
typedef struct {
    u8  start;          //开始标志 固定（AA）
    u16 dataLen;        //整包数据长度
    u16 startOption;    //节点起始位置
    u8  cmd;            //命令
    u16 readMun;        //要读取的节点数量
    u8  data;           //数据
}cmd_send_style;

//查找历史数据命令指定的格式
typedef struct {
    u8  start;          //开始标志 固定（AA）
    u16 dataLen;        //整包数据长度
    u16 startOption;    //节点起始位置
    u8  cmd;            //命令
    u8  data;           //数据
}cmd_search_hisdata_style;

//实时数据参数格式
typedef struct {
    u8  cycMun;             //循环次数
    u8  stepMun;            //工步号
    u8  stepStyle;          //工步类型
    u16 V;                  //电压
    u16 I;                  //电流
    u32 cap;                //容量
    u32 power;              //能量
    u8  ccRatio;            //恒流比
    u16 platTime;           //平台时间
    u16 runTime;            //运行时间
    u8  noteStatue;         //节点状态
    u8  returnFlag;         //返充标志
    u8  returnRatio;        //返充比例
}realtime_data_style;

typedef struct {
    u16  cycMun;            //主循环次数
    u8  sub_cycMUn;         //子循环次数
    u8  stepMun;            //工步号
    u8  stepStyle;          //工步类型
    u16 V;                  //电压
    u32 I;                  //电流
    u32 cap;                //容量
    u32 power;              //能量
    u16 ccRatio;            //恒流比
    u16 platTime;           //平台时间
    u32 runTime;            //运行时间
    u8  noteStatue;         //节点状态
    u8  returnFlag;         //返充标志
    u8  returnRatio;        //返充比例
}realtime_data_style_new;

//读取工步数据格式
typedef struct{
    u8  start;          //开始标志 固定（AA）
    u16 dataLen;        //整包数据长度
    u16 startOption;    //节点起始位置
    u8  cmd;            //命令
    u8  step_sum;       //工步总数
    u8  data;           //数据
}read_step_style;

//工步全局配置读取
typedef struct{
    u8  start;          //开始标志 固定（AA）
    u16 dataLen;        //整包数据长度
    u16 startOption;    //节点起始位置
    u8  cmd;            //命令
    u8  data;           //数据
}read_global_step;

//节点信息
typedef struct
{
	u8  s_type;          //CC_charge,CC_discharge,rest,CV_charge,
	u16 s_parm1;
	u16 s_parm2;
	u32 s_cond1;    //stop vlotage
	u32 s_cond2;	//stop current
	u32 s_cond3;	//time limit
	u32	s_cond4;	//capt limit
	u8  s_cond5;
	u8  s_cond6;
}step_data;

typedef struct {
    u32 read_cap;
}send_cap;


#pragma pack(pop)  //恢复对齐状态

extern void server_pthread_task(void);
extern void tcp_communication_init(void);
extern void udp_communication_init(void);
extern void disconnect_udp(void);
#endif

/***************end file*******************************/

