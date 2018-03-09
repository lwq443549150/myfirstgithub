/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：sys_logic_control.c
*文件功能描述:电池运行控制
*创建标识   ：xuchangwei 20170819
//
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>  //信号量
#include <signal.h>
#include "evc.h"
#include "sqlite_opera.h"
#include "io_control.h"
#include "sys_logic_control.h"

#include <linux/unistd.h>     /* 包含调用 _syscallX 宏等相关信息*/
#include <linux/kernel.h>     /* 包含sysinfo结构体信息*/

extern          sem_t   save_step_sem;    //保存工步信号量
extern volatile int     save_data_flag;   //保存工步
extern volatile int     delete_table_mun;

static  int             time_cnt = 0;    //计时定数器
        int             save_history_flag = 0; //保存历史数据标志位
        u32             sys_running_time,sys_running_time_seconds = 0;
        sem_t           save_global_par_sem, save_running_data_sem, save_history_data_sem, \
                        save_global_par_sem_1, delete_data_sem, delete_cash_sem;
        BTS_DEF         bst;
        SAVE_DATA_DEF   save_real_data[MAX_NODE];   //保存实时数据结构体数组
        NET_OFF_DATA_DEF save_history_data[MAX_NODE][10];   //保存历史数据结构体数组

/***********函数声明************************************/
static  void next_step_handle(u16 n);
        void write_work_global_info(u8 group);
        void copy_real_data(u8 group);



int cat_mmp()
{
    struct sysinfo s_info;
    int error;

    error = sysinfo(&s_info);
    printf("Uptime = %ds\n\n" "RAM: total %lu used = %lu, free = %lu\n",
           s_info.uptime,
           s_info.totalram/1024,(s_info.totalram/1024-s_info.freeram/1024),s_info.freeram/1024);
    return 0;
}

/******************************************************************
* 函数名: get_sys_running_time
* 功能:   获取系统运行时间 单位1ms
* 参数：
* 返回值: 系统运行时间
******************************************************************/
u32 get_sys_running_time(void)
{
	return sys_running_time;
}

/******************************************************************
* 函数名: get_sys_running_time_seconds
* 功能:   获取系统运行时间 单位1s
* 参数：
* 返回值: 系统运行时间
******************************************************************/
u32 get_sys_running_time_seconds(void)
{
	return sys_running_time_seconds;
}

/******************************************************************
* 函数名: reset_module_work
* 功能:   重设电池状态
* 参数：  u8 signal_bat
* 返回值:
******************************************************************/
void reset_module_work(u8 signal_bat)
{
    u8 i;
    u8 module;
    u8 chnal[4]={0};
    u8 cnt;		            //通道
    u8 on_off[8]={0};
    u8 set_v[8]={0};        //设置电压
    u8 set_i[8]={0};        //设置电流

    module  = signal_bat / 4;
    cnt     = signal_bat % 4;

    if(cnt == 0){
        chnal[1] = signal_bat + 1;
        chnal[2] = signal_bat + 2;
        chnal[3] = signal_bat + 3;
    }else if(cnt == 1){
        chnal[0] = signal_bat - 1;
        chnal[2] = signal_bat + 1;
        chnal[3] = signal_bat + 2;
    }else if(cnt == 2){
        chnal[0] = signal_bat - 2;
        chnal[1] = signal_bat - 1;
        chnal[3] = signal_bat + 1;
    }else if(cnt == 3){
        chnal[0] = signal_bat - 3;
        chnal[1] = signal_bat - 2;
        chnal[2] = signal_bat - 1;
    }

    for(i = 0; i < 4; i++){
        if( cnt == i){
            on_off[i*2+1] = STOP;   //搁置
            set_v[i*2]    = 0;
            set_v[i*2+1]  = 0;
            set_i[i*2]    = 0;
            set_i[i*2+1]  = 0;
        }else{
            if(bst.node[chnal[i]].work_type == CV || bst.node[chnal[i]].work_type == HCV){
                on_off[i*2+1] = CHARGE;
                set_v[i*2]    = (u8)(bst.node[chnal[i]].set_v >> 8);
                set_v[i*2+1]  = (u8)(bst.node[chnal[i]].set_v & 0x00ff);
                set_i[i*2]    = (u8)(bst.node[chnal[i]].set_i >> 8);
                set_i[i*2+1]  = (u8)(bst.node[chnal[i]].set_i & 0x00ff);
            }else if(bst.node[chnal[i]].work_type == DCV){
                on_off[i*2+1] = DISCHARGE;
                set_v[i*2]    = (u8)(bst.node[chnal[i]].set_v >> 8);
                set_v[i*2+1]  = (u8)(bst.node[chnal[i]].set_v & 0x00ff);
                set_i[i*2]    = (u8)(bst.node[chnal[i]].set_i >> 8);
                set_i[i*2+1]  = (u8)(bst.node[chnal[i]].set_i & 0x00ff);
            }
        }
    }

    module_single_ctr(module+1, &set_i[0], SETC);     //设置电流
    module_single_ctr(module+1, &set_v[0], SETV);     //设置电压
    module_single_ctr(module+1, on_off, C_S_S);
}

/******************************************************************
* 函数名: node_start_init
* 功能:   节点启动初始化函数
* 参数：  n 节点
* 返回值:
******************************************************************/
static void node_start_init(u16 n)
{
    bst.node[n].time            = 0;
    bst.node[n].platform_time   = 0;
    bst.node[n].cc_cv           = 0;
    bst.node[n].cc_capacity     = 0;
    bst.node[n].capacity        = 0;
    bst.node[n].discapacity     = 0;
    bst.node[n].info            = NODE_ENABLE_BIT | NODE_RUN_BIT;
    bst.node[n].em              = 0;
    bst.node[n].now_cyc         = 0;
    bst.node[n].now_step        = 0;
    bst.node[n].step_end        = 0;
    bst.node[n].sub_now_cyc     = 0;
}

/******************************************************************
* 函数名: update_start_test_v_i
* 功能:   更新启动电流电压
* 参数：  n 节点
* 返回值:
******************************************************************/
static void update_start_test_v_i(u8 n)
{
    u8 now_step,type;
    u16 set_i;

    now_step = bst.node[n].now_step;         //当前工步
    type     = bst.node[n].step[now_step].type;
    set_i    = (u16)bst.node[n].step[now_step].cond1;

    if(type == CV || type == HCV || type == DCV)
    {
        bst.node[n].start_v = bst.node[n].v;
        bst.node[n].start_i = set_i;
    }
    else
    {
        bst.node[n].start_v = bst.node[n].v;
        bst.node[n].start_i = 0;
    }

    bst.node[n].read_start_v_flag = 1;   //上位机需要上传起始电压电流
    bst.node[n].end_v             = bst.node[n].v;
    bst.node[n].end_i             = 0;
}

