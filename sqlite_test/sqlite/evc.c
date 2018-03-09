/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：evc.c
*文件功能描述: lcd通信应用程序
*创建标识   ：xuchangwei 20170819
//
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <getopt.h>
#include <semaphore.h>  //信号量
#include "sys_sem.h"
#include "sys_cfg.h"
#include "sys_logic_control.h"
#include "evc.h"
#include "can.h"
#include "debug.h"

static  sem_t       can_rx_sem;                 //can 接收信号量
        u8          div_cap_led[MAX_NODE][4];	//分容时led灯指示
struct  can_frame   CAN_RxMessage;              //can 接收
struct  can_frame   read_data;

static  pthread_t   can_rev_thread;             //线程标识
static  u8          remeder[MODULE_NUM][8];     //临时变量 用于对比
static  u8          remeder2[MODULE_NUM][8];

extern u8 dgus_get_current_page_id(void);

/**************************************************
* name      :   can_rev_pthread_task
* funtion   :   can  接收线程
* parameter :   void
* return    :   void
***************************************************/
void can_rev_pthread_task(void)
{
    u8 nbytes;
   u8 send_node_id,rev_node_id;

    while(1)
    {
        nbytes = read(can_fd, &CAN_RxMessage, sizeof(CAN_RxMessage));

        if(nbytes > 0)
        {
            send_node_id = (read_data.can_id >> 8) & 0xff;
            rev_node_id = CAN_RxMessage.can_id  & 0xff;;

            if(((read_data.can_id >> 16) == (CAN_RxMessage.can_id >>16)) && (send_node_id == rev_node_id))
            {
                 sem_post(&can_rx_sem);
            }
            //getNowTime();
        }
        usleep(1000);
    }
}

/**************************************************
* name      :   can_communit_send
* funtion   :   can 发送数据
* parameter :   struct can_frame  *frame 一帧数据的格式内容
* return    :   成功返回1 失败返回0
***************************************************/
u8 can_communit_send(struct can_frame  *frame)
{
    u8 nbytes;
    u8 re_send_cnt = 1;
    u8 send_node_id,rev_node_id;

    do{
        frame->can_id = frame->can_id | 0x80000000;
        nbytes = write(can_fd,frame, sizeof(struct can_frame));
        if (nbytes == sizeof(struct can_frame))  //
        {
            if(setsem_wait(&can_rx_sem, 50*1000000,0) == 0)
            {
                #if NET_DEBUG_CONFIG > 2
                printf("can_rx_sem rcv!\r\n");
                #endif

                send_node_id = (frame->can_id >> 8) & 0xff;
                rev_node_id = CAN_RxMessage.can_id  & 0xff;;

                if(((frame->can_id >> 16) == (CAN_RxMessage.can_id >>16)) && (send_node_id == rev_node_id))
                {
                    #if NET_DEBUG_CONFIG > 2
                    printf("can rev ok! %d \n", re_send_cnt);
                    #endif
                    return 1;
                }
            }else{
              //  sem_init(&can_rx_sem,0, 0);
            }
            //usleep(5 * 1000);
        }
    }while(re_send_cnt--);

    //发送失败
    return 0;
}

/**************************************************
* name      :   update_divide_capacity_led
* funtion   :   更新分容时LED的状态
* parameter :   u8 *buff 数据内容 , u16 len 长度
* return    :   void
***************************************************/
void update_divide_capacity_led(u8 *buff,u16 len)
{
    u8 module;

	module = 0;

	for(module = 0; module < MAX_NODE; module++)
	{
		div_cap_led[module][0] = buff[0 + module * 4];
		div_cap_led[module][1]=  buff[1 + module * 4];
		div_cap_led[module][2] = buff[2 + module * 4];
		div_cap_led[module][3]=  buff[3 + module * 4];
	}
}

