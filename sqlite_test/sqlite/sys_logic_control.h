/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ： sys_logic_control.h
*文件功能描述: 逻辑控制头文件
*创建标识   ：xuchangwei 20170819
*修改标识   : 20170819 初次创建
*修改描述   : 20170819 初次创建
***************************************************/

#ifndef __SYS_LOGIC_CONTROL_H_
#define __SYS_LOGIC_CONTROL_H_
#include <stdio.h>
#include "sys_cfg.h"

/***************20171109改单点控制***************************/

//****************************************

#define A(x) x

//#define MAX_NODE(x)            x                         //最大模块节点数量
#define MAX_NODE            128                         //最大模块节点数量
#define MODULE_CH_NUM       4                          //每个模块通道数量
#define MODULE_NUM          (MAX_NODE/MODULE_CH_NUM)   //模块数量
#define STEP_GROUP_SELECT   (MAX_NODE / 16 - 1)
#define MAX_STEP            10

#define SCAN_CYCLE		50//	每个单位200ms
#define SCAN_CLK		0.01	//10ms	扫描周期
#define Curr_uint	    10		 // 10mA
#define Vlot_uint		0.1	 // 0.1V
//define chrage instruction CODE
#define	CCv		0x11
#define	CCt		0x12
#define	CV		0x01			//多条件充电 恒流恒压充电
#define	CVc		0x21
#define	CVt		0x22
#define	REST	0x02			//搁置0x30
#define	DCCv	0x41
#define	DCCt	0x42
#define	DCV		0x03			//多条件放电0x43
#define	CYCL	0x50			//循环
#define	CEND	0xf0			//结束
#define HCV     0x04            //恒流充电

#define	CLEA	0xf1            //清除工步表
#define	RD_STEP	0xf2            //读取制定工步

#define STOP        0
#define CHARGE      1
#define DISCHARGE   2

#define   SYS_IDLE  	0			//空闲
#define   SYS_START  	1			//需要启动
#define   SYS_STOP  	2			//需要停止
#define   SYS_RUNNING   3			//运行中
#define   SYS_COMP      4			//完成
#define   SYS_DIVCAP    5			//分容中
#define   SYS_PAUSE     6			//暂停中
#define   SYS_DISABLE   7           //禁止失能

//设备工作状态
#define DEV_WORKING      0xAA
#define DEV_COMP         0xA0
#define DEV_MAN_STOP     0xA1

//当前工步测试类型
#define TEST_DIVCAP     0       //分容
#define TEST_RECHARGE   1       // 返充
//

#define NODE_ENABLE_BIT   0x01
#define NODE_RUN_BIT      0x02
#define NODE_CMP_BIT      0x04
#define NODE_LINKOFF_BIT  0x08
#define NODE_ERR_BIT   	  0x80


#define SAVE_GLOBAL_PAR     10
#define SAVE_RUNNING_DATA   40
#define SAVE_HISTORY_DATA   60
#define DELETE_CASH         3600

#define VERSION 1

//

#pragma pack(push) //保存对齐状态
#pragma pack(1)// 设定为4字节对齐

//20171111
typedef struct
{
	u8  type;           //CC_charge,CC_discharge,rest,CV_charge,0x00：充（恒流恒压) 0x01：搁 0x02：放 0x03：充（恒流)

	u16 parm1;          //上限电压
	u16 parm2;          //下限电压

	u32 cond1;          //电流

	u32 cond2;	        //终止电流
	u32 cond3;	        //终止时间
	u32	cond4;	        //终止容量

    u8 jump_step;       //子循环需要跳转的工步号 0 表示切换到下一步,非0表示需要跳转到相应到工号 循环开始行
    u8 sub_cyc;         //当前子循环需要循环到次数
}NODE_STEP_DEF;

//20171208 添加保存结构体
typedef struct{
    u8 cyc_son;         //子循环次数
    u8 step_mun;        //工步号
    u8 step_style;      //工步类型
    u8 node_status;     //节点状态
    u8 r_flag;          //返充标志
    u8 r_rio;           //返充比例
    u8 step_sum;        //工步总数*
    u8 now_cyc;         //当前循环数*
    u8 info;            //hardware info*
    u8 run_status;      //*
    u8 step_end;        //工步结束 0 表示当前工步未结束, 1 表示当前工步已结束*
    u8 working_status;	//0 未有启动的工步,0xAA表示工步未完成,0xA0 表示工步完成,0xA1工步未完成中途手动停止

    u16 cyc;            //主循环次数*
    u16 v;              //电压
    u16 cc_cv;          //恒流比*
    u16 plat_time;      //平台时间*
    u16 platform_v;     //平台电压*

    u32 i;              //电流
    u32 run_time;       //运行时间*
    u32 charge;         //容量
    u32 em;             //能量*
    u32 cc_capacity;    //steady current capacity: unit maH*
    u32 capacity;       //charge capacity, unit mah*
    u32 discapacity;    // discharge  capacity, unit:mah*
    u32 real_capacity;  //*
}SAVE_DATA_DEF;