/******************************************************************
* 函数名: node_start
* 功能:   节点启动
* 参数：  n 节点
* 返回值:
******************************************************************/
static void node_start(u16 n)
{
    u8 temp,num;

    if(bst.node[n].run_status == SYS_START)		//需要启动
	{

		if(bst.node[n].step_num < 1)
		{
			bst.node[n].run_status = SYS_IDLE;
			return ;
		}

        if(save_real_data[n].working_status == 0xAA){
            bst.node[n].run_status = SYS_PAUSE;       //如果上一次未完成工步 则重启后置为暂停

        }else{
            node_start_init(n);      //节点启动初始化
            save_real_data[n].working_status = 0xAA;
        }

		num  = bst.node[n].now_step;
		temp = bst.node[n].step[num].type;		//当前工步类型
		bst.node[n].time_tmp_1 = get_sys_running_time_seconds();    //获取启动时候的秒数

        update_start_test_v_i(n); 	//记录起始电压电流参数
        if(temp == CV || temp == HCV || temp == DCV || temp == REST)
        {
            bst.node[n].run_status = SYS_RUNNING;       //启动运行
            bst.node[n].work_type = temp;
            if(temp == CV || temp == HCV)
            {
                bst.node[n].node_status = 3;   //恒流充电
                bst.node[n].set_v = bst.node[n].step[num].parm1;    //充电用上限电压
                bst.node[n].set_i = (u16)bst.node[n].step[num].cond1;
                bst.node[n].precharge_v = bst.node[n].v;
            }
            else if(temp == DCV)
            {
                bst.node[n].node_status = 4;   //节点状态表示为恒流放电
                bst.node[n].set_v = bst.node[n].step[num].parm2;    //放电用下限电压
                bst.node[n].set_i = (u16)bst.node[n].step[num].cond1;
            }
            else
            {
                bst.node[n].node_status = 1;   //搁置
                bst.node[n].set_v = 0;
                bst.node[n].set_i = 0;
            }

            //判断当前工步是否为返充工步
            if((num < (bst.node[n].divcap_end_step - 1)) && (bst.node[n].divcap_end_step != 0xff ) && (bst.node[n].divcap_end_step > 0))
            {
                bst.node[n].recharge_status = 0;  //分容
            }
            else if((num >= (bst.node[n].recharge_start_step - 1)) && (bst.node[n].recharge_start_step != 0xff) && (bst.node[n].recharge_start_step > 0))
            {
                bst.node[n].recharge_status = 1;  //返充
            }
        }
	}
}