/**************************************************
* name      :   read_all_battery_v
* funtion   :   读取所有节点的电压
* parameter :   void
* return    :   成功返回1
***************************************************/
u8 read_all_battery_v(void)
{
  //  struct  can_frame   read_data;
            u8          *buf = NULL;
            u8          ret, module, cnt;
            u16         v[4];

    read_data.can_id    = 0x000000000;
	read_data.can_dlc   = 8;

	for(module = 0; module < MODULE_NUM; module++)
	{
		read_data.can_id = 0x034100f4 | ((module+1) << 8); //+node IDE
		memset(read_data.data,0,8);
		ret = can_communit_send(&read_data);
		buf = (u8*)CAN_RxMessage.data;

		if(ret)
		{
            bst.module_info[module].link_status  = MODULE_LINK_ON;
            bst.module_info[module].link_off_cnt = 0;
            for(cnt = 0; cnt < MODULE_CH_NUM; cnt++)
            {
                v[cnt] = (u16)(buf[cnt * 2 + 1] + (u16)(buf[cnt * 2] << 8));
                if(v[cnt] > 0x8000 || v[cnt] < 200)
                {
                    v[cnt] = 0;
                }
                bst.node[module*MODULE_CH_NUM+cnt].v = v[cnt];
            }
		}
		else
		{
			if(bst.module_info[module].link_status == MODULE_LINK_OFF)	//掉线
			{
                for(cnt = 0; cnt < MODULE_CH_NUM; cnt++)
                {
                    bst.node[module * MODULE_CH_NUM + cnt].v = 0;
                }
			}
			else
			{
				if( bst.module_info[module].link_off_cnt > 10)
				{
					 bst.module_info[module].link_status = MODULE_LINK_OFF;

					 #if NET_DEBUG_CONFIG > 0
					 printf("module = %d MODULE_LINK_OFF\r\n",(module+1));
					 #endif
				}
				else
				{
					 bst.module_info[module].link_off_cnt++;
				}
			}
		}
	}

	return 1;
}

/**************************************************
* name      :   read_all_battery_i
* funtion   :   读取所有节点的电流
* parameter :   void
* return    :   成功返回1
***************************************************/
u8 read_all_battery_i(void)
{
  //  struct  can_frame   read_data;
            u8          *buf;
            u8          ret, module, cnt;
            u16         i[4];

    read_data.can_id    = 0x000000000;
	read_data.can_dlc   = 8;

	for(module = 0; module < MODULE_NUM; module++)
	{
		if(bst.module_info[module].link_status == MODULE_LINK_OFF)	//掉线
		{
            for(cnt = 0; cnt < MODULE_CH_NUM; cnt++)
            {
                bst.node[module * MODULE_CH_NUM + cnt].i = 0;
            }
            continue;
		}

		read_data.can_id = 0x000000F4 | (RNC << 16) | ((module + 1) << 8); //+node IDE
		ret = can_communit_send(&read_data);
		if(ret)
		{
			buf = (u8*)CAN_RxMessage.data;
            for(cnt = 0; cnt < MODULE_CH_NUM; cnt++)   //
            {
                i[cnt] = (u16)(buf[cnt * 2 + 1] + (u16)(buf[cnt *  2] << 8));
                if(i[cnt] < 50)
                {
                    i[cnt] = 0;
                }
                bst.node[module * MODULE_CH_NUM + cnt].i = i[cnt];
            }
        }
	}

	return 1;
}

/**************************************************
* name      :   read_module_version
* funtion   :   读取软件版本信息
* parameter :   u8 module 读取的模块编码, u8 *buf 返回的模块版本信息
* return    :   成功返回 1 失败返回 0
***************************************************/
u8 read_module_version(u8 module,u8 *buf)
{
  //  struct  can_frame   read_data;
            u8          ret;

    read_data.can_id   = 0x000000000;
	read_data.can_dlc  = 8;
	read_data.can_id   = 0x000000F4 | (READ_GET_VERSION << 16) | (module << 8);
	ret                = can_communit_send(&read_data);

	if(ret)
	{
		memcpy(buf, (u8*)CAN_RxMessage.data, 8);
		return 1;
	}
	return 0;
}

/**************************************************
* name      :   comper
* funtion   :   can  接收线程
* parameter :   u8 *buff 比较的内容, u8 module 比较的位置
* return    :   相同返回 0 不同返回 1
***************************************************/
u8 comper(u8 *buff, u8 module)
{
	if(memcmp(&remeder[module][0],buff,8) == 0)
	{
		return 0;
	}
	return 1;
}

/**************************************************
* name      :   comper2
* funtion   :   比较停止启动标志位
* parameter :   u8 *buff 比较的内容, u8 module 比较的位置
* return    :   相同返回 0 不同返回 1
***************************************************/
u8 comper2(u8 *buff, u8 module)
{
	if(memcmp(&remeder2[module][0],buff,8) == 0)
	{
		return 0;
	}
	return 1;
}