//20180227
typedef struct
{
    u16 mun; //编号 代表第几个节点
    u8 sub_now_cyc; //子循环次数
    u8 now_step;    //工步号
    u8 work_type;   //工步类型
    u8 node_status; //节点状态
    u8 recharge_status; //返充标志
    u8 recharge_ratio;  //返充比例

    u16 cyc;    //主循环次数
    u16 v;  //电压
    u16 cc_cv;  //恒流比
    u16 platform_time;  //平台时间

    u32 i;  //电流
    u32 charge; //容量
    u32 em; //能量
    u32 time;   //运行时间
}NET_OFF_DATA_DEF;

//20171109,改单点控制
typedef struct
{
    struct{
        NODE_STEP_DEF  step[MAX_STEP];          //节点工步   *
        u8 step_num;        //工步总数*
        u16 platform_v;     //平台电压*

        u8 test_style;           //测试类型 1分容,2返充,3分容加返充
        u8 divcap_start_step;   //分容起始工步号
        u8 divcap_end_step;   //分容结束工步号

        u8 recharge_start_step;   //返充起始工步号
        u8 recharge_end_step;   //返充结束工步号
        u8 recharge_ratio;      //返充比例0~100%

        u16 fwsta;          //
        u16 v;              //unit: MV
        u32 i;              //unit: MA
        u32 time;           //unit: s(second)
        u16 platform_time;  //platform time,  unit: s(second)

        u16 cc_cv;           //steady current capacity ratio : 0~ 1000
        u32 cc_capacity;    //steady current capacity: unit maH
        u32 capacity;       //charge capacity, unit mah
        u32 discapacity;    // discharge  capacity, unit:mah
        u32 real_capacity;  //电池 真实容量值
        u32 recharge_cap;   //返充下发容量

        u8 info;            //hardware info
        u8 run_status;
                            /*//run status
                            #define   SYS_IDLE  	0			//空闲
                            #define   SYS_START  	1			//需要启动
                            #define   SYS_STOP  	2			//需要停止
                            #define   SYS_RUNNING   3			//运行中
                            #define   SYS_COMP      4			//完成
                            #define   SYS_DIVCAP    5			//分容中
                            #define   SYS_PAUSE     6			//暂停中

                            */
        u8 now_step;        //当前工步*
        u16 cyc;             //循环*
        u32 em;             //能量*
        u16 now_cyc;         //当前循环数*
        u8 step_end;        //工步结束 0 表示当前工步未结束, 1 表示当前工步已结束*
        //u8 step_type;     //工步类型
        u16 set_v;          //设置电压
        u16 set_i;          //设置电流
        u8 work_type;       //工作类型  搁置 充电 放电
        u8 node_status;
        u8 sub_now_cyc;     //子循环次数

        //启动状态 结束状态时电压电流参数保存
        u16 start_v;        //启动测试时到电压
        u32 start_i;
        u16 end_v;
        u32 end_i;
     //   u32 sys_handle_delay_time;  //延时处理
        u8 read_start_v_flag;   //起始电压读取标志 0表示 读取,1表示未读取
        u32 end_time;       //工步结束时刻的时间

        //节点运行使用到临时变量
        u8 err_cnt;
        u8 div_cap_led;
        u16 v_tmp;
        u32 time_tmp;

        u32 record_id;      //记录掉线记录的条数

        u16 precharge_v;    //启动充电之前的电压值
        u8 recharge_status;   //表示当前工步处于何种状态: 0分容  1返充
        u32 time_tmp_1;
    }node[MAX_NODE];           //max node

    struct{
        u8 link_off_cnt;
        u8 link_status;         //模块链接状态, MODULE_LINK_ON,在线, MODULE_LINK_ON,MODULE_LINK_OFF掉线
        u8 module_ver[10];     //module soft ver
    }module_info[MAX_NODE/MODULE_CH_NUM];    //模块总数
    u8 net_status;              //网络连接标志位 0x01 在线 0x02掉线
}BTS_DEF;

#pragma pack(pop) //保存对齐状态


//声明外部变量
extern int  save_history_flag;
extern  BTS_DEF bst;
extern SAVE_DATA_DEF save_real_data[MAX_NODE];
extern NET_OFF_DATA_DEF save_history_data[MAX_NODE][10];
extern void sys_logic_control_pthread_task(void);
extern void save_data_thread_task(void);
extern void sys_time_thread_task(void);
extern void  write_work_global_info(u8 group); //保存工步自行的数据信息
#endif

/******************end file***************************/