/******************************************************************
* 函数名: step_time_end_poll
* 功能:   结束时间查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static u8 step_time_end_poll(u16 n)
{
	u8 now_step;
	u32 stop_t;			//停止时间

	now_step = bst.node[n].now_step;
	stop_t   = bst.node[n].step[now_step].cond3;

	if(bst.node[n].time >= stop_t && stop_t != 0)
	{
		//达到时间时先判断
        //只有在节点没有 停止时才更新
        if( bst.node[n].node_status < 10)
        {
            bst.node[n].node_status = 20;
        }
		//达到时间
		bst.node[n].step_end = 1;

		//达到中止条件时保存实时数据
        copy_real_data(n);
		return 1;
	}
	return 0;
}

/******************************************************************
* 函数名: charge_vot_end_poll
* 功能:   充电终止电压查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void charge_vot_end_poll(u16 n)
{
	u8 now_step;
	u16 stop_v;			//停止电压

    if(bst.node[n].step_end !=0)
    {
        return;
    }
	now_step = bst.node[n].now_step;
	stop_v   = bst.node[n].step[now_step].parm1;
	if((bst.node[n].v >= (stop_v)) && (stop_v != 0))     //判断是否达到 停止电压
    {
        bst.node[n].step_end    = 1;
        bst.node[n].node_status = 21;

        copy_real_data(n);  //达到中止条件时保存实时数据

        #if BAT_DEBUG_CONFIG > 0
        printf("charge_vot_end_poll %d\r\n", n);
        #endif
    }
}

/******************************************************************
* 函数名: charge_curr_end_poll
* 功能:   充电终止电流查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void charge_curr_end_poll(u16 n)
{
	u8 now_step;
	u16 stop_i;			//设定电流

    if(bst.node[n].step_end !=0)
    {
        return;
    }
	now_step = bst.node[n].now_step;
	stop_i   = (u16)bst.node[n].step[now_step].cond2;
	if((bst.node[n].i <= stop_i)&& (stop_i != 0))  //判断是否达到 终止电流条件
    {
        bst.node[n].step_end    = 1;
        bst.node[n].node_status = 22; //达到终止电流

        copy_real_data(n);   //达到中止条件时保存实时数据

        #if BAT_DEBUG_CONFIG > 0
        printf("bst.node[%d].i = %d stop_i = %d\r\n", n, bst.node[n].i, stop_i);
        printf("charge_curr_end_poll %d\r\n", n);
        #endif
    }
}

/******************************************************************
* 函数名: charge_capacity_end_poll
* 功能:   工步终止容量查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void charge_capacity_end_poll(u16 n)
{
	u16 cap_tmp;
	u8 now_step;
	u32 set_cap;			//设定容量

    if(bst.node[n].step_end !=0)
    {
        return;
    }
	now_step = bst.node[n].now_step;
	set_cap  = bst.node[n].step[now_step].cond4;
	cap_tmp  = (u16)(bst.node[n].capacity / 3600);
	if((cap_tmp >= set_cap)&& (set_cap != 0))  //达到容量
	{
	    bst.node[n].step_end    = 1;
        bst.node[n].node_status = 23; //达到终止容量

        copy_real_data(n); //达到中止条件时保存实时数据

        #if BAT_DEBUG_CONFIG > 0
        printf("charge_capacity_end_poll %d \r\n", n);
        #endif
	}
}

/******************************************************************
* 函数名: charge_curr_err_poll
* 功能:   充电电流异常查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void charge_curr_err_poll(u16 n)
{
	u8 now_step;
	u16 set_i;			//设定电流

    if(bst.node[n].step_end != 0)
    {
        return;
    }
	now_step = bst.node[n].now_step;
	set_i    = (u16)bst.node[n].step[now_step].cond1;

    if(bst.node[n].i > (set_i + 300))			//判断充电电流值是否大于设定值150ma 充电电流过大
    {
        bst.node[n].step_end    = 1;
        bst.node[n].node_status = 24; //电流过大

        copy_real_data(n);  //达到中止条件时保存实时数据

        #if BAT_DEBUG_CONFIG > 0
        printf("charge_curr_err_poll\r\n");
        printf("bst.node[%d].i = %d set_i = %d\r\n", n, bst.node[n].i, set_i);
        #endif
    }
}

/******************************************************************
* 函数名: charge_vot_err_poll
* 功能:   充电电压异常查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void charge_vot_err_poll(u16 n)
{
	u8 now_step;
	u16 set_v;			//设定电压

    if(bst.node[n].step_end !=0)
    {
        return;
    }
    now_step =  bst.node[n].now_step;
	set_v = bst.node[n].step[now_step].parm1;

    if(bst.node[n].v >(set_v + 40))		//充电电压过大
    {
        bst.node[n].step_end = 1;
        bst.node[n].node_status = 25; //电压偏大停止

        //达到中止条件时保存实时数据
        copy_real_data(n);

        #if BAT_DEBUG_CONFIG > 0
        printf("charge_vot_err_poll==1 n = %d\r\n", n);
        printf("bst.node[n].v = %d set_v = %d\r\n" , bst.node[n].v, set_v);
        #endif
    }
    if(bst.node[n].v <= 400)		//电压小于300MV
    {
        bst.node[n].step_end = 1;       //充电电压过小
        bst.node[n].node_status = 29;   //电压偏小停止

        copy_real_data(n);      //达到中止条件时保存实时数据

        #if BAT_DEBUG_CONFIG > 0
        printf("bst.node[%d].time = %d\r\n", n, bst.node[n].time);
        printf("charge_vot_err_poll==2 n = %d\r\n", n);
        printf("bst.node[n].v = %d set_v = %d\r\n" , bst.node[n].v, set_v);
        #endif
    }
}

/******************************************************************
* 函数名: charge_status_err_poll
* 功能:   充电状态异常查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void charge_status_err_poll(u16 n)
{
	u8 now_step;
	u16 set_i;			//设定电流
	u16 set_v;			//设定电压

    if(bst.node[n].step_end !=0)
    {
        return;
    }
    now_step =  bst.node[n].now_step;
	set_i = (u16)bst.node[n].step[now_step].cond1;
	set_v = bst.node[n].step[now_step].parm1;

	if((bst.node[n].v < set_v - 40) && \
        bst.node[n].i < (set_i - 500))      //既不是恒压充电也不是恒流充电
	{
        bst.node[n].step_end = 1;
        bst.node[n].node_status = 26; //充电恒流恒压异常

        //达到中止条件时保存实时数据
        copy_real_data(n);

        #if BAT_DEBUG_CONFIG > 0
        printf("charge_status_err_poll %d\r\n", n);
        printf("bst.node[n].v = %d, set_v = %d\r\n ", bst.node[n].v, set_v);
        printf("bst.node[n].i = %d, set_i = %d\r\n ", bst.node[n].i, set_i);
        #endif
	}
}

/******************************************************************
* 函数名: charge_bat_err_poll
* 功能:   充电电池异常查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void charge_bat_err_poll(u16 n)
{
	u8 now_step;
	u16 set_i;			//设定电流

    if(bst.node[n].step_end !=0)
    {
        return;
    }

    now_step =  bst.node[n].now_step;
    set_i    = (u16)bst.node[n].step[now_step].cond1;

	if( bst.node[n].v_tmp !=  bst.node[n].v)
	{
		bst.node[n].v_tmp 	 = bst.node[n].v;
		bst.node[n].time_tmp =  bst.node[n].time;
	}
	else
	{
			if(bst.node[n].v < 2000)		//电压小于2V 持续2分钟
			{
                if(set_i >= 3000)  //3000-10a
                {
                    if(bst.node[n].time  > 3* 60)
                        bst.node[n].step_end = 1;
                }
                else if(bst.node[n].time  > 5* 60) //运行时间超过10分钟
                {
                    bst.node[n].step_end = 1;
                }
			}
			else if(bst.node[n].v < 2600)		//电压小于2.6V 持续2分钟
            {
                if(set_i  >= 10000)
                {
                    if(bst.node[n].time > 40)    //1分钟未达到2.6
                        bst.node[n].step_end = 1;
                }
                else if(set_i  >= 6000)
                {
                    if(bst.node[n].time > 1.2* 60 )
                        bst.node[n].step_end = 1;
                }
                else if(set_i  >= 5500)
                {
                    if(bst.node[n].time> 1.8* 60 )
                        bst.node[n].step_end = 1;
                }
                else if(set_i  >= 4500)
                {
                    if(bst.node[n].time > 3.5* 60 )
                        bst.node[n].step_end = 1;
                }
                else if(set_i  >= 2500)
                {
                    if(bst.node[n].time > 10* 60 )
                        bst.node[n].step_end = 1;
                }
                else if(set_i  >= 1250)
                {
                    if(bst.node[n].time > 20* 60 )
                        bst.node[n].step_end = 1;
                }
                else if(set_i  >= 625)
                {
                    if(bst.node[n].time > 30* 60 )
                        bst.node[n].step_end = 1;
                }
                else if(bst.node[n].time > 40* 60 ) //运行时间超过10分钟
                {      //只要超过10分钟不管什么,电压没有超过2V都得停止
                    bst.node[n].step_end = 1;
                }
            }
            else if(bst.node[n].v < 2800)		//电压小于2.9V 持续2分钟
            {
                if(set_i >= 10000)
                {
                    if(bst.node[n].time > 1.2* 60 )
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 6000)
                {
                    if(bst.node[n].time > 1.8* 60)
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 5500)
                {
                    if(bst.node[n].time > 2.5* 60)
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 4500)
                {
                    if(bst.node[n].time > 8* 60)
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 2500)
                {
                    if(bst.node[n].time > 16* 60 )
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 1250)
                {
                    if(bst.node[n].time > 32* 60)
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 625)
                {
                    if(bst.node[n].time> 60* 60)
                        bst.node[n].step_end = 1;
                }
            }

				//电压不变持续时间判断
            if(bst.node[n].v < (3100))  //电压小于3250mv才进行判断
            {  //按照电流档位来分
                if(set_i >= 10000)  //5500
                {
                    if((bst.node[n].time - bst.node[n].time_tmp) > 5* 60 ) //超过6分钟
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 5495)  //5500
                {
                    if((bst.node[n].time - bst.node[n].time_tmp) > 7* 60) //超过6分钟
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 2745)  //2750
                {
                    if((bst.node[n].time - bst.node[n].time_tmp) > 14*60 ) //超过6分钟
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 1370)  //1375
                {
                    if((bst.node[n].time - bst.node[n].time_tmp) > 28*60) //超过6分钟
                        bst.node[n].step_end = 1;
                }
                else if(set_i >= 685)  //687
                {
                    if((bst.node[n].time - bst.node[n].time_tmp) > 56*60 ) //超过6分钟
                         bst.node[n].step_end = 1;
                }
                else if(set_i >= 343)  //343
                {
                    if((bst.node[n].time - bst.node[n].time_tmp) > 112*60 ) //超过6分钟
                         bst.node[n].step_end = 1;
                }
			}
			if(bst.node[n].step_end)
			{
                bst.node[n].node_status = 27; //电池电压异常
                copy_real_data(n);

                #if BAT_DEBUG_CONFIG > 0
                printf("charge_bat_err_poll\r\n");
                #endif
			}
		}
}

/******************************************************************
* 函数名: discharge_capacity_end_poll
* 功能:   工步终止容量查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void discharge_capacity_end_poll(u16 n)
{
	u16 cap_tmp;
	u8 now_step;
	u32 stop_cap;			//结束容量

    if(bst.node[n].step_end !=0)
    {
        return;
    }
	now_step = bst.node[n].now_step;
	stop_cap = bst.node[n].step[now_step].cond4;
	cap_tmp  = (u16)(bst.node[n].discapacity / 3600);
	if((cap_tmp >= stop_cap)&&(stop_cap != 0))
	{
	    //达到容量
        bst.node[n].step_end = 1;
        copy_real_data(n);
        bst.node[n].node_status= 23;  //达到终止容量
	}
}

/******************************************************************
* 函数名: discharge_vot_end_poll
* 功能:   放电中止电压查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void discharge_vot_end_poll(u16 n)
{
	u8 now_step;
	u16 stop_v;			//结束电压
    if(bst.node[n].step_end !=0)
    {
        return;
    }
 	now_step = bst.node[n].now_step;
	stop_v   = bst.node[n].step[now_step].parm2;    //放电停止电压即下限电压
	if((bst.node[n].v <= stop_v)&& (stop_v != 0))
	{
	    //判断是否达到终止电压
        bst.node[n].step_end    = 1;
        bst.node[n].node_status = 21;   //达到终止电压
        copy_real_data(n);

        #if BAT_DEBUG_CONFIG > 0
        printf("discharge_vot_end_poll end = %d\r\n", bst.node[n].v);
        #endif
	}else if(bst.node[n].v <= 1500 &&(stop_v == 0)){   //电压过小
	    #if BAT_DEBUG_CONFIG > 0
	    printf("discharge_vot_end_poll bst.node[n].v = %d n = %d\r\n", bst.node[n].v, n);
	    #endif

	    bst.node[n].step_end    = 1;
        bst.node[n].node_status = 30;//电池放电电压过低
        copy_real_data(n);
	}
}

/******************************************************************
* 函数名: discharge_curr_end_poll
* 功能:   放电终止电流查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void discharge_curr_end_poll(u16 n)
{
    u8 now_step;
	u16 stop_i;			//设定电流

    if(bst.node[n].step_end !=0)
    {
        return;
    }
	now_step = bst.node[n].now_step;
	stop_i   = (u16)bst.node[n].step[now_step].cond2;

	if((bst.node[n].i <= stop_i)&& (stop_i != 0) && bst.node[n].step_end == 0) //电流达到终止条件
	{

	    bst.node[n].step_end    = 1;
        bst.node[n].node_status = 22;   //达到终止电流
        copy_real_data(n);

        #if BAT_DEBUG_CONFIG > 0
        printf("discharge_curr_end_poll  n = %d\r\n", n);
        #endif
	}
}

/******************************************************************
* 函数名: discharge_curr_err_poll
* 功能:   放电电流异常查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void discharge_curr_err_poll(u16 n)
{

	u8 now_step;
	u16 set_i;			//设定电流

    if(bst.node[n].step_end !=0)
    {
        return;
    }
	now_step = bst.node[n].now_step;
	set_i    = (u16)bst.node[n].step[now_step].cond1;

    if(bst.node[n].step_end == 0)
	{

		if(((bst.node[n].i <= 200 || (bst.node[n].i > (set_i +100))))) //电池电压大于终止电压时 判断电流是否达到结束条件
		{
            bst.node[n].step_end    = 1;    //电流启动不来过小 或者 过大
            bst.node[n].node_status = 31;   // 放电电流异常 偏小或偏大
            copy_real_data(n);

            #if BAT_DEBUG_CONFIG > 0
            printf("discharge_curr_err_poll bst.node[n].i = %d n =%d\r\n", bst.node[n].i, n);
            #endif
		}
	}
}

/******************************************************************
* 函数名: discharge_status_err_poll
* 功能:   放电状态异常查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void discharge_status_err_poll(u16 n)
{
	u8 now_step;
    u16 set_v;
    u16 set_i;

    if(bst.node[n].step_end != 0)
    {
        return;
    }
	now_step = bst.node[n].now_step;
    set_v    = bst.node[n].step[now_step].parm2;        //放电设置电压为下限电压
    set_i    = (u16)bst.node[n].step[now_step].cond1;
    if((bst.node[n].v > (set_v + 20)) && \
        (bst.node[n].i < (set_i - 100)) && \
        (bst.node[n].step_end == 0))        //既不是恒压也不是恒流
	{
	    #if BAT_DEBUG_CONFIG > 0
	    printf("set_v = %d, set_i = %d\r\n", set_v, set_i);
	    printf("discharge_status_err_poll v = %d i = %d n = %d\r\n", bst.node[n].v, bst.node[n].i, n);
	    #endif

	    bst.node[n].step_end    = 1;
        bst.node[n].node_status = 32;  //非恒流放电停止
        copy_real_data(n);
	}
}

/******************************************************************
* 函数名: sys_protect_paramter_poll
* 功能:   保护电流电压查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void sys_protect_paramter_poll(u16 n)
{
    u16 cur_v,cur_i;

	cur_v = bst.node[n].v;		 //当前电压
	cur_i = bst.node[n].i;			//当前电流

	//printf("cur_v = %d cur_i = %d \r\n", cur_v, cur_i);
	//printf(" sys_cfg.protect_low_v = %d  sys_cfg.protect_high_v = %d sys_cfg.protect_low_i = %d  sys_cfg.protect_high_i = %d\r\n", \
    // sys_cfg.protect_low_v,  sys_cfg.protect_high_v,sys_cfg.protect_low_i,sys_cfg.protect_high_i);

    if(bst.node[n].step_end != 0)
    {
        return;
    }
	if(cur_v <= sys_cfg.protect_low_v || cur_v >= sys_cfg.protect_high_v ||\
		 cur_i <= sys_cfg.protect_low_i || cur_i >= sys_cfg.protect_high_i)
	{
		//电流或电压达到保护值
        bst.node[n].step_end    = 1;
        bst.node[n].node_status = 28;
        copy_real_data(n);
        printf("sys_protect_paramter_poll\r\n");
	}
}

/******************************************************************
* 函数名: charge_bat_vot_up_poll
* 功能:   充电时电压上升过慢 保护
* 参数：  n 节点
* 返回值:
******************************************************************/
static void charge_bat_vot_up_poll(u16 n)
{
	u16 start_v;
    u16 cur_v;

    if(bst.node[n].step_end != 0)
    {
        return;
    }
	start_v = bst.node[n].precharge_v;
	cur_v   =  bst.node[n].v;		 //当前电压值
	if(start_v < 2100 && cur_v >= start_v)
	{
		if((cur_v - start_v < 260))
		{
            bst.node[n].node_status = 33;
		}
	}
}