/**************************************************
* name      :   dcdc_ctr_init
* funtion   :   临时保存的对比数组初始化
* parameter :   void
* return    :   void
***************************************************/
void dcdc_ctr_init(void)
{
    memset(&remeder[0][0],  0xff, sizeof(remeder[0]) * MODULE_NUM);
    memset(&remeder2[0][0], 0xff, sizeof(remeder2[0]) * MODULE_NUM);
}

/**************************************************
* name      :   module_single_ctr
* funtion   :   单独控制模块启动
* parameter :   u8 module 模块编号, u8 *buff
                模块启动停止的数据位 , u16 cmd CAN命令
* return    :   发送结果
***************************************************/
u8 module_single_ctr(u8 module, u8 *buff,u16 cmd)
{
   // struct  can_frame   read_data;
            u8          ret;

    read_data.can_id  = 0x000000000;
	read_data.can_dlc = 8;
	read_data.can_id  = 0x000000F4 | (cmd<<16) | (module<<8);

	memcpy(read_data.data,buff,8);

	ret = can_communit_send(&read_data);

	return ret;
}

/**************************************************
* name      :   module_single_read
* funtion   :   模块单独读取命令
* parameter :   u8 module  模块编号, u8 *buff
                返回的数据起始指针 u16 cmd  读取命令
* return    :   1 成功, 0失败
***************************************************/
u8 module_single_read(u8 module, u8 *buff,u16 cmd)
{
  //  struct  can_frame   read_data;
            u8          ret;

    read_data.can_id  = 0x000000000;
	read_data.can_dlc = 8;
	read_data.can_id  = 0x000000F4 | (cmd << 16) | (module << 8);

	memset(read_data.data,0,8);

	ret = can_communit_send(&read_data);

	if(ret)
	{
		memcpy(buff,read_data.data,8);
		return 1;
	}
	return 0;
}

/**************************************************
* name      :   node_led_run_poll
* funtion   :   运行过程查询节点LED灯的控制
* parameter :   void
* return    :   void
***************************************************/
void node_led_run_poll(void)
{
	u8 cnt, module, ret;
	u8 led_status[8] = {0};

	for(module = 0; module < MODULE_NUM; module++)		//各个模块扫描
	{
        for(cnt = 0; cnt < MODULE_CH_NUM; cnt++)
        {
            if(bst.node[module * MODULE_CH_NUM + cnt].run_status == SYS_IDLE ||\
               bst.node[module * MODULE_CH_NUM + cnt].run_status == SYS_PAUSE)
            {   //空闲状态 暂停
                if((bst.node[module * MODULE_CH_NUM + cnt].v <= sys_cfg.high_volt)&&\
                   (bst.node[module * MODULE_CH_NUM + cnt].v >= sys_cfg.low_volt))
                {
                    if(sys_cfg.led_lighting_mode)
                        led_status[8 - (cnt * 2 + 1)] = 1;
                    else
                        led_status[8 - (cnt * 2 + 1)] = 0;
                }
                else
                {
                    if(sys_cfg.led_lighting_mode == 0)
                        led_status[8 - (cnt * 2 + 1)] = 1;
                    else
                        led_status[8 - (cnt * 2 + 1)] = 0;
                }
            }
            else if(bst.node[module * MODULE_CH_NUM + cnt].run_status == SYS_RUNNING)		//运行中
            {
                if(bst.node[module * MODULE_CH_NUM + cnt].time < 3)  //
                {
                    led_status[8 - (cnt * 2 + 1)] = 1;
                }
                else
                {
                    led_status[8 - (cnt * 2 + 1)] = 0;
                }
            }
            else if(bst.node[module * MODULE_CH_NUM + cnt].run_status == SYS_DIVCAP)  //分容中
            {
                led_status[8 - (cnt * 2 + 1)] = bst.node[module * MODULE_CH_NUM + cnt].div_cap_led;  //分容
               // bst.node[module * MODULE_CH_NUM + cnt].run_status = SYS_IDLE;
            }
            else if(bst.node[module * MODULE_CH_NUM + cnt].run_status == SYS_COMP)  //完成
            {
                led_status[8 - (cnt * 2 + 1)] = 1;  //
            }
        }

        //发送点灯数据
        if(comper(led_status, module))
        {
            if(bst.module_info[module].link_status == MODULE_LINK_ON)
            {
                ret = module_single_ctr(module + 1, led_status, CLIG);
                if(ret)
                {
                    memcpy(&remeder[module][0], led_status, 8);
                }
            }
        }
	}
}

/**************************************************
* name      :   node_finish_status_poll
* funtion   :   查询节点停止完成状态,然后将其停止
* parameter :   void
* return    :   void
***************************************************/
void node_finish_status_poll(void)
{
    u8 i;
	u8 module, cnt, ret;
	u8 on_off[8] = {0};
    u8 set_v[8];        //设置电压
    u8 set_i[8];        //设置电流

	for(module = 0; module < MODULE_NUM; module++)		//各个模块扫描
	{
        for(cnt = 0; cnt < MODULE_CH_NUM; cnt++)
        {

            if(bst.node[module * MODULE_CH_NUM + cnt].work_type == REST || bst.node[module * MODULE_CH_NUM + cnt].work_type == CEND)//&& bst.node[module * MODULE_CH_NUM + cnt].step_end == 1)   //搁置 停止
            {   // 0 1 2 3
                // 1 3 5 7
                //printf("STOP============%d\r\n", (module * MODULE_CH_NUM + cnt));
                on_off[cnt*2+1] = STOP;   //搁置
                set_v [cnt*2  ] = 0;
                set_v [cnt*2+1] = 0;
                set_i [cnt*2  ] = 0;
                set_i [cnt*2+1] = 0;
            }
            else if(bst.node[module * MODULE_CH_NUM + cnt].work_type == CV || bst.node[module * MODULE_CH_NUM + cnt].work_type == HCV)   //充电
            {
               // printf("CHARGE============%d\r\n", (module * MODULE_CH_NUM + cnt));
                on_off[cnt*2+1]  = CHARGE;
                set_v [cnt*2]    = (u8)(bst.node[module * MODULE_CH_NUM + cnt].set_v >> 8);
                set_v [cnt*2+1]  = (u8)(bst.node[module * MODULE_CH_NUM + cnt].set_v & 0x00ff);
                set_i [cnt*2]    = (u8)(bst.node[module * MODULE_CH_NUM + cnt].set_i >> 8);
                set_i [cnt*2+1]  = (u8)(bst.node[module * MODULE_CH_NUM + cnt].set_i & 0x00ff);
            }
            else if(bst.node[module * MODULE_CH_NUM + cnt].work_type == DCV)   //放电
            {
               // printf("DISCHARGE============%d\r\n", (module * MODULE_CH_NUM + cnt));
                on_off[cnt*2+1] = DISCHARGE;
                set_v[cnt*2]    = (u8)(bst.node[module * MODULE_CH_NUM + cnt].set_v >> 8);
                set_v[cnt*2+1]  = (u8)(bst.node[module * MODULE_CH_NUM + cnt].set_v & 0x00ff);
                set_i[cnt*2]    = (u8)(bst.node[module * MODULE_CH_NUM + cnt].set_i >> 8);
                set_i[cnt*2+1]  = (u8)(bst.node[module * MODULE_CH_NUM + cnt].set_i & 0x00ff);
            }
        }

        if(comper2(on_off,module))
        {
            if(bst.module_info[module].link_status == MODULE_LINK_ON)
            {
                module_single_ctr(module + 1, &set_i[0], SETC);     //设置电流
                module_single_ctr(module + 1, &set_v[0], SETV);     //设置电压
                ret = module_single_ctr(module + 1, on_off, C_S_S);
                if(ret)
                {
                    memcpy(&remeder2[module][0], on_off, 8);
                }
            }
        }
	}
}