/******************************************************************
* 函数名: recharge_capacity_end_poll
* 功能:  返充到达截止容量查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void recharge_capacity_end_poll(u16 n)
{
	u16 cap_tmp;
	u8 now_step;
	u16 recharge_cap;			//设定容量

    if(bst.node[n].step_end != 0)
    {
        return;
    }
	if(bst.node[n].recharge_status != 0 && (bst.node[n].recharge_ratio > 0 && bst.node[n].recharge_ratio <= 100))  //返充标志位
	{
		now_step = bst.node[n].now_step;		//当前工步

		if(now_step >= (bst.node[n].recharge_start_step - 1))		//当前工步为 返充 工步
		{
			recharge_cap = (u16)(( bst.node[n].real_capacity/3600) * bst.node[n].recharge_ratio / 100);  //计算需要返充的容量
			cap_tmp      = (u16)(bst.node[n].capacity / 3600);		//实时容量

			if((cap_tmp >= recharge_cap)&& (recharge_cap != 0))
			{  //达到容量
                printf("n = %d, recharge_cap = %d\r\n", n, recharge_cap);
                bst.node[n].step_end    = 1;
                bst.node[n].node_status = 34;    //达到返充容量值
                copy_real_data(n);
			}
		}
	}
}

/******************************************************************
* 函数名: sys_run_status_poll
* 功能:  工作状态查询
* 参数：  n 节点
* 返回值:
******************************************************************/
static void sys_run_status_poll(u16 n)
{
	u8 temp;
	u8 now_step;
	u8 powerfault_type;
    u16 stop_v,set_i,set_v;
    u64 ind1,ind2;

    if(bst.node[n].step_end)
        return;

    now_step = bst.node[n].now_step;		//读取当前工步号
    temp     = bst.node[n].step[now_step].type;

    set_v  = bst.node[n].step[now_step].parm2; //放电判断时需要 因此此处的设置电压为下限电压
    set_i  = (u16)bst.node[n].step[now_step].cond1;

    stop_v = bst.node[n].step[now_step].parm2;      //终止电压 即放电时候的下限电压

    if(get_sys_running_time_seconds() > (bst.node[n].time_tmp_1 + 4))
    {
        bst.node[n].read_start_v_flag = 0;
    }
    switch (temp)
    {
        case CV :		//多条件终止充电
        case HCV:       //恒流充电
            if((bst.node[n].fwsta & 0x0012) == 0x0002) //节点处于充电模式且非恒压状态
            {
                bst.node[n].cc_capacity = bst.node[n].capacity;
                if(bst.node[n].node_status < 10 || bst.node[n].node_status >= 20)
                {
                    bst.node[n].node_status = 3;		  // 恒流充电
                }
            }
            else
            {
                if(temp == HCV && bst.node[n].v != 0 && bst.node[n].i != 0 && (bst.node[n].v > bst.node[n].step[now_step].parm1 -10) && \
                    (bst.node[n].i < bst.node[n].step[now_step].cond1) ){   //如果是恒流充电 当电压达到设定值 电流开始下降 则恒流充电停止
                        bst.node[n].step_end = 1;
                        copy_real_data(n);  //达到中止条件时保存实时数据
                        return;
                }
                if(bst.node[n].node_status < 10 || bst.node[n].node_status >= 20)
                {
                    bst.node[n].node_status = 2;		  // 恒压充电
                }
            }

            ind1 = bst.node[n].cc_capacity;
            ind2 = bst.node[n].capacity;

            if(ind2 != 0){
                bst.node[n].cc_cv = (u16)(ind1 * 1000 / ind2);			//计算恒流比
            }
            if(bst.node[n].cc_cv > 1000)	//判断恒流比的合法性
            {
                bst.node[n].cc_cv  = 0;
            }

            if((bst.node[n].step_end == 0) && (bst.node[n].info & 0x01) == 0) //屏蔽未用节点
            {
                bst.node[n].step_end    = 1;
                bst.node[n].node_status = 13; //节点禁用
                copy_real_data(n);
            }
 			bst.node[n].end_v = bst.node[n].v;
            bst.node[n].end_i = bst.node[n].i;
            bst.node[n].end_time = get_sys_running_time_seconds();

            step_time_end_poll(n);		//工步终止时间查询
           // if(bst.node[n].time < (bst.node[n].time_tmp_1 + 10))
            if((get_sys_running_time_seconds() - bst.node[n].time_tmp_1) < 10)
            {
                return;
            }

            if((bst.node[n].step_end == 0)&&(bst.node[n].info &0x88) != 0) //屏蔽错误节点
            {
                bst.node[n].step_end = 1;
                //节点尚未处于故障报警状态
                powerfault_type = (bst.node[n].fwsta >>9)&0x1F;		//提取故障类型
                if(powerfault_type == 0)  //DCDC板软件版本为: 1.01.04之前的软件版本号,固定为0
                {
                    bst.node[n].node_status = 10;   //老版本故障
                }
                else if(powerfault_type == 1)       //硬件故障
                {
                    bst.node[n].node_status = 12;
                }
                else if(powerfault_type == 2)       //采样故障
                {
                    bst.node[n].node_status = 11;
                }
            }

            if((bst.node[n].step_end == 0)&&\
                ((bst.node[n].info &(NODE_ERR_BIT | NODE_CMP_BIT | NODE_ENABLE_BIT)) == NODE_ENABLE_BIT))//无故障并达到条件并无标记
            {
                charge_vot_end_poll(n);		 //判断充电终止电压
                charge_curr_end_poll(n);	 //判断充电终止电流
                charge_curr_err_poll(n);
                charge_vot_err_poll(n);
                charge_status_err_poll(n);
                charge_bat_err_poll(n);
                charge_capacity_end_poll(n);
                sys_protect_paramter_poll(n);
                charge_bat_vot_up_poll(n);
                //返充终止容量查询
                recharge_capacity_end_poll(n);

            }
            //step_time_end_poll(n);		//工步终止时间查询
        break;

        case DCV :		//多条件终止放电

            if(bst.node[n].step_end == 0 && (bst.node[n].v > bst.node[n].platform_v))		//若判断该节点放电未完成,则更新平台电压时间
            {
                bst.node[n].platform_time = bst.node[n].time;
            }
            bst.node[n].end_v = bst.node[n].v;

            if(stop_v != 0)
            {   //由于恒压放电时 电流会下降, 所以为了不让过程数据采集到该过程,当判断节点为了恒压放电时则不更新结束电流变量
                if( bst.node[n].v < (set_v + 30)) //
                {
                    if( bst.node[n].i > (set_i - 100))
                    {
                        bst.node[n].end_i = bst.node[n].i;          //实时记录更新结束电流
                    }
                }
                else
                {
                    bst.node[n].end_i = bst.node[n].i;          //实时记录更新结束电流
                }
            }
            else
            {
                bst.node[n].end_i = bst.node[n].i;          //实时记录更新结束电流
            }

            bst.node[n].end_time = get_sys_running_time_seconds();  //

            step_time_end_poll(n);		//工步终止时间查询
            //if(bst.node[n].time < (bst.node[n].time_tmp_1 + 10))
            if((get_sys_running_time_seconds() - bst.node[n].time_tmp_1) < 10)
            {
                return;
            }

            if((bst.node[n].step_end == 0) && (bst.node[n].info & 0x88) != 0 )//屏蔽故障
            {   //节点尚未处于故障报警状态
                powerfault_type = (bst.node[n].fwsta >> 9 )&0x1F;		//提取故障类型
                if(powerfault_type == 0)  //DCDC板软件版本为: 1.01.04之前的软件版本号,固定为0
                {
                    bst.node[n].node_status = 10;
                }
                else if(powerfault_type == 1)
                {
                    bst.node[n].node_status = 12;
                }
                else if(powerfault_type == 2)
                {
                    bst.node[n].node_status = 11;
                }
            }

            if(bst.node[n].step_end == 0)//无故障并达到条件并无标记
            {
                discharge_vot_end_poll(n);
                discharge_curr_end_poll(n);
                discharge_curr_err_poll(n);
                discharge_status_err_poll(n);
                discharge_capacity_end_poll(n);
                sys_protect_paramter_poll(n);
            }
           // step_time_end_poll(n);		//工步终止时间查询
        break;

        case REST : 	 //搁置
            if(bst.node[n].node_status < 10 || bst.node[n].node_status >= 20)
            {
                bst.node[n].node_status = 1;   //状态表示为搁置
            }

            bst.node[n].end_v = bst.node[n].v;
            bst.node[n].end_i = bst.node[n].i;          //实时记录更新结束电流

            step_time_end_poll(n);//工步终止时间查询
        break;

        default:
        break;
    }
}