/**************************************************
* name      :   read_all_module_status
* funtion   :   读取模块状态
* parameter :   void
* return    :   void
***************************************************/
void read_all_module_status(void)
{
     //module     = 0,
	static  u8  status_cnt = 0, \
                ret        = 0, \
                cnt        = 0;
            u8  *buf, module_idle_status;
            u16 temp, module;

  //  struct can_frame read_data;

	if(++status_cnt >= 5)  //5s 确认一次节点故障状态
	{
		status_cnt          = 0;
        read_data.can_id    = 0x000000000;
        read_data.can_dlc   = 8;

        for(module = 0; module < MODULE_NUM; module++)
        {
            module_idle_status = 0;

             for(cnt = 0; cnt <  MODULE_CH_NUM; cnt++)
             {
                if(bst.node[module * MODULE_CH_NUM + cnt].run_status == SYS_RUNNING)
                {
                    module_idle_status = 1;
                    break;
                }
             }
            if(bst.module_info[module].link_status == MODULE_LINK_OFF || (module_idle_status == 0))
            { //掉线或者 处于停止状态 则跳过查询
                continue;
            }

            read_data.can_id = 0x000000F4 | (RSTC << 16) | ((module + 1) << 8);
            if(module < MAX_NODE)
            {
                ret = can_communit_send(&read_data);
                buf = (u8*)CAN_RxMessage.data;
            }
            if(ret)
            {
                for(cnt = 0; cnt <  MODULE_CH_NUM; cnt++)
                {
                    temp = (u16)(buf[cnt * 2 + 1] + (buf[cnt * 2] << 8));
                    bst.node[module * MODULE_CH_NUM + cnt].fwsta = temp;     //节点状态
                    if((temp & 0x1c0) > 0)  //无严重故障，极性，电源故障，电池欠压
                    {
                        if(bst.node[module * MODULE_CH_NUM + cnt].err_cnt >= 1)
                        {
                            bst.node[module * MODULE_CH_NUM + cnt].info |= NODE_ERR_BIT;  //标记错误
                        }
                        else
                        {
                            bst.node[module * MODULE_CH_NUM + cnt].err_cnt++;
                        }
                    }
                    else
                    {
                           bst.node[module * MODULE_CH_NUM + cnt].err_cnt = 0;
                    }
                }
            }
        }
	}
}

/**************************************************
* name      :   read_all_module_ver
* funtion   :   读取所有模块版本信息
* parameter :   void
* return    :   void
***************************************************/
void read_all_module_ver(void)
{
    static  u8 read_ver_cnt = 1;
            u8 ret          = 0;

	if(dgus_get_current_page_id() != 7)  //第70页为读取软件版本页面
	{
		return;
	}
    if(bst.module_info[read_ver_cnt -1].link_status != MODULE_LINK_OFF)
	{
		ret = read_module_version(read_ver_cnt, &bst.module_info[read_ver_cnt -1].module_ver[0]);
		if(ret)
        {
            bst.module_info[read_ver_cnt-1].module_ver[9] = '\0';
        }
	}
	if(++read_ver_cnt > MODULE_NUM)   //MODULE_NUM
	{
		read_ver_cnt = 1;
	}
}

/**************************************************
* name      :   evc_group_led_on
* funtion   :   整组亮灯
* parameter :   void
* return    :   void
***************************************************/
void evc_group_led_on(void)
{
/*
    u8 module;
    u8 group;
    u8 led_status[8]={0};
    u8 ret;

    for(group = 0;group < GROUP_NUM2; group++)	//所有组别一起扫描
    {
        if(evc_ctr.led_group_ctr[group] == 1)	 //全组亮灯
        {
            evc_ctr.led_group_ctr[group] = 0;
             for(module =group*GROUP_MODULE_NUM; module<((group+1)*GROUP_MODULE_NUM); module++)
             {
                led_status[7]= 1;
                led_status[5]= 1;
                led_status[3]= 1;
                led_status[1]= 1;
                ret = module_single_ctr(module+1,led_status,CLIG);
                if(ret)
                {
                     memcpy(&remeder[module][0],led_status,8);
                }
             }
        }
    }
*/
}

/**************************************************
* name      :   evc_group_led_off
* funtion   :   整组关灯
* parameter :   void
* return    :   void
***************************************************/
void evc_group_led_off(void)
{
/*
		u8 module;
		u8 group;
		u8 led_status[8]={0};
		u8 ret;

		for(group = 0;group < GROUP_NUM2; group++)	//所有组别一起扫描
		{
			if(evc_ctr.led_group_ctr[group] == 2)	 //全组关灯
			{
				 evc_ctr.led_group_ctr[group] = 0;
				 memset(&led_status[0],0,8);
				 for(module =group*GROUP_MODULE_NUM; module<((group+1)*GROUP_MODULE_NUM); module++)
				 {
						ret = module_single_ctr(module+1,led_status,CLIG);
					 if(ret)
					 {
						  memcpy(&remeder[module][0],led_status,8);
					 }
				 }
			}
		}
*/
}

/**************************************************
* name      :   evc_group_charge_discharge
* funtion   :   整组充电 放电
* parameter :   void
* return    :   void
***************************************************/
void evc_group_charge_discharge(void)
{
/*
	u8 module,channel;
	u8 group;
	u16 set_v,set_i;
	u8 module_set_status[8]={0};		//模块工作状态
	u8 now_step;
	u8 channel_end_status;			//通道当前的状态


	for(group = 0;group < GROUP_NUM2; group++)	//所有组别一起扫描
	{
		if(evc_ctr.evc_group_ctr[group] == 2 || evc_ctr.evc_group_ctr[group] == 1)	 //全组充电或者放电
		{
			 now_step = Global_info[group].now_step;
			 set_v = Test_Step_Comm[group][now_step].parameter2;	//工作电压
			 set_i = Test_Step_Comm[group][now_step].parameter1;	//工作电流

			//现将所有模块关闭
			 memset(&module_set_status[0],0,8);
			 for(module =group*GROUP_MODULE_NUM; module<((group+1)*+GROUP_MODULE_NUM); module++)
			 {
				 if(N.NPS_info[module].link_status == MODULE_LINK_OFF)
						continue;		//掉线直接跳过
					module_single_ctr(module+1,&module_set_status[0],C_S_S);		//先将所有模块关闭
			 }

			 //设置电压

			 for(module =group*GROUP_MODULE_NUM; module<((group+1)*+GROUP_MODULE_NUM); module++)	//设置电压
			 {
				 if(N.NPS_info[module].link_status == MODULE_LINK_OFF)
					{
						continue;		//掉线直接跳过
					}
					for(channel = 0;channel < MODULE_CH_NUM;channel++)
					{
						channel_end_status = get_group_node_end_status(module,channel);	//该节点已经完成状态
						if((channel_end_status !=0) || ((N.NPS_info[module].node_info[channel] & NODE_ERR_BIT)!=0))		//故障
						{
							 module_set_status[channel*2+1] = 0;
							 module_set_status[channel*2+0] = 0;
						}
						else
						{
							 module_set_status[channel*2+1]= (u8)(set_v&0xff);
							 module_set_status[channel*2+0]= (u8)((set_v>>8)&0XFF);
						}
				 }
					module_single_ctr(module+1,&module_set_status[0],SETV);
			 }

			 //设置电流
			 for(module =group*GROUP_MODULE_NUM; module<((group+1)*+GROUP_MODULE_NUM); module++)	//设置电流
			 {
				 if(N.NPS_info[module].link_status == MODULE_LINK_OFF)
					{
						continue;		//掉线直接跳过
					}
					for(channel = 0;channel < MODULE_CH_NUM;channel++)
				  {
						channel_end_status = get_group_node_end_status(module,channel);	//该节点已经完成状态
						if((channel_end_status !=0)|| ((N.NPS_info[module].node_info[channel] & NODE_ERR_BIT)!=0))		//故障
						{
							 module_set_status[channel*2+1] = 0;
							 module_set_status[channel*2+0] = 0;
						}
						else
						{
								module_set_status[channel*2+1]= (u8)(set_i&0xff);
								module_set_status[channel*2+0]= (u8)((set_i>>8)&0XFF);
						}
				  }
				 module_single_ctr(module+1,&module_set_status[0],SETC);
			 }

			 //启动充放电
			 for(module =group*GROUP_MODULE_NUM; module<((group+1)*+GROUP_MODULE_NUM); module++)	//启动放电
			 {
				 if(N.NPS_info[module].link_status == MODULE_LINK_OFF)
					{
						continue;		//掉线直接跳过
					}
				  for(channel = 0;channel < MODULE_CH_NUM;channel++)
				  {
						 channel_end_status = get_group_node_end_status(module,channel);	//该节点已经完成状态
						 if((channel_end_status !=0)||(N.NPS_info[module].node_info[channel] & NODE_ERR_BIT)!=0)
						 {
							 module_set_status[channel*2+1] = 0;
							 module_set_status[channel*2+0] = 0;
						 }
						 else
						 {
							 module_set_status[channel*2+1] = evc_ctr.evc_group_ctr[group];
							 module_set_status[channel*2+0] = 0;
						 }
				  }
					module_single_ctr(module+1,&module_set_status[0],C_S_S);		//
			 }
			 evc_ctr.evc_group_ctr[group] = 0;
		}
	}
*/
}

/**************************************************
* name      :   evc_group_reset
* funtion   :   整组搁置
* parameter :   void
* return    :   void
***************************************************/
void evc_group_reset(void)
{
/*
	u8 module;
	u8 group;
	u8 module_set_status[8]={0};		//模块工作状态

	for(group = 0;group < GROUP_NUM2; group++)	//所有组别一起扫描
	{
		if(evc_ctr.evc_group_ctr[group] == 3)	 //全组搁置
		{
			 evc_ctr.evc_group_ctr[group] = 0;
			 memset(&module_set_status[0],0,8);
			 for(module =group*GROUP_MODULE_NUM; module<((group+1)*+GROUP_MODULE_NUM); module++)
			 {
					module_single_ctr(module+1,&module_set_status[0],C_S_S);		//
			 }
		}
	}
*/
}