/******************************************************************
* 函数名: next_step_handle
* 功能:  下一个工步处理函数
* 参数：  n 节点
* 返回值:
******************************************************************/
static void next_step_handle(u16 n)
{
    u8 temp;
    u8 now_step;
    u8 now_cyc_tmp;
    u8 sub_now_cyc_tmp;
    u8 type;

    //先判断否存在子循环
    now_step = bst.node[n].now_step;         //当前工步
    type     = bst.node[n].step[now_step].type;

    bst.node[n].time_tmp_1 = get_sys_running_time_seconds();

    //如果分容 放电工步结束 则更新真实容量值
    if(type == DCV && bst.node[n].recharge_status == 0)
    {
         bst.node[n].real_capacity = bst.node[n].discapacity;        //更新真实容量,容易返充时计算返充容量
         printf("bst.node[n].real_capacity = %d\r\n", bst.node[n].real_capacity / 3600);
    }

    if( bst.node[n].step[now_step].jump_step != 0 && \
       (bst.node[n].step[now_step].sub_cyc != 0 ) && \
       ((bst.node[n].step[now_step].jump_step-1) < now_step))    //先判断是否 存在子循环
    {
        //判断子循环次数
        sub_now_cyc_tmp = bst.node[n].sub_now_cyc;
        if((++sub_now_cyc_tmp) > (bst.node[n].step[now_step].sub_cyc))
        {
            //子循环次数已达到 ,进入下一个工步
            now_step++;
            bst.node[n].sub_now_cyc = 0;
        }
        else
        {
            //子循环次数未达到
            now_step = bst.node[n].step[now_step].jump_step - 1;     //跳转工步
            bst.node[n].sub_now_cyc++;
        }
    }
    else
    {
        //不存在子循环则切换到下一个工步
        now_step++;
    }

    //判断当前工步是否为返充工步
    if((now_step < (bst.node[n].divcap_end_step-1)) && (bst.node[n].divcap_end_step != 0xff ) && (bst.node[n].divcap_end_step > 0))
    {
        bst.node[n].recharge_status = 0;  //分容
    }
    else if((now_step >= (bst.node[n].recharge_start_step-1)) && (bst.node[n].recharge_start_step != 0xff) && (bst.node[n].recharge_start_step >0))
    {
        bst.node[n].recharge_status = 1;  //返充
    }

    if(bst.node[n].step_num > now_step)
    {   //未完成
        bst.node[n].now_step = now_step;
        temp = bst.node[n].step[now_step].type;
    }
    else
    {
       now_cyc_tmp = bst.node[n].now_cyc;
       now_cyc_tmp++;
       if(now_cyc_tmp == bst.node[n].cyc || bst.node[n].step[now_step].type == 0xf0)
       {
            //printf("dowm====1 now_cyc_tmp = %x,  bst.node[n].cyc = %x, bst.node[n].step[now_step].type = %x\r\n", now_cyc_tmp,  bst.node[n].cyc, bst.node[n].step[now_step].type);
            temp = CEND;        //完成
       }
       else
       {
           // printf("cycle\r\n");
            now_step = 0;
            bst.node[n].now_cyc = now_cyc_tmp;
            bst.node[n].now_step = now_step;
            temp = bst.node[n].step[now_step].type;
       }
    }

    update_start_test_v_i(n);
    switch (temp)
    {
        case CV :           //恒流恒压充电
        case HCV:           //恒流充电
        {
            bst.node[n].cc_cv       = 100;
            bst.node[n].cc_capacity = 0;
            bst.node[n].capacity    = 0;
            bst.node[n].em          = 0;
            bst.node[n].time        = 0;

            if((bst.node[n].info & (NODE_ERR_BIT | NODE_LINKOFF_BIT)) != 0)//故障或者不允许
            {
                bst.node[n].work_type = REST;
            }
            else
            {
                if(bst.node[n].step[now_step].cond2 != 0){
                    bst.node[n].work_type = CV;
                }else{
                    bst.node[n].work_type = HCV;
                }
            }

            bst.node[n].node_status = 3; //恒流充电
            bst.node[n].set_v       = bst.node[n].step[now_step].parm1; //充电时 设置电压为上限电压
            bst.node[n].set_i       = (u16)bst.node[n].step[now_step].cond1;
            bst.node[n].precharge_v = bst.node[n].v;
        }
        break;

        case DCV :
        {
            bst.node[n].discapacity     = 0;
            bst.node[n].platform_time   = 0;
            bst.node[n].em              = 0;
            bst.node[n].time            = 0;
            if((bst.node[n].info & (NODE_ERR_BIT | NODE_LINKOFF_BIT)) != 0)//故障或者不允许
            {
                bst.node[n].work_type = REST;
            }
            else
            {
                 bst.node[n].work_type = DCV;
            }

            bst.node[n].node_status = 4;   //节点状态表示为恒流放电
            bst.node[n].set_v       = bst.node[n].step[now_step].parm2; //放电时 设置电压为下限电压
            bst.node[n].set_i       = (u16)bst.node[n].step[now_step].cond1;
        }
        break;

        case REST :
        {
            bst.node[n].em          = 0;
            bst.node[n].time        = 0;
            bst.node[n].work_type   = REST;
            bst.node[n].node_status = 1; //搁置
            bst.node[n].set_v       = 0;
            bst.node[n].set_i       = 0;
        }
        break;

        case CYCL :
        {

        } break;

        case CEND :
        {
            bst.node[n].work_type   = REST;
            bst.node[n].run_status  = SYS_COMP;
            bst.node[n].run_status  &=~NODE_RUN_BIT;
            bst.node[n].run_status  |= NODE_CMP_BIT;
            save_real_data[n].working_status = 0xA0;	//工步完成

            open_sqlite_db();
            start_commit();
            save_global_group_data(n);
            stop_commit();
            close_sqlite_db();

        } break;

        default:
            break;
    }
}

/*********************统计组内所有模块的电荷储集**************/
/**************************************************
* name     : battery_capacity_mAS
* funtion  : 统计每一组到电池容量
* parameter：n-电池编号,deta_t-时间差值,单位为:10ms
* return   : NULL
***************************************************/
static void battery_capacity_mAS(u16 n,u16 deta_t) //段地址//dir=1 is charge;dir=2 is discharge;dir=3 is reset
{
	u32 temp_c;
    u8 now_step;
	u8 step_type;

    if(bst.node[n].step_end == 0){
        now_step = bst.node[n].now_step;					//当前工步
        step_type = bst.node[n].step[now_step].type; //工步类型
        temp_c = (u32)(bst.node[n].i * deta_t * SCAN_CLK);  //Curr_uint  电流单位

        //计算能量
        bst.node[n].em += bst.node[n].v * bst.node[n].i * deta_t * SCAN_CLK / 1000;       //能量mw*S
        if(step_type == CV || step_type == HCV)		//充电
        {
            bst.node[n].capacity += temp_c;  //capacity = mA*S
        }
        else if(step_type == DCV)		//放电
        {
            bst.node[n].discapacity += temp_c;
        }
    }
}

//定时计算容量
static void timer_cal_tick(void)
{
    static  long        tv_ms,last_ms,count = 0;
    struct  timespec    time;
            long        tv_ms_temp  = 0;
            long        tv_second,deta_t;
            u16         n;

    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    tv_ms_temp = time.tv_nsec/1000000;
    tv_second  = time.tv_sec;

    if(tv_ms != tv_ms_temp)
    {
        tv_ms = tv_ms_temp;  //更新ms

        if(tv_ms % 10 == 0)  //10ms更新一次
        {
            if(++count>(SCAN_CYCLE-1))
            {
                count = 0;
                if(tv_ms>last_ms)
                {
                    deta_t = abs(tv_ms-last_ms)/10;
                }
                else
                {
                     deta_t = abs((tv_ms+1000)-last_ms)/10;
                }

               // printf("deta_t = %d\n",deta_t);
                for(n = 0;n < MAX_NODE;n++)
                 {
                      if((bst.node[n].run_status == SYS_RUNNING))
                      {//计算容量
                      //  printf("battery_mAS = %d,tv_second = %d,seconds = %d\n",tv_ms,tv_second,sys_running_time_seconds);
                        battery_capacity_mAS(n,deta_t);
                        if(sys_running_time_seconds != tv_second)
                        {
                            if((bst.node[n].step_end == 0)){
                                bst.node[n].time++;         //更新时间
                            }
                        }
                      }
                 }
                last_ms = tv_ms;
                sys_running_time_seconds = tv_second;
            }
        }
    }
}