/**************************************************
* name      :   evc_module_ctr_single
* funtion   :   模块单板控制事件
* parameter :   void
* return    :   void
***************************************************/
void evc_module_ctr_single(void)
{
/*
	u8 module;
	u16 set_v1,set_v2,set_v3,set_v4;
	u16 set_i1,set_i2,set_i3,set_i4;

	u8 module_set_status[8]={0};		//模块工作状态

   for(module = 0; module < MODULE_NUM; module++)
	 {
	    if(evc_ctr.module_ctr[module])	//非零表示该板需要控制
			{
				set_v1 = evc_ctr.set_v[module][0];
				set_v2 = evc_ctr.set_v[module][1];
				set_v3 = evc_ctr.set_v[module][2];
				set_v4 = evc_ctr.set_v[module][3];
				module_set_status[1]= (u8)(set_v1&0xff);
				module_set_status[0]= (u8)((set_v1>>8)&0XFF);
				module_set_status[3]= (u8)(set_v2&0xff);
				module_set_status[2]= (u8)((set_v2>>8)&0XFF);
				module_set_status[5]= (u8)(set_v3&0xff);
				module_set_status[4]= (u8)((set_v3>>8)&0XFF);
				module_set_status[7]= (u8)(set_v4&0xff);
				module_set_status[6]= (u8)((set_v4>>8)&0XFF);			//设置4个通道电压
				module_single_ctr(module+1,&module_set_status[0],SETV);		//
			}
	 }

	 for(module = 0; module < MODULE_NUM; module++)
	 {
	    if(evc_ctr.module_ctr[module])	//非零表示该板需要控制
			{
				set_i1 = evc_ctr.set_i[module][0];
				set_i2 = evc_ctr.set_i[module][1];
				set_i3 = evc_ctr.set_i[module][2];
				set_i4 = evc_ctr.set_i[module][3];
				module_set_status[1]= (u8)(set_i1&0xff);
				module_set_status[0]= (u8)((set_i1>>8)&0XFF);
				module_set_status[3]= (u8)(set_i2&0xff);
				module_set_status[2]= (u8)((set_i2>>8)&0XFF);
				module_set_status[5]= (u8)(set_i3&0xff);
				module_set_status[4]= (u8)((set_i3>>8)&0XFF);
				module_set_status[7]= (u8)(set_i4&0xff);
				module_set_status[6]= (u8)((set_i4>>8)&0XFF);
				module_single_ctr(module+1,&module_set_status[0],SETC);		//设置4个通道电流
			}
	 }

	 for(module = 0; module < MODULE_NUM; module++)
	 {
	    if(evc_ctr.module_ctr[module])	//非零表示该板需要控制
			{
               memset(&module_set_status[0],0,8);
			  module_set_status[1] = evc_ctr.ouput[module][0];
			  module_set_status[3] = evc_ctr.ouput[module][1];
			  module_set_status[5] = evc_ctr.ouput[module][2];
			  module_set_status[7] = evc_ctr.ouput[module][3];
			  module_single_ctr(module+1,&module_set_status[0],C_S_S);		//发送启动停止命令
              evc_ctr.module_ctr[module]  = 0;
			}
		}
*/
}

/**************************************************
* name      :   evc_module_mode_dir_cnv
* funtion   :   模块充放电模式直接切换
* parameter :   void
* return    :   void
***************************************************/
void evc_module_mode_dir_cnv(void)
{
/*
	u8 module;
	u8 module_set_status[8]={0};		//模块工作状态

    for(module = 0; module < MODULE_NUM; module++)
    {
        if(evc_ctr.module_ctr[module])	//非零表示该板需要控制
        {
            //先停止
            module_single_ctr(module+1,&module_set_status[0],C_S_S);		//发送启动停止命令
        }
    }
	evc_module_ctr_single();
*/
}