/******************************************************************
* 函数名: sigalrm_fn
* 功能:  定时器信号响应
* 参数： sig 传入参数
* 返回值:
******************************************************************/
void sigalrm_fn(int sig)
{
    time_cnt++;
    if(time_cnt > 61201){
        time_cnt = 0;
    }
    if(time_cnt != 0 && time_cnt % SAVE_GLOBAL_PAR == 0){
        sem_post(&save_global_par_sem);
    }
    if(time_cnt != 0 && time_cnt % SAVE_RUNNING_DATA == 0){
        sem_post(&save_running_data_sem);
    }
    if(time_cnt != 0 && time_cnt % SAVE_HISTORY_DATA == 0){
        sem_post(&save_history_data_sem);
    }

  /*
    if(time_cnt != 0 && time_cnt % DELETE_CASH == 0){
        sem_post(&delete_cash_sem);
    }*/

    alarm(1);
    return;
}

/******************************************************************
* 函数名: sys_time_thread_task
* 功能:  定时器响应线程
* 参数：
* 返回值:
******************************************************************/
void sys_time_thread_task(void)
{
    signal(SIGALRM, sigalrm_fn);
    alarm(1);

    while(1){
        usleep(1*1000);
        timer_cal_tick();
    }

}

/******************************************************************
* 函数名: copy_netleft_data
* 功能:  拷贝离线数据
* 参数：
* 返回值:
******************************************************************/
void copy_netleft_data()
{
    u16 i, j;

    for(i = 0; i < MAX_NODE; i++){
        for(j = 0; j < 10; j++){
            save_history_data[i][j].mun = i + 1;
            save_history_data[i][j].sub_now_cyc = bst.node[i].sub_now_cyc;    //子循环次数
            save_history_data[i][j].now_step = bst.node[i].now_step;    //工步号
            save_history_data[i][j].work_type = bst.node[i].work_type;   //工步类型
            save_history_data[i][j].node_status = bst.node[i].node_status; //节点状态
            save_history_data[i][j].recharge_status = bst.node[i].recharge_status; //返充标志
            save_history_data[i][j].recharge_ratio = bst.node[i].recharge_ratio;  //返充比例

            save_history_data[i][j].cyc = bst.node[i].cyc;    //主循环次数
            save_history_data[i][j].v = bst.node[i].v;  //电压
            save_history_data[i][j].cc_cv = bst.node[i].cc_cv;  //恒流比
            save_history_data[i][j].platform_time = bst.node[i].platform_time;  //平台时间

            if(save_history_data[i][j].work_type == CV || save_history_data[i][j].work_type == HCV){
                save_history_data[i][j].charge = bst.node[i].capacity / 3600;
            }else if(save_history_data[i][j].work_type == DCV){
                save_history_data[i][j].charge  = bst.node[i].discapacity / 3600;
            }else{
                save_history_data[i][j].charge = 0;
            }

            save_history_data[i][j].i = bst.node[i].i;  //电流
            save_history_data[i][j].em = bst.node[i].em; //能量
            save_history_data[i][j].time = bst.node[i].time;   //运行时间
        }
    }
}

/************************************************
*  name      :  save_data_thread_task
*  function  :
*  parameter :  void
*  return    :  void
*************************************************/
void save_data_thread_task(void)
{
    u16 i, j;

    sem_init(&save_global_par_sem, 0,0);
    sem_init(&save_global_par_sem_1, 0, 0);
    sem_init(&save_running_data_sem, 0,0);
    sem_init(&save_history_data_sem, 0,0);
    sem_init(&delete_data_sem, 0, 0);
  // sem_init(&delete_cash_sem, 0, 0);

    while(1){

        copy_netleft_data();

        if(setsem_wait(&save_step_sem, 100*1000000, 0) == 0){
            open_sqlite_db();
            start_commit();
            for(j = 0; j < MAX_NODE; j++){
                save_step_mun(j, bst.node[j].step_num);
            }
            stop_commit();
            close_sqlite_db();
        }

        if(setsem_wait(&save_global_par_sem, 100*1000000, 0) == 0){
            open_sqlite_db();
            start_commit();
            for(j = 0; j < MAX_NODE; j++){
                if(bst.node[j].run_status == SYS_RUNNING && bst.node[j].step_end != 1){
                    copy_real_data(j);
                    save_global_group_data(j);
                }
            }
            stop_commit();
            close_sqlite_db();
        }

        if(setsem_wait(&save_running_data_sem, 100*1000000, 0) == 0){
            open_sqlite_db();
            start_commit();
            for(j = 0; j < MAX_NODE; j++){
                if((bst.node[j].run_status == SYS_RUNNING && bst.node[j].step_end != 1)){
                    copy_real_data(j);
                    insert_running_data(j);
                }
            }
            stop_commit();
            close_sqlite_db();
        }

        if(setsem_wait(&save_history_data_sem, 100*1000000, 0) == 0){
            cat_mmp();
            if(bst.net_status == 0x02){
                open_sqlite_db();
                net_off_save_data();
                stop_commit();
                getNowTime();
                close_sqlite_db();
            }
        }

        if(setsem_wait(&delete_data_sem, 100*1000000, 0) == 0){
            open_sqlite_db();
            start_commit();
            getNowTime();
            for(j = 0; j < MAX_NODE; j++){
               delete_bat_runningdata(j);
            }
            stop_commit();
            Vacuum_data();  //清sqlite内存
            getNowTime();
            close_sqlite_db();
        }
    }
}

/******************************************************************
* 函数名: step_init
* 功能:  调试时初始化工步
* 参数： n 通道
* 返回值:
******************************************************************/
void step_init(u16 n)
{
    int i;

    bst.node[n].step_num = 2;
    bst.node[n].cyc = 1;
    for(i = 0; i < bst.node[n].step_num; i++){
        bst.node[n].step[i].type  = REST;
        bst.node[n].step[i].parm1 = 0;
        bst.node[n].step[i].parm2 = 0;
        bst.node[n].step[i].cond1 = 0;
        bst.node[n].step[i].cond2 = 0;
        bst.node[n].step[i].cond3 = 10; //搁置30秒
        bst.node[n].step[i].cond4 = 0;
        bst.node[n].step[i].jump_step = 0;
        bst.node[n].step[i].sub_cyc = 0;

        if(n==0 && i ==0)
        {
            bst.node[n].step[i].type  = CV;
            bst.node[n].step[i].parm1 = 3500;
            bst.node[n].step[i].parm2 = 0;
            bst.node[n].step[i].cond1 = 5500;
            bst.node[n].step[i].cond2 = 220;
            bst.node[n].step[i].cond3 = 30; //搁置30秒
            bst.node[n].step[i].cond4 = 20;
            bst.node[n].step[i].jump_step = 30;
            bst.node[n].step[i].sub_cyc = 400;
        }
    }
}