/**************************************************
* name      :   evc_group_debug_poll
* funtion   :   模式设置为调试模式
* parameter :   void
* return    :   void
***************************************************/
void evc_group_debug_poll(void)
{
/*
	u8 module;
	u8 group;
	u8 module_set_status[8]={0};		//模块工作状态

	for(group = 0;group < GROUP_NUM2; group++)	//所有组别一起扫描
	{
		if(evc_ctr.evc_debug_mode[group] != 0)	 //全组搁置
		{
			if(evc_ctr.evc_debug_mode[group] == 1)
			{ //调试模式
				 memset(&module_set_status[0],1,8);
			}
			else
			{  //正常模式
				 memset(&module_set_status[0],0,8);
			}
			evc_ctr.evc_debug_mode[group] = 0;
			for(module =group*GROUP_MODULE_NUM; module<((group+1)*+GROUP_MODULE_NUM); module++)
			{
			   module_single_ctr(module+1,&module_set_status[0],DEBUG_MODE);		//
			}
		}
	}
*/
}

/**************************************************
* name     : evc_evt_poll
* funtion  : 查询外部触发的EVC控制事件模块EVC事件处理函数
* parameter： NULL
* return   : NULL
***************************************************/
void evc_evt_poll(void)
{
/*
	u32 evt;  //led 事件

//	evt = os_evt_wait_or(0xffff, evc_poll_time);  //等待事件
//	if(evt == OS_R_EVT)  //收到数据
	{
//		evt = os_evt_get();  //获取事件

		evt = evc_ctr.evt_status;
		if(evt & LED_EVT_CTR_ON)		//打开组LED灯
		{
            evc_ctr.evt_status &=~LED_EVT_CTR_ON;
            evc_group_led_on();
		}

	  if(evt & LED_EVT_CTR_OFF)		//关闭组led
		{
			evc_ctr.evt_status &=~LED_EVT_CTR_OFF;
			evc_group_led_off();
		}

		if(evt & LED_EVT_CTR_CHARGE)		//整组充电
		{
			evc_ctr.evt_status &=~LED_EVT_CTR_CHARGE;
			evc_group_charge_discharge();
		}

		if(evt & LED_EVT_CTR_DISCHARGE)		//整组放电
		{
			  evc_ctr.evt_status &=~LED_EVT_CTR_DISCHARGE;
				evc_group_charge_discharge();
		}

		if(evt & LED_EVT_CTR_RESET)		//整组搁置
		{
			evc_ctr.evt_status &=~LED_EVT_CTR_RESET;
			evc_group_reset();
		}
		if(evt & MODULE_EVT_CTR_SINGLE)			//模块单板控制
		{
			evc_ctr.evt_status &=~MODULE_EVT_CTR_SINGLE;
			evc_module_ctr_single();
		}

		if(evt & MODULE_EVT_DIR_CNV)			//模块的工作模式直接切换
		{
			evc_ctr.evt_status &=~MODULE_EVT_DIR_CNV;
			evc_module_mode_dir_cnv();
		}
	}
*/
}

/**************************************************
* name     : check_linkoff_status
* funtion  : 查询所有模块到 连接状态,如果所有模块都显示掉线的话就重新初始化can 接口
* parameter： NULL
* return   : NULL
***************************************************/
void check_linkoff_status(void)
{
    /*
	u8 module;
	u8 linkoff_cnt;

	for(module = 0; module < MAX_NODE; module++)
	{
		if(N.NPS_info[module].link_status == MODULE_LINK_OFF)
          linkoff_cnt++;
	}
	if(linkoff_cnt == MAX_NODE)		//如果所有模块都掉线则重新配置一下CAN
	{
	    //初始化can
//		can_init();
	}*/
}

/**************************************************
* name     : evc_pthread_task
* funtion  : 后台服务 通信线程
* parameter：void
* return   : void
***************************************************/
void evc_pthread_task(void)
{
    dcdc_ctr_init();
    sem_init(&can_rx_sem,0, 0);
    can_init();         //can 初始化
    pthread_create(&can_rev_thread,NULL,(void *(*)(void *))can_rev_pthread_task,NULL);              //lcd 通信线程

    while(1)
    {
        // check_linkoff_status();                  //查询判断网络掉线状态
        // evc_evt_poll();
        // getifaddrs(&ifAddrStruct);
        // os_sem_init(&CAN1_Rx_SEM, 0);		    //can 中断接收信号量初始化

        node_led_run_poll();				//运行过程中LED查询控制
        node_finish_status_poll();		    //运行过程中启动停止查询

        read_all_module_status();			//读取所有模块的状态
        read_all_module_ver(); 				//读取软件版本号
        read_all_battery_i();				//读取电流
        read_all_battery_v();				//读取电压*/
     //   usleep(10*1000);

    }
    exit(1);
}

/***************end file****************************/