/**************************************************
* name     : sys_logic_control_pthread_task
* funtion  :  逻辑控制线程
* parameter：创建线程时需要传递到参数
* return   : NULL
***************************************************/
void sys_logic_control_pthread_task(void)
{
    u16 n;

#if 1
	for(n = 0; n < MAX_NODE; n++)
	{
	    //刚上电先所有节点都发送停止命令
        bst.node[n].work_type = REST;
        bst.node[n].read_start_v_flag = 0;
	}
	sleep(3);		//必须延时一段事件之后才能继续启动发送事件数据
	for(n = 0; n < MAX_NODE; n++)
	{
        //bst.node[n].run_status = SYS_START;
	}
#endif

/*
    for(n = 0; n < MAX_NODE; n++){
         node_start_init(n);      //节点启动初始化
    }*/

	while(1)
	{
		usleep(50*1000);
        for(n = 0; n < MAX_NODE; n++)
		{
            node_start(n);                                  //扫描节点启动
			if(bst.node[n].run_status == SYS_RUNNING)		//系统运行中
			{
                //io_leds_run_on();                           //运行指示灯
				sys_run_status_poll(n);
			}

			if(bst.node[n].run_status == SYS_STOP)		//系统需要停止  未测试完关闭
			{
			    io_leds_run_off();
			    bst.node[n].work_type            = REST;       //主动停止时需要将模式工作类型设置为搁置停止
				bst.node[n].info                 &=~NODE_RUN_BIT;
				bst.node[n].run_status           = SYS_IDLE;         //停止之后,将标志设置为空闲
				save_real_data[n].working_status = 0xA1;
			}
			if(bst.node[n].run_status == SYS_PAUSE)  //手动暂停
			{
                io_leds_run_off();
			    bst.node[n].work_type            = REST;       //主动停止时需要将模式工作类型设置为搁置停止
				bst.node[n].info                 &= ~NODE_RUN_BIT;
				save_real_data[n].working_status = 0xA1;
			}

			if(bst.node[n].run_status == SYS_COMP){
                //save_global_group_data(j);
			}
		}

        for(n = 0; n < MAX_NODE; n++)
        {
             //下一个工步 充电 放电完成时延时4秒钟在切换到下一个工步,以便上位机能够读取最后结束时到电压电流等参数
            if((bst.node[n].step_end == 1) && (bst.node[n].run_status == SYS_RUNNING) && ((bst.node[n].end_time + 4) < get_sys_running_time_seconds()))
            {   //
                next_step_handle(n);
                if(bst.node[n].run_status == SYS_RUNNING){
                    bst.node[n].step_end = 0;
                }
            }
        }

		//timer_cal_tick();
	}
}

/**************************************************
* name     : update_group_info_to_save
* funtion  : 更新组的运行数据信息
* parameter：group 相应的组
* return   :
***************************************************/
void update_group_info_to_save(u8 group)
{
/*
	u8 module,module2,ch;

	for(module =group*GROUP_MODULE_NUM; module < (group+1)*GROUP_MODULE_NUM; module++)
	{
		module2 = module-group*GROUP_MODULE_NUM;
		for(ch = 0; ch < MODULE_CH_NUM; ch++)
		{
            Global_info[group].module_status[module2].capacity[ch]                  =  N.NPS_info[module].capacity[ch];
            Global_info[group].module_status[module2].discapacity[ch]               =  N.NPS_info[module].discapacity[ch];
            Global_info[group].module_status[module2].i[ch] 		                =  N.NPS_info[module].i[ch];
            Global_info[group].module_status[module2].v[ch] 					    =  N.NPS_info[module].v[ch];
            Global_info[group].module_status[module2].Btime[ch] 			        =  N.NPS_info[module].Btime[ch];
            Global_info[group].module_status[module2].tCC[ch]                       =  N.NPS_info[module].tCC[ch];
            Global_info[group].module_status[module2].node_info[ch]                 =  N.NPS_info[module].node_info[ch];
            Global_info[group].module_status[module2].node_status[ch]               =  N.NPS_info[module].node_status[ch];	//节点状态
            Global_info[group].module_status[module2].FWsta[ch]                     =  N.NPS_info[module].FWsta[ch];
            Global_info[group].module_status[module2].DisCharge_Platform_Time[ch]   =  N.NPS_info[module].DisCharge_Platform_Time[ch];
	  }
	}
*/
}

/**************************************************
* name     : copy_real_data
* funtion  : 拷贝实时数据
* parameter：
* return   :
***************************************************/
void copy_real_data(u8 group)
{
    save_real_data[group].cyc           = bst.node[group].cyc;
    save_real_data[group].cyc_son       = bst.node[group].sub_now_cyc;
    save_real_data[group].step_mun      = bst.node[group].now_step;
    save_real_data[group].step_style    = bst.node[group].work_type;
    save_real_data[group].node_status   = bst.node[group].node_status;
    save_real_data[group].r_flag        = 0x01;
    save_real_data[group].r_rio         = 0x01;

//
    save_real_data[group].step_sum      = bst.node[group].step_num;
    save_real_data[group].now_cyc       = bst.node[group].now_cyc;
    save_real_data[group].info          = bst.node[group].info;
    save_real_data[group].run_status    = bst.node[group].run_status;
    save_real_data[group].step_end      = bst.node[group].step_end;

    save_real_data[group].i             = bst.node[group].i;
    save_real_data[group].v             = bst.node[group].v;
    save_real_data[group].cc_cv         = bst.node[group].cc_cv;
    save_real_data[group].run_time      = bst.node[group].time;
    save_real_data[group].plat_time     = bst.node[group].platform_time;

//
    save_real_data[group].platform_v    = bst.node[group].platform_v;

    save_real_data[group].em            = bst.node[group].em / 3600;
    save_real_data[group].capacity      = bst.node[group].capacity / 3600;
    save_real_data[group].discapacity   = bst.node[group].discapacity / 3600;
    if(save_real_data[group].step_style == CV || save_real_data[group].step_style == HCV){
        save_real_data[group].charge  = bst.node[group].capacity / 3600;
    }else if(save_real_data[group].step_style == DCV){
        save_real_data[group].charge  = bst.node[group].discapacity / 3600;
    }else{
        save_real_data[group].charge = 0;
    }

//
    save_real_data[group].cc_capacity   = bst.node[group].cc_capacity / 3600;
    save_real_data[group].real_capacity = bst.node[group].real_capacity / 3600;
}




/**************************************************
* name     : write_work_global_info
* funtion  : 保存工步执行的数据信息
* parameter：group 相应的组
* return   : NULL
***************************************************/
void  write_work_global_info(u8 group)
{
    insert_running_data(group);
    save_global_group_data(group);    //保存工步组的状态信息
}

/********************end file*********************/

