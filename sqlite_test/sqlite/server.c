/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：server.c
*文件功能描述: 后台通信服务
*创建标识   ：xuchangwei 20170819
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建
***************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>  //信号量
#include "server.h"
#include "sys_cfg.h"
#include "sys_sem.h"
#include "sys_logic_control.h"
#include "lcd_ui.h"
#include "evc.h"
#include "debug.h"
#include "sqlite_opera.h"

//后台服务相关宏定义
#define SVR_PORT           8080               //后台通信端口号
#define SOCK_REV_LEN        1024              //接收缓存区大小为1024byte
#define SOCK_SEND_LEN       1024              //发送缓存区大小为1024byte

#define TCP_CONFIG    0

#define HTONL(n) ((((unsigned int)(n) & 0xff000000) >> 24) | (((unsigned int)(n) & 0x00ff0000) >> 8) | (((unsigned int)(n) & 0x0000ff00) << 8) | (((unsigned int)(n) & 0x000000ff) << 24))
#define HTONS(n) (u16)((((u16) (n)) << 8) | (((u16) (n)) >> 8))

extern volatile int     save_data_flag;
       volatile int     delete_table_mun = 0;  //要删除的数据ID
extern          sem_t   sem;
extern          sem_t   delete_data_sem;

static struct sockaddr_in   client_addr;
static struct sockaddr_in   server_addr;
static struct timeval       so_timeo;                       //收发超时定时结构体
static          u8          udp_rcv_buff[SOCK_REV_LEN];     //接收缓存区
static          u8          udp_send_buff[SOCK_SEND_LEN];   //接收缓存区
//udp 通信相关变量定义
static          s16         sock_fd;                        //文件描述符
static          s16         client_len,server_len;
static          s16         udp_rcv_num = -1;
                sem_t       save_step_sem;                  //保存工步信号量
//static          u32         net_run_time;                   //保存网络启动时候的时间，要读取数据，必须在网络启动后的1秒后才开始，以防读到停止数据


/**************************************************
* name     : para_init
* funtion  : 参数初始化
* parameter：mun 通道
* return   :
***************************************************/
static void para_init(u16 mun){

    save_real_data[mun].working_status      =  0;
    bst.node[mun].cyc                       =  0;
    bst.node[mun].now_cyc                   =  0;
    bst.node[mun].sub_now_cyc               =  0;
    bst.node[mun].now_step                  =  0;
    bst.node[mun].run_status                =  0;
    bst.node[mun].step_end                  =  0;
    bst.node[mun].cc_cv                     =  0;
    bst.node[mun].time                      =  0;
    bst.node[mun].platform_time             =  0;
    bst.node[mun].platform_v                =  0;
    bst.node[mun].em                        =  0;
    bst.node[mun].cc_capacity               =  0;
    bst.node[mun].capacity                  =  0;
    bst.node[mun].discapacity               =  0;
    bst.node[mun].real_capacity             =  0;

}

/**************************************************
* name     : check_sum
* funtion  : 后台通信命令校验函数
* parameter：buff,需要校验的数据指针,len-需要校验的数据长度
* return   : true - 校验通过 , false - 校验失败
***************************************************/
static u8 check_sum(u8 *buff, u16 len)
{
    u8 i = 0;
    u16 sum = 0;
    u16 temp;

   // temp = HTONL(*(u16*) & buff[len-1]);
    temp = (buff[len - 2] << 8  | buff[len - 1]);
    printf("temp = %x\r\n", temp);

    //计算校验吗
	do{
        sum += buff[i++];
	}while(i < (len-2));

    printf("sum = %x\r\n", sum);
    if(sum == temp)
        return TRUE;
    else
        return FALSE;
}

/**************************************************
* name     : gen_check_sum
* funtion  : 生成发送的校验码
* parameter：buff,需要校验的数据指针,len-需要校验的数据长度
* return   : true - 校验通过 , false - 校验失败
***************************************************/
static u16 gen_check_sum(u8 *buff, u16 len)
{
	u16 i;
	u16 sum = 0;
    for(i = 0; i < len - 2; i++){
        sum += buff[i];
    }
    /*
	do{
		sum += buff[i++];
	}while(i < (len-2));*/
    return sum;
}

/******************************************************************
* 函数名: getServerIp
* 功能:   获取服务器IP
* 参数：
* 返回值:
******************************************************************/
void getServerIp(char *string_ip, sys_cfg_def int_ip)
{
    u8 temp_ip[4];
    int i;
    temp_ip[0] = int_ip.rip1;
    temp_ip[1] = int_ip.rip2;
    temp_ip[2] = int_ip.rip3;
    temp_ip[3] = int_ip.rip4;

    for(i = 0; i < 4; i++){
        if((temp_ip[i] / 100) != 0 ){
            *string_ip = (temp_ip[i] / 100) + '0';
            temp_ip[i] = temp_ip[i] % 100;
            string_ip++;
        }else{
            *string_ip = '0';
            string_ip++;
        }
        if(temp_ip[i] / 10 != 0){
            *string_ip = (temp_ip[i] / 10) + '0';
            temp_ip[i] = temp_ip[i] % 10;
            string_ip++;
        }else{
            *string_ip = '0';
            string_ip++;
        }
         *string_ip = temp_ip[i] + '0';
         string_ip++;
         if(i != 3){
             *string_ip = '.';
             string_ip++;
         }
         else{
             *string_ip = '\0';
         }
    }
}

/**************************************************
* name     : pc_clear_step_inst
* funtion  : 清除工步列表
* parameter：add
* return   : void
***************************************************/
static void pc_clear_step_inst(u8 add)
{
	/*
	char i,j;
	if(add == 0xff)
	{
		for(i = 0;i < 8;i++)
		{
			for(j = 0;j < Global_info[add].step_num;j++)
			{
				Test_Step_Comm[i][j].Action_type = 0;
				Test_Step_Comm[i][j].parameter1  = 0;
				Test_Step_Comm[i][j].parameter2  = 0;
				Test_Step_Comm[i][j].conditions1 = 0;
				Test_Step_Comm[i][j].conditions2 = 0;
			}
			Global_info[add].step_num = 0;
		}
	}
	else
	{
		for(j = 0;j < Global_info[add].step_num;j++)
		{
			Test_Step_Comm[add][j].Action_type = 0;
			Test_Step_Comm[add][j].parameter1  = 0;
			Test_Step_Comm[add][j].parameter2  = 0;
			Test_Step_Comm[add][j].conditions1 = 0;
			Test_Step_Comm[add][j].conditions2 = 0;
		}
		Global_info[add].step_num = 0;
	}
	*/
}


/**************************************************
* name     : udp_communication_init
* funtion  : udp通信初始化
* parameter：NUll
* return   : NULL
***************************************************/
void udp_communication_init(void)
{
    #if NET_DEBUG_CONFIG > 0
    printf("udp_communication_init\r\n");
    #endif

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM,0)) < 0)          //后区描述符
    {
        perror("socket create error\n");
        exit(1);
    }

    memset(&server_addr,0,sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(sys_cfg.rport);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_len = sizeof(struct sockaddr_in);
    server_len = sizeof(struct sockaddr_in);

    so_timeo.tv_sec = 1;
    so_timeo.tv_usec = 0;
    setsockopt(sock_fd,SOL_SOCKET,SO_RCVTIMEO,&so_timeo,sizeof(so_timeo));  //将udp接收超时设置为1秒
    setsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&so_timeo,sizeof(so_timeo));  //将udp发送超时设置为1秒

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) < 0)  //绑定
    {
        perror("bind error.\n");
        return;
    }
}

/**************************************************
* name     : tcp_communication_init
* funtion  : tcp通信初始化
* parameter：NUll
* return   : NULL
***************************************************/
void tcp_communication_init(void)
{
    int ret;
    char serverIP[24];
    u16 serverPort;

    getServerIp(serverIP, sys_cfg);
    serverPort = sys_cfg.rport;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM,0)) < 0)          //后区描述符
    {
        perror("socket create error\n");
        exit(1);
    }

    memset(&server_addr,0,sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serverPort);
  //  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_addr.s_addr = inet_addr(serverIP);
    client_len = sizeof(struct sockaddr_in);
    server_len = sizeof(struct sockaddr_in);

    so_timeo.tv_sec=1;
    so_timeo.tv_usec=0;
    setsockopt(sock_fd,SOL_SOCKET,SO_RCVTIMEO,&so_timeo,sizeof(so_timeo));  //将udp接收超时设置为1秒
    setsockopt(sock_fd,SOL_SOCKET,SO_SNDTIMEO,&so_timeo,sizeof(so_timeo));  //将udp发送超时设置为1秒

    ret = connect(sock_fd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr));
    if(ret < 0){
        printf("connect faild\r\n");
        close(sock_fd);
        return ;
    }else{
        printf("connect success\r\n");
    }

}



/**************************************************
* name     : udp_send
* funtion  : udp发送数据函数，发送之前添加校验和数据帧头
* parameter：buff- 需要发送数据的地址指针， len- 需要发送数据到数据长度
* return   : NULL
***************************************************/
void udp_send(u8 *buff, u16 len)
{
    u8 cnt;
    u16 sum;
   // u8 group;

    if(len > SOCK_SEND_LEN)
    {
      return;
    }
    buff[0] = SVR_START;   //0xAA
   // group = buff[3];

    sum = gen_check_sum(buff,len);

    buff[len - 2] = (u8)(sum >> 8 ) & 0xff;
    buff[len - 1] = (u8)(sum) & 0xff;
    cnt = 5;

    do{
        #if TCP_CONFIG > 0
        if(sendto(sock_fd, buff,  len, 0, (struct sockaddr *)&client_addr, client_len) > 0) //sizeof
        {
            //printf("sendto success\n");
            break;
        }
        #else
        if(sendto(sock_fd, buff,  len, 0, (struct sockaddr *)&client_addr, client_len) > 0) //sizeof
        {
            //printf("sendto success\n");
            break;
        }
        #endif
    }while(cnt--);
/*
    if(cnt == 0){   //如果发送五次都失败 表示掉线 否则表示在线
        Global_info[group].net_status = 0x01;
    }else{
        Global_info[group].net_status = 0x02;
    }

    if(sendto(sock_fd, buff,  len, 0, (struct sockaddr *)&client_addr, client_len)<0) //sizeof
    {
        printf("sendto error\n");
       // exit(1);
    }
*/
}


/**************************************************
* name     : cfg_step_decode
* funtion  :  配置工步解码函数
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void cfg_step_decode(u8 *buff, u16 len)
{
    u8 sendToServer[32] = {0};
    u8 step_mun;
    u16 bat_mun, i, j;
    tsCFG_STEP_DATA  *temp_step_data = NULL;
    cmd_send_style *dowm_data = NULL;   //上位机下发数据
    cmd_send_style *up_data = NULL;     //回复数据

    if(check_sum(buff,len) != TRUE)
    {
        #if NET_DEBUG_CONFIG > 0
        printf("check_sum error!\r\n");
        #endif
        return;
    }

    dowm_data = (cmd_send_style *)buff;
    up_data = (cmd_send_style *)sendToServer;

    memcpy(&up_data->start, &dowm_data->start, 6);
    up_data->dataLen = htons(8);

    bat_mun = (buff[3] << 8 | buff[4]);
    step_mun = buff[6];

    printf("bat_mun = %d, step_mun = %d\r\n",bat_mun, step_mun);

    if(bat_mun == 0xffff){
        for(j = 0; j < MAX_NODE; j++){
            temp_step_data =(tsCFG_STEP_DATA *)&(buff[7]);
            if(bst.node[j].run_status == SYS_RUNNING)      //
            {
                #if NET_DEBUG_CONFIG > 0
                printf("node is running, Please try again wait moment!\r\n");
                #endif
                continue;
            }

            if(step_mun  == 0 || step_mun > MAX_STEP)
            {
                #if NET_DEBUG_CONFIG > 0
                printf("step_mun error!\r\n");
                #endif
                continue;
            }

            memset(&bst.node[j].step[0], 0, MAX_STEP * sizeof(NODE_STEP_DEF));

            bst.node[j].step_num = step_mun;
            para_init(j);

            for(i = 0; i < MAX_STEP; i++){
                if(i < step_mun){
                    bst.node[j].step[i].type        = temp_step_data->type;
                    bst.node[j].step[i].parm1       = htons(temp_step_data->parm1);
                    bst.node[j].step[i].parm2       = htons(temp_step_data->parm2);
                    bst.node[j].step[i].cond1       = htonl(temp_step_data->cond1); //<< 16 | htons(temp_step_data->cond1 >> 16);
                    bst.node[j].step[i].cond2       = htonl(temp_step_data->cond2);
                    bst.node[j].step[i].cond3       = htonl(temp_step_data->cond3);
                    bst.node[j].step[i].cond4       = htonl(temp_step_data->cond4); //   x = (x << 8) & 0xFF00FF00FF | (x >> 8) & 0x00FF00FF00Ff;return x << 16 | x >> 16;
                    bst.node[j].step[i].jump_step   = (temp_step_data->cond5);
                    bst.node[j].step[i].sub_cyc     = (temp_step_data->cond6);
                    bst.node[j].time_tmp_1          = 0;

                    temp_step_data++;
                }else{
                    bst.node[j].step[i].type        = 0xf0;
                    bst.node[j].step[i].parm1       = 0;
                    bst.node[j].step[i].parm2       = 0;
                    bst.node[j].step[i].cond1       = 0;
                    bst.node[j].step[i].cond2       = 0;
                    bst.node[j].step[i].cond3       = 0;
                    bst.node[j].step[i].cond4       = 0;
                    bst.node[j].step[i].jump_step   = 0;
                    bst.node[j].step[i].sub_cyc     = 0;
                    bst.node[j].time_tmp_1        =   0;
                }
            }
            //save_step_mun(j, step_mun);
            save_real_data[j].working_status = 0;
            //udp_send(sendToServer,8); //按源数据返回应答数据 去掉数据部分 重新发送
        }
        udp_send(sendToServer,8); //按源数据返回应答数据 去掉数据部分 重新发送
        sem_post(&save_step_sem);
        return;
    }

    if(bat_mun > MAX_NODE){
        return;
    }

    temp_step_data = (tsCFG_STEP_DATA *)&(buff[7]);
    if(bst.node[bat_mun].run_status == SYS_RUNNING)      //
	{
        #if NET_DEBUG_CONFIG > 0
        printf("node is running, Please try again wait moment!\r\n");
        #endif
        return;
	}

    if(step_mun  == 0 || step_mun > MAX_STEP)
	{
        #if NET_DEBUG_CONFIG > 0
        printf("step_mun error!\r\n");
        #endif
		return;
	}

	memset(&bst.node[bat_mun].step[0], 0, MAX_STEP * sizeof(NODE_STEP_DEF));

	bst.node[bat_mun].step_num = step_mun;
    para_init(bat_mun);
    for(i = 0; i < MAX_STEP; i++){
        if(i < step_mun){
            bst.node[bat_mun].step[i].type      = temp_step_data->type;
            bst.node[bat_mun].step[i].parm1     = htons(temp_step_data->parm1);
            bst.node[bat_mun].step[i].parm2     = htons(temp_step_data->parm2);
            bst.node[bat_mun].step[i].cond1     = htonl(temp_step_data->cond1);
            bst.node[bat_mun].step[i].cond2     = htonl(temp_step_data->cond2);
            bst.node[bat_mun].step[i].cond3     = htonl(temp_step_data->cond3);
            bst.node[bat_mun].step[i].cond4     = htonl(temp_step_data->cond4); //   x = (x << 8) & 0xFF00FF00FF | (x >> 8) & 0x00FF00FF00Ff;return x << 16 | x >> 16;
            bst.node[bat_mun].step[i].jump_step = (temp_step_data->cond5);
            bst.node[bat_mun].step[i].sub_cyc   = (temp_step_data->cond6);
            bst.node[bat_mun].time_tmp_1        =   0;

            temp_step_data++;
        }else{
            bst.node[bat_mun].step[i].type      = 0xf0;
            bst.node[bat_mun].step[i].parm1     = 0;
            bst.node[bat_mun].step[i].parm2     = 0;
            bst.node[bat_mun].step[i].cond1     = 0;
            bst.node[bat_mun].step[i].cond2     = 0;
            bst.node[bat_mun].step[i].cond3     = 0;
            bst.node[bat_mun].step[i].cond4     = 0;
            bst.node[bat_mun].step[i].jump_step = 0;
            bst.node[bat_mun].step[i].sub_cyc   = 0;
            bst.node[bat_mun].time_tmp_1        =   0;
        }
    }

    open_sqlite_db();
    save_step_mun(bat_mun, step_mun);
    close_sqlite_db();

    udp_send(sendToServer,8); //按源数据返回应答数据 去掉数据部分 重新发送
}

/**************************************************
* name     : cfg_sys_parm
* funtion  : 配置系统参数
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void cfg_sys_parm(u8 *buff, u16 len)
{
    u8 sendToServer[32] = {0};
    cmd_send_style *dowm_data = NULL;   //上位机下发数据
    cmd_send_style *up_data = NULL;     //回复数据

    dowm_data = (cmd_send_style *)buff;
    up_data = (cmd_send_style *)sendToServer;

    memcpy(&up_data->start, &dowm_data->start, 6);
    up_data->dataLen = htons(8);

    tsCFG_SYS_PARM *temp_sys_parm = NULL;

    if(check_sum(buff,len) != TRUE)
    {
        #if NET_DEBUG_CONFIG > 0
        printf("check_sum error!\r\n");
        #endif
        return;
    }

   // bat_mun = (buff[3] << 8 | buff[4]);
    temp_sys_parm =(tsCFG_SYS_PARM*)&(buff[6]);

    sys_cfg.led_lighting_mode   = temp_sys_parm->ligth_type;
    sys_cfg.low_volt            = (temp_sys_parm->parm1);
    sys_cfg.high_volt           = (temp_sys_parm->parm2);
    sys_cfg.protect_low_v       = (temp_sys_parm->parm3);
    sys_cfg.protect_high_v      = (temp_sys_parm->parm4);
    sys_cfg.protect_low_i       = (temp_sys_parm->parm5);
    sys_cfg.protect_high_i      = (temp_sys_parm->parm6);
    sys_cfg.capacityMode        = (temp_sys_parm->parm7);

    update_sys_config_values(sys_cfg);

    udp_send(sendToServer,8); //按源数据返回应答数据
}

/**************************************************
* name     : clr_step_decode
* funtion  : 清除工步或者读取工步
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void clr_or_read_step_decode(u8 *buff, u16 len)
{
    u8 i;
    u16 group;
    read_step_style  *temp_downlink = NULL;     //下行数据指针
    read_step_style  *temp_uplink = NULL;       //上行数据指针
    step_data   *temp_step_data = NULL;

    if(check_sum(buff,len) != TRUE)
    {
        #if NET_DEBUG_CONFIG > 0
        printf("clr_or_read_step_decode check error!\r\n");
        #endif
        return;
    }

    temp_downlink   =   (read_step_style *)buff;
    temp_uplink     =   (read_step_style *)udp_send_buff;
    temp_step_data  =   (step_data *)&temp_uplink->data;      //上传工步数据到起始地址
    memcpy(&temp_uplink->start,&temp_downlink->start,6);

    group = htons(temp_downlink->startOption);

    if(group > MAX_NODE){
        return;
    }

    #if NET_DEBUG_CONFIG > 0
    printf("group = %d\r\n", group);
    #endif

    temp_uplink->step_sum = (bst.node[group].step_num);

    #if NET_DEBUG_CONFIG > 0
    printf("temp_uplink->step_sum = %d\r\n", temp_uplink->step_sum);
    #endif

    for(i = 0; i < bst.node[group].step_num; i++){
        temp_step_data->s_type        = (bst.node[group].step[i].type);
        temp_step_data->s_parm1       = htons(bst.node[group].step[i].parm1);
        temp_step_data->s_parm2       = htons(bst.node[group].step[i].parm2);
        temp_step_data->s_cond1       = htonl(bst.node[group].step[i].cond1);
        temp_step_data->s_cond2       = htonl(bst.node[group].step[i].cond2);
        temp_step_data->s_cond3       = htonl(bst.node[group].step[i].cond3);
        temp_step_data->s_cond4       = htonl(bst.node[group].step[i].cond4);
        #if VERSION > 0
        temp_step_data->s_cond5       = (bst.node[group].step[i].jump_step);
        temp_step_data->s_cond6       = (bst.node[group].step[i].sub_cyc);
        #endif
        temp_step_data++;
    }
    #if VERSION > 0
    temp_uplink->dataLen = htons(bst.node[group].step_num * 23 + 9);
    #else
    temp_uplink->dataLen = htons(bst.node[group].step_num * 15 + 9);
    #endif

    #if NET_DEBUG_CONFIG > 0
    printf("temp_uplink->dataLen = %x\r\n", htons(temp_uplink->dataLen));
    #endif
   /* for(i = 0; i < temp_uplink->dataLen; i ++){
                printf("%x", udp_send_buff[i]);
                printf(" ");
    }
            printf("\r\n");*/
    udp_send(udp_send_buff,htons(temp_uplink->dataLen)); //按源数据返回应答数据
}


/**************************************************
* name     : read_group_run_data_decode
* funtion  : 取得整组运行数据
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void read_group_run_data_decode(u8 *buff, u16 len)
{
    u8              i;
    u16             returnLen, batMum, batSum;

    cmd_send_style  *temp_downlink      = NULL;     //下行数据指针
    cmd_send_style  *temp_uplink        = NULL;     //下行数据指针

    realtime_data_style_new *temp_run_data  = NULL;

    if(check_sum(buff, len) != TRUE){
        #if NET_DEBUG_CONFIG > 0
        printf("read_group_run_data_decode check error!\r\n");
        #endif
        return;
    }

    temp_downlink = (cmd_send_style *)buff;
    temp_uplink   = (cmd_send_style *)udp_send_buff;
    temp_run_data = (realtime_data_style_new *)&temp_uplink->data;

    memcpy(&temp_uplink->start,&temp_downlink->start,8);

    batMum = htons(temp_downlink->startOption);
    batSum = htons(temp_downlink->readMun);

    if((batSum + batMum) > MAX_NODE || batSum > 31){
        #if NET_DEBUG_CONFIG > 0
        printf("read node to large, Please reset again!\r\n");
        #endif
        temp_uplink->dataLen = htons(8);
        udp_send(udp_send_buff, 8);//按源数据返回应答数据
        return;
    }
    #if NET_DEBUG_CONFIG > 0
    printf("batMum = %d batSum = %d\r\n", batMum, batSum);
    #endif

    for(i = 0; i < batSum; i++){
        if(bst.node[batMum].run_status != SYS_RUNNING && bst.node[batMum].run_status != SYS_START){
            temp_run_data->cycMun       =  0x00;//htons(5);
            temp_run_data->sub_cycMUn   =  0x00;//3;
            temp_run_data->stepMun      =  0x00;//6;
            temp_run_data->stepStyle    =  0xf0;
            temp_run_data->V            =  htons(bst.node[batMum].v);//htons(3500);
            temp_run_data->I            =  htonl(bst.node[batMum].i);//htonl(600000);
            temp_run_data->cap          =  0x00;//htonl(600001);
            temp_run_data->power        =  0x00;//htonl(600002);
            temp_run_data->ccRatio      =  0x00;//htons(100);
            temp_run_data->platTime     =  0x00;//htons(3000);
            temp_run_data->runTime      =  0x00;//htonl(600003);
            temp_run_data->noteStatue   =  bst.node[batMum].node_status;//5;
            temp_run_data->returnFlag   =  0x01;//1;
            temp_run_data->returnRatio  =  0x00;//90;
        }else{
            temp_run_data->cycMun       =  htons(bst.node[batMum].now_cyc);
            temp_run_data->stepMun      =  (bst.node[batMum].now_step + 1);
            temp_run_data->sub_cycMUn   =  (bst.node[batMum].sub_now_cyc);

            temp_run_data->stepStyle    =  (bst.node[batMum].work_type);
            if(bst.node[batMum].read_start_v_flag)   //开始
            {
                temp_run_data->V        =  htons(bst.node[batMum].start_v);
                temp_run_data->I        =  htonl(bst.node[batMum].start_i);
            }
            else  if(bst.node[batMum].step_end)   //结束
            {
                temp_run_data->V        =  htons(bst.node[batMum].end_v);
                temp_run_data->I        =  htonl(bst.node[batMum].end_i);
            }
            else    //运行中
            {
                temp_run_data->V        =  htons(bst.node[batMum].v);
                temp_run_data->I        =  htonl(bst.node[batMum].i);
            }

            if(bst.node[batMum].work_type == 1){
                temp_run_data->cap      =  htonl(bst.node[batMum].capacity / 3600);
            }else if(bst.node[batMum].work_type == 3){
                temp_run_data->cap      =  htonl(bst.node[batMum].discapacity / 3600);
            }else{
                temp_run_data->cap      =  0;
            }

            temp_run_data->power        =  htonl(bst.node[batMum].em / 3600);
            temp_run_data->ccRatio      =  htons(bst.node[batMum].cc_cv / 10);
            temp_run_data->platTime     =  htons(bst.node[batMum].platform_time);
            temp_run_data->runTime      =  htonl(bst.node[batMum].time);
            temp_run_data->noteStatue   =  bst.node[batMum].node_status;

            if(bst.node[batMum].recharge_status == 0)  //分容、
            {
                temp_run_data->returnFlag   = 0x01; //分容标识
                temp_run_data->returnRatio  = 0xFF;
            }
            else
            {
                temp_run_data->returnFlag   = 0x02; //返充标识
                temp_run_data->returnRatio  = bst.node[batMum].recharge_ratio;
            }
        }
        temp_run_data++;
        batMum++;
    }

    returnLen = batSum * 30;

    temp_uplink->dataLen = htons(returnLen + 10);
    printf("temp_uplink->dataLen = %d\r\n", htons(temp_uplink->dataLen));

    /*
    for(i = 0; i < temp_uplink->dataLen; i ++){
                printf("%x", udp_send_buff[i]);
                printf(" ");
    }
    printf("\r\n");*/

    udp_send(udp_send_buff, htons(temp_uplink->dataLen));  //按源数据返回应答数据
}

/**************************************************
* name     : send_signal_bat_cap
* funtion  : 下发单点容量
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void send_signal_bat_cap(u8 *buff, u16 len)
{
    u8  i, temp_nodeS;
    u16 node_start;
    u16 node_sum;

    u8 sendToServer[32] = {0};
    cmd_send_style *dowm_data = NULL;   //上位机下发数据
    cmd_send_style *up_data = NULL;     //回复数据

    dowm_data = (cmd_send_style *)buff;
    up_data = (cmd_send_style *)sendToServer;

    memcpy(&up_data->start, &dowm_data->start, 6);
    up_data->dataLen = htons(8);

    if(check_sum(buff, len) != TRUE){
        #if NET_DEBUG_CONFIG > 0
        printf("send_signal_bat_cap check error!\r\n");
        #endif
        return;
    }

    node_start  = buff[3] << 8 | buff[4];
    node_sum    = buff[6] << 8 | buff[7];

    temp_nodeS = node_start;    //临时保存单点起始节点
    printf("node_start = %d, node_sum = %d\r\n", node_start, node_sum);
    open_sqlite_db();
    if((node_start + node_sum) > (MAX_NODE + 1)){
        #if NET_DEBUG_CONFIG > 0
        printf("The send node is to large!\r\n");
        #endif
        return ;
    }

    for(i = 0; i < node_sum; i++){
        bst.node[node_start].recharge_cap = (buff[8 + i*4] << 24 | buff[9 + i*4] << 16 | buff[10 + i*4] << 8 | buff[11 + i*4]);
        #if NET_DEBUG_CONFIG > 0
        printf("bst.node[%d].recharge_cap = %x\r\n", node_start, bst.node[node_start].recharge_cap);
        #endif
        save_stepcfg_fun(node_start);       //保存进数据库
        node_start++;
    }

    udp_send(sendToServer, 8);
    close_sqlite_db();
}

/**************************************************
* name     : read_bat_cap
* funtion  : 读取单点容量
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void read_bat_cap(u8 *buff, u16 len)
{
    u8 i;
    u16 node_start;
    u16 node_sum;

    send_cap *bat_cap = NULL;    //保存容量指针
    cmd_send_style  *temp_downlink      = NULL;     //下行数据指针
    cmd_send_style  *temp_uplink        = NULL;     //下行数据指针

    temp_downlink = (cmd_send_style *)buff;
    temp_uplink   = (cmd_send_style *)udp_send_buff;
    bat_cap = (send_cap *)&temp_uplink->data;

    memcpy(&temp_uplink->start,&temp_downlink->start,8);

    if(check_sum(buff, len) != TRUE){
        #if NET_DEBUG_CONFIG > 0
        printf("read_bat_cap check error!\r\n");
        #endif
        return;
    }

    node_start  = buff[3] << 8 | buff[4];
    node_sum    = buff[6] << 8 | buff[7];
    printf("node_start = %d, node_sum = %d\r\n", node_start, node_sum);

    if((node_start + node_sum) > (MAX_NODE + 1)){
        #if NET_DEBUG_CONFIG > 0
        printf("The send node is to large!\r\n");
        #endif
        return ;
    }

    for(i = 0; i < node_sum; i++){
        printf("bst.node[node_start].recharge_cap = %x", bst.node[node_start].recharge_cap);
        bat_cap->read_cap = htonl(bst.node[node_start].recharge_cap);
        bat_cap++;
        node_start++;
    }

    temp_uplink->dataLen = htons(10 + 4 * node_sum);

    udp_send(udp_send_buff, htons(temp_uplink->dataLen));  //按源数据返回应答数据
}

/**************************************************
* name     : node_enable_decode
* funtion  : 节点使能
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void node_enable_decode(u8 *buff, u16 len)
{
    u8  status_tmp, i;
    u16 start_option;
    u16 node_mun;

    u8 sendToServer[32] = {0};
    cmd_send_style *dowm_data = NULL;   //上位机下发数据
    cmd_send_style *up_data = NULL;     //回复数据

    dowm_data = (cmd_send_style *)buff;
    up_data = (cmd_send_style *)sendToServer;

    memcpy(&up_data->start, &dowm_data->start, 6);
    up_data->dataLen = htons(8);

    if(check_sum(buff, len) != TRUE){
        #if NET_DEBUG_CONFIG > 0
        printf("node_enable_decode check error!\r\n");
        #endif
        return;
    }

    start_option = buff[3] << 8 | buff[4];
    node_mun     = buff[6] << 8 | buff[7];
    printf("start_option = %d, node_mun = %d\r\n", start_option, node_mun);

    /*如果发送的节点超过最大节点范围，不处理返回*/
    if((start_option + node_mun) > (MAX_NODE + 1) ){
        #if NET_DEBUG_CONFIG > 0
        printf("The send node mun to large!\r\n");
        #endif
        return ;
    }

    for(i = 0; i < node_mun; i++){
        if(buff[8 + i] == 0)					//停止
        {
            bst.node[start_option].run_status = SYS_DISABLE;		//需要停止
        }
        else if(buff[8 + i] == 1)
        {
            bst.node[start_option].run_status = SYS_IDLE;		//需要停止
        }
        start_option++;
    }

    udp_send(sendToServer, 8);
}

/**************************************************
* name     : read_step_status_decode
* funtion  : 读取当前 运行工步状态
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void read_step_status_decode(u8 *buff, u16 len)
{
 /*
    u8 group,now_step;
    tsSVR_CTR_STD_FH  *temp_downlink = NULL;     //下行数据指针
    tsSVR_CTR_STD_FH  *temp_uplink = NULL;     //下行数据指针
    tsSTEP_STA     *temp_step_sta = NULL;

    if(check_sum(buff,len) != TRUE)
    {
        return;
    }
    temp_downlink = (tsSVR_CTR_STD_FH *)buff;
    temp_uplink   = (tsSVR_CTR_STD_FH *)udp_send_buff;

    temp_step_sta =(tsSTEP_STA *)(&temp_uplink->data);

    memcpy(&temp_uplink->start,&temp_downlink->start,4);
    group = temp_downlink->group;
    now_step = Global_info[group].now_step;

    #if NET_DEBUG_CONFIG > 0
    printf("now_step = %d  group= %d\r\n", Global_info[group].step_num, group);
    #endif


    if(Global_info[group].step_num != 0){
        temp_step_sta->num   = Global_info[group].now_step;
        if(Global_info[group].run_status == SYS_COMP || Global_info[group].run_status == SYS_PAUSE){
            temp_step_sta->type  = CEND;
        }else{
            temp_step_sta->type  = Test_Step_Comm[group][now_step].Action_type;
        }
    }else{
        temp_step_sta->num   = 0;
        temp_step_sta->type  = CEND;
    }

    temp_step_sta->cycle = Global_info[group].now_cyc;
    temp_step_sta->time  = Global_info[group].time;

//返充
    if(N.recharge[group] != 0 )
    {
        temp_step_sta->recharge_flag = 1;             	//返充标志
        temp_step_sta->recharge_ratio = (u8)(N.recharge_ratio[group]&0xff);    //返充比例
    }
    else
    {
        temp_step_sta->recharge_flag = 0;           //返充标志
        temp_step_sta->recharge_ratio = (u8)(0xff);
    }
//
    //temp_step_sta->recharge_flag = 0;           // 暂固定为0
    udp_send(udp_send_buff,(sizeof(tsSVR_CTR_STD_FH) + sizeof(tsSTEP_STA)+ CRC_CODE_LEN - 1));
    */
}

/**************************************************
* name     : read_history_status_decode
* funtion  : 读取历史状数据状态
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void read_history_status_decode(u8 *buff, u16 len)
{
    u8 i;
    u16 nodeMun;
    int dataStatus; //是否存在历史数据标志位

    tsHISTORY_STA *bat_cap = NULL;    //保存容量指针
    cmd_search_hisdata_style  *temp_downlink      = NULL;     //下行数据指针
    cmd_search_hisdata_style  *temp_uplink        = NULL;     //下行数据指针

    temp_downlink = (cmd_search_hisdata_style *)buff;
    temp_uplink   = (cmd_search_hisdata_style *)udp_send_buff;
    bat_cap = (tsHISTORY_STA *)&temp_uplink->data;

    memcpy(&temp_uplink->start,&temp_downlink->start,6);

    if(check_sum(buff, len) != TRUE){
        #if NET_DEBUG_CONFIG > 0
        printf("read_bat_cap check error!\r\n");
        #endif
        return;
    }

    nodeMun  = htons(temp_uplink->startOption);
    printf("nodeMun = %d\r\n", nodeMun);
    if((nodeMun) > MAX_NODE ){
        #if NET_DEBUG_CONFIG > 0
        printf("The send node is to large!\r\n");
        #endif
        return ;
    }
    dataStatus = search_hisdata_fun(nodeMun);
    printf("dataStatus = %d\r\n", dataStatus);
    if(dataStatus == 0){    //没有历史数据
        bat_cap->sta = 0x00;
    }else if(dataStatus == 1){  //有历史数据
        bat_cap->sta = 0x01;
    }

    if(save_real_data[nodeMun].working_status == 0xA0){
        bat_cap->run_sta = 0x00;  //工步完成
    }else if(save_real_data[nodeMun].working_status == 0xAA || save_real_data[nodeMun].working_status == 0xA1){
        bat_cap->run_sta = 0x01;  //工步未完成
    }
    bat_cap++;

    temp_uplink->dataLen = htons(10);  //重新计算数据长度

    udp_send(udp_send_buff, htons(temp_uplink->dataLen));  //按源数据返回应答数据
}

/**************************************************
* name     : read_history_data_decode
* funtion  : 读取历史状数据
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void read_history_data_decode(u8 *buff, u16 len)
{
    u16 i, readFlag, sendCnt;
    u16 nodeMun, nodeSum;

    cmd_send_style  *temp_downlink      = NULL;     //下行数据指针
    cmd_send_style  *temp_uplink        = NULL;     //下行数据指针
    realtime_data_style_new *temp_run_data  = NULL;

    temp_downlink = (cmd_send_style *)buff;
    temp_uplink   = (cmd_send_style *)udp_send_buff;
    temp_run_data = (realtime_data_style_new *)&temp_uplink->data;

    memcpy(&temp_uplink->start,&temp_downlink->start,8);

    if(check_sum(buff, len) != TRUE){
        #if NET_DEBUG_CONFIG > 0
        printf("read_bat_cap check error!\r\n");
        #endif
        return;
    }

    nodeMun  = htons(temp_uplink->startOption);
    nodeSum = htons(temp_uplink->readMun);

    if((nodeSum + nodeMun) > MAX_NODE || nodeSum > 31){
        #if NET_DEBUG_CONFIG > 0
        printf("read node to large, Please reset again!\r\n");
        #endif
        temp_uplink->dataLen = htons(8);
        udp_send(udp_send_buff, 8);//按源数据返回应答数据
        return;
    }

    printf("nodeMun = %d\r\n", nodeMun);
    sendCnt = 0;
    for(i = nodeMun; i < (nodeMun + nodeSum); i++){
        readFlag = read_hisdata_fun(temp_run_data, i);
        if(readFlag == 1){
            sendCnt++;
        }
        temp_run_data++;
    }

    printf("sendCnt = %d\r\n", sendCnt);
    if(sendCnt > 0){
        temp_uplink->dataLen = htons(nodeSum * 30 + 10);
        udp_send(udp_send_buff,htons(temp_uplink->dataLen)); //按源数据返回应答数据
    }else{
        temp_uplink->dataLen = htons(10);
        udp_send(udp_send_buff,htons(temp_uplink->dataLen)); //按源数据返回应答数据
    }

}

/**************************************************
* name     : ctr_pump_decode
* funtion  : 气缸控制
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void ctr_pump_decode(u8 *buff, u16 len)
{
    u16 pumpAdr;
    u8 temp, pump;

    u8 sendToServer[32] = {0};
    cmd_send_style *dowm_data = NULL;   //上位机下发数据
    cmd_send_style *up_data = NULL;     //回复数据

    dowm_data = (cmd_send_style *)buff;
    up_data = (cmd_send_style *)sendToServer;

    memcpy(&up_data->start, &dowm_data->start, 6);
    up_data->dataLen = htons(8);

    if(check_sum(buff,len) != TRUE)
    {
        return;
    }
    pumpAdr = buff[3] << 8 | buff[4];
    //pump  = buff[3];
    temp = buff[6];

    printf("pumpAdr = %x,temp = %x\r\n",pumpAdr, pump, temp);
    if(pumpAdr == 1)
    {
        if(temp == 0xff)  //闭合
        {
            //io_air_pump1_off();  //
            system("echo 1 > /sys/class/gpio/ry1/value");
        }
        else //断开
        {
            //io_air_pump1_on();
            system("echo 0 > /sys/class/gpio/ry1/value");
        }
    }
    else if(pumpAdr == 2)
    {
        if(temp == 0xff)  //闭合
        {
            //io_air_pump2_off();  //开
            system("echo 1 > /sys/class/gpio/ry2/value");
        }
        else  //断开
        {
            //io_air_pump2_on();
           system("echo 0 > /sys/class/gpio/ry2/value");
        }
    }

    udp_send(sendToServer,8); //按源数据返回应答数据
}

/**************************************************
* name     : read_bat_enabling_decode
* funtion  : 读取电池使能状态
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void read_bat_enabling_decode(u8 *buff, u16 len)
{
   /*
    u8 module, offset, ch, group;

	offset = 0;

    if(check_sum(buff,len) != TRUE)
    {
        return;
    }

    group = buff[3];

	for(module = group*(GROUP_MODULE_NUM/2); module < (group+1)*(GROUP_MODULE_NUM/2); module++)
	{
		for(ch = 0;ch < MODULE_CH_NUM;ch++)
		{
			buff[offset]=N.NPS_info[module].node_info[ch] & 0x01;
			offset++;
		}
	}
	for(ch = 0;ch < MODULE_CH_NUM;ch++)
	{
		buff[offset]=0;
		offset++;
	}
    udp_send(buff,len); //按源数据返回应答数据
    */
}

/**************************************************
* name     : ctr_len_decode
* funtion  : led 指示灯控制， 分容的时候控制
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void ctr_len_decode(u8 *buff, u16 len)
{
    u8 i, j;
    u8 div_mun[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    u8 led_status[512] = {0};

    u8 sendToServer[32] = {0};
    cmd_send_style *dowm_data = NULL;   //上位机下发数据
    cmd_send_style *up_data = NULL;     //回复数据

    dowm_data = (cmd_send_style *)buff;
    up_data = (cmd_send_style *)sendToServer;

    memcpy(&up_data->start, &dowm_data->start, 6);
    up_data->dataLen = htons(8);

    if(check_sum(buff,len) != TRUE)
    {
        #if NET_DEBUG_CONFIG > 0
        printf("check_sum ctr_len_decode error!\r\n");
        #endif
        return;
    }

    for(i = 0; i < 64; i++){
        #if NET_DEBUG_CONFIG > 0
       // printf("buff[%d] = %d\r\n", i, buff[6 + i]);
        #endif
        for(j = 0; j < 8; j++){
            if(((buff[6 + i] & div_mun[j]) != 0)){
                led_status[j + 8 * i] = 1;
            }else{
                led_status[j + 8 * i] = 0;
            }
            //printf("led_status[j + 8 * i] = %d  ", led_status[j + 8 * i]);
        }
      //  printf("\r\n");
    }

    for(j = 0; j < MAX_NODE; j++){
        bst.node[j].run_status = SYS_DIVCAP;
        bst.node[j].div_cap_led = led_status[j];
       // printf("%d  ", bst.node[j].div_cap_led);
    }
  //  printf("\r\n");

    udp_send(sendToServer, 8);  //去掉数据段返回

  /*
    u8 group_cnt;
    tsSVR_CTR_STD_FH  *temp_downlink = NULL;     //下行数据指针
    //tsSVR_CTR_STD_FH  *temp_uplink = NULL;     //shang行数据指针

    temp_downlink = (tsSVR_CTR_STD_FH *)buff;

    if(check_sum(buff,len) != TRUE)
    {
        return;
    }

    udp_send(buff, 5);
 //   memcpy(&temp_uplink->start,&temp_downlink->start,4);
    for(group_cnt = 0 ; group_cnt < GROUP_NUM2 ; group_cnt++)
    {
        if(Global_info[group_cnt].run_status != SYS_START &&\
            Global_info[group_cnt].run_status != SYS_STOP &&\
            Global_info[group_cnt].run_status != SYS_RUNNING){
                Global_info[group_cnt].run_status = SYS_DIVCAP;	//切换到分容模式
        }
    }

    //更新led 控制变量
    update_divide_capacity_led(&temp_downlink->data,128);
  //  udp_send(udp_send_buff,(sizeof(tsSVR_CTR_STD_FH) + CRC_CODE_LEN - 1));
  */
}

/**************************************************
* name     : ctrol_node_status
* funtion  : 控制节点状态
* parameter：
* return   :
***************************************************/
void ctrol_node_status(u8 status, int group_cnt)
{
    u8 status_tmp;

    if(status == 0xFF){    //停止
        if(bst.node[group_cnt].run_status == SYS_RUNNING ||  bst.node[group_cnt].run_status == SYS_PAUSE)
        {
            bst.node[group_cnt].run_status = SYS_STOP;		//需要停止
            open_sqlite_db();
            start_commit();
            delete_bat_runningdata((group_cnt));
            stop_commit();
            close_sqlite_db();
        }
    }else if(status == 0x00){  //启动
        if( bst.node[group_cnt].run_status == SYS_IDLE || \
            bst.node[group_cnt].run_status == SYS_COMP || \
            bst.node[group_cnt].run_status == SYS_DIVCAP || \
            bst.node[group_cnt].run_status == SYS_PAUSE)	//空闲状态
        {
            status_tmp  = read_stage_state(group_cnt);
            if(bst.node[group_cnt].run_status == SYS_PAUSE){    //如果上一步为暂停，继续开始测试时候重新获取当前秒数
                bst.node[group_cnt].time_tmp_1 = get_sys_running_time_seconds();
            }
            if( status_tmp != STAGE_CHARGE && \
                status_tmp != STAGE_DISCHARGE && \
                status_tmp != STAGE_RESET && \
                status_tmp != STAGE_WORKING)
            {
                bst.node[group_cnt].run_status = SYS_START;  //进入启动阶段
            }
        }
    }else if(status == 0xFD){  //暂停
        if( bst.node[group_cnt].run_status == SYS_DIVCAP || \
            bst.node[group_cnt].run_status == SYS_RUNNING)	//空闲状态
        {
            status_tmp  = read_stage_state(group_cnt);
            if( status_tmp == STAGE_CHARGE || \
                status_tmp == STAGE_DISCHARGE || \
                status_tmp == STAGE_RESET || \
                status_tmp == STAGE_WORKING)
            {
                bst.node[group_cnt].run_status = SYS_PAUSE;  //进入暂停阶段
            }
        }
    }else if(status == 0xFE){  //恢复
        if(bst.node[group_cnt].run_status == SYS_PAUSE || read_stage_state(group_cnt) == STAGE_PAUSE){
            bst.node[group_cnt].run_status = SYS_RUNNING;
            bst.node[group_cnt].time_tmp_1 = get_sys_running_time_seconds();
        }
    }
}

/**************************************************
* name     : start_or_stop_decode
* funtion  : 启动/停止 控制
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void start_or_stop_decode(u8 *buff, u16 len)
{
    u8 status_tmp;
    u16 i;
	u16  group_cnt = 0;

    u8 sendToServer[32] = {0};
    cmd_send_style *dowm_data = NULL;   //上位机下发数据
    cmd_send_style *up_data = NULL;     //回复数据

    dowm_data = (cmd_send_style *)buff;
    up_data = (cmd_send_style *)sendToServer;

    memcpy(&up_data->start, &dowm_data->start, 6);
    up_data->dataLen = htons(8);

    if(check_sum(buff,len) != TRUE)
    {
        #if NET_DEBUG_CONFIG > 0
        printf("start_or_stop_decode check error\r\n");
        #endif
        return;
    }

    group_cnt = (buff[3] << 8 | buff[4]);
    printf("group_cnt = %x\r\n", group_cnt);
    udp_send(sendToServer,8);

    if(group_cnt == 0xffff){
        if(buff[6] == 0xFF){    //停止
            for(i = 0;i < MAX_NODE; i++){
                if(bst.node[i].run_status == SYS_RUNNING ||  bst.node[i].run_status == SYS_PAUSE)
                {
                    bst.node[i].run_status = SYS_STOP;		//需要停止
                }
            }
            sem_post(&delete_data_sem);
        }else if(buff[6] == 0x00){  //启动
            for(i = 0;i < MAX_NODE; i++){
                if( bst.node[i].run_status == SYS_IDLE || \
                    bst.node[i].run_status == SYS_COMP || \
                    bst.node[i].run_status == SYS_DIVCAP || \
                    bst.node[i].run_status == SYS_PAUSE)	//空闲状态
                {
                    status_tmp  = read_stage_state(i);
                    if(bst.node[i].run_status == SYS_PAUSE){    //如果上一步为暂停，继续开始测试时候重新获取当前秒数
                        bst.node[i].time_tmp_1 = get_sys_running_time_seconds();
                    }
                    if( status_tmp != STAGE_CHARGE && \
                        status_tmp != STAGE_DISCHARGE && \
                        status_tmp != STAGE_RESET && \
                        status_tmp != STAGE_WORKING)
                    {
                        bst.node[i].run_status = SYS_START;  //进入启动阶段
                    }
                }
            }
        }else if(buff[6] == 0xFD){  //暂停
            for(i = 0;i < MAX_NODE; i++){
                if( bst.node[i].run_status == SYS_DIVCAP || \
                    bst.node[i].run_status == SYS_RUNNING)	//空闲状态
                {
                    status_tmp  = read_stage_state(i);
                    if( status_tmp == STAGE_CHARGE || \
                        status_tmp == STAGE_DISCHARGE || \
                        status_tmp == STAGE_RESET || \
                        status_tmp == STAGE_WORKING)
                    {
                        bst.node[i].run_status = SYS_PAUSE;  //进入暂停阶段
                        bst.node[i].node_status = 5;
                    }
                }
            }
        }else if(buff[6] == 0xFE){  //恢复
            for(i = 0; i < MAX_NODE; i ++){
                if(bst.node[i].run_status == SYS_PAUSE || read_stage_state(i) == STAGE_PAUSE){
                    bst.node[i].run_status = SYS_RUNNING;
                    bst.node[i].time_tmp_1 = get_sys_running_time_seconds();
                }
            }
        }
        return;
    }

    if(group_cnt > MAX_NODE){
        return;
    }

    ctrol_node_status(buff[6], group_cnt);
   // udp_send(buff,9);
}

/**************************************************
* name     : cmp_divide_capacity_decode
* funtion  :  完成分容
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void cmp_divide_capacity_decode(u8 *buff, u16 len)
{
    /*
    u8 group;

    if(check_sum(buff,len) != TRUE)
    {
        return;
    }

    for(group = 0 ; group < GROUP_NUM2; group++)
    {
        if(Global_info[group].run_status  == SYS_DIVCAP)
        {
            Global_info[group].run_status = SYS_IDLE; //完成分容 则清除标志位
        }
    }

    udp_send( buff,len);//按源数据返回应答数据*/
}

/**************************************************
* name     : read_battery_em
* funtion  :  读取电池电能
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void read_battery_em(u8 *buff, u16 len)
{
    if(check_sum(buff,len) != TRUE)
    {
        return;
    }


    udp_send(buff,len); //按源数据返回应答数据
}

/**************************************************
* name     : clr_battery_err
* funtion  :  清除节点错误
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void clr_battery_err(u8 *buff, u16 len)
{
  /*
    u8 module, ch, group;

    if(check_sum(buff,len) != TRUE)
    {
        return;
    }

    group = buff[3];

    for(module = group* (GROUP_MODULE_NUM/2); module<(group+1)*(GROUP_MODULE_NUM/2); module++)
	{
		for(ch = 0; ch < MODULE_CH_NUM; ch++)
		{
			if(N.NPS_info[module].node_info[ch] & NODE_ERR_BIT)
			{
				N.NPS_info[module].node_info[ch]&=~NODE_ERR_BIT;
			}
		}
	}

    udp_send(buff,len); //按源数据返回应答数据*/
}

/**************************************************
* name     : set_step_global_decode
* funtion  : 工步全局配置下发
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void set_step_global_decode(u8 *buff, u16 len)
{
    u16  j;
    u16 bat_mun;
    tsCFG_STEP_CONFIG  *temp_step_data = NULL;

    u8 sendToServer[32] = {0};
    cmd_send_style *dowm_data = NULL;   //上位机下发数据
    cmd_send_style *up_data = NULL;     //回复数据

    dowm_data = (cmd_send_style *)buff;
    up_data = (cmd_send_style *)sendToServer;

    memcpy(&up_data->start, &dowm_data->start, 6);
    up_data->dataLen = htons(8);

    if(check_sum(buff,len) != TRUE)
    {
        #if NET_DEBUG_CONFIG > 0
        printf("check_sum error!\r\n");
        #endif
        return;
    }

    bat_mun = (buff[3] << 8 | buff[4]);

    printf("bat_mun = %d\r\n",bat_mun);

    if(bat_mun == 0xffff){
        for(j = 0; j < MAX_NODE; j++){
            temp_step_data =(tsCFG_STEP_CONFIG *)&(buff[6]);
            if(bst.node[j].run_status == SYS_RUNNING)      //
            {
                #if NET_DEBUG_CONFIG > 0
                printf("node is running, Please try again wait moment!\r\n");
                #endif
                continue;
            }

            bst.node[j].cyc                     = htons(temp_step_data->cyc);
            bst.node[j].platform_v              = htons(temp_step_data->platform_v);
            bst.node[j].test_style              = (temp_step_data->con1);
            bst.node[j].divcap_start_step       = (temp_step_data->con2);
            bst.node[j].divcap_end_step         = (temp_step_data->con3);
            bst.node[j].recharge_start_step     = (temp_step_data->con4);
            bst.node[j].recharge_end_step       = (temp_step_data->con5); //   x = (x << 8) & 0xFF00FF00FF | (x >> 8) & 0x00FF00FF00Ff;return x << 16 | x >> 16;
            bst.node[j].recharge_ratio          = (temp_step_data->con6 );

            temp_step_data++;
            udp_send(sendToServer,8); //按源数据返回应答数据
        }
        getstimeval();
        open_sqlite_db();
        start_commit();
        for(j = 0; j < MAX_NODE; j++){
            save_stepcfg_fun(j);
        }
        stop_commit();
        close_sqlite_db();
        getstimeval();

        return;
    }

    if(bat_mun > MAX_NODE){
        return;
    }

    temp_step_data =(tsCFG_STEP_CONFIG *)&(buff[6]);
    if(bst.node[bat_mun].run_status == SYS_RUNNING)      //
	{
        #if NET_DEBUG_CONFIG > 0
        printf("node is running, Please try again wait moment!\r\n");
        #endif
        return;
	}

    bst.node[bat_mun].cyc                     = htons(temp_step_data->cyc);
    bst.node[bat_mun].platform_v              = htons(temp_step_data->platform_v);
    bst.node[bat_mun].test_style              = (temp_step_data->con1);
    bst.node[bat_mun].divcap_start_step       = (temp_step_data->con2);
    bst.node[bat_mun].divcap_end_step         = (temp_step_data->con3);
    bst.node[bat_mun].recharge_start_step     = (temp_step_data->con4);
    bst.node[bat_mun].recharge_end_step       = (temp_step_data->con5); //   x = (x << 8) & 0xFF00FF00FF | (x >> 8) & 0x00FF00FF00Ff;return x << 16 | x >> 16;
    bst.node[bat_mun].recharge_ratio          = (temp_step_data->con6 );

    temp_step_data++;
    open_sqlite_db();
    save_stepcfg_fun(bat_mun);
    close_sqlite_db();
    udp_send(sendToServer,8); //按源数据返回应答数据
}

/**************************************************
* name     : read_step_global_decode
* funtion  : 工步全局配置读取
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void read_step_global_decode(u8 *buff, u16 len)
{
    u8 i;
    u16 group;
    read_global_step  *temp_downlink = NULL;     //下行数据指针
    read_global_step  *temp_uplink = NULL;       //上行数据指针
    tsCFG_STEP_CONFIG *temp_step_data = NULL;

    if(check_sum(buff,len) != TRUE)
    {
        #if NET_DEBUG_CONFIG > 0
        printf("clr_or_read_step_decode check error!\r\n");
        #endif
        return;
    }

    temp_downlink   =   (read_global_step *)buff;
    temp_uplink     =   (read_global_step *)udp_send_buff;
    temp_step_data  =   (tsCFG_STEP_CONFIG *)&temp_uplink->data;      //上传工步数据到起始地址
    memcpy(&temp_uplink->start,&temp_downlink->start,6);

    group = htons(temp_downlink->startOption);


    if(group > MAX_NODE - 1){
        return;
    }

    #if NET_DEBUG_CONFIG > 0
    printf("group = %d\r\n", group);
    #endif

    temp_step_data->cyc        = htons(bst.node[group].cyc);
    temp_step_data->platform_v = htons(bst.node[group].platform_v);
    temp_step_data->con1       = (bst.node[group].test_style);
    temp_step_data->con2       = (bst.node[group].divcap_start_step);
    temp_step_data->con3       = (bst.node[group].divcap_end_step);
    temp_step_data->con4       = (bst.node[group].recharge_start_step);
    temp_step_data->con5       = (bst.node[group].recharge_end_step);
    temp_step_data->con6       = (bst.node[group].recharge_ratio);

    temp_step_data++;

    temp_uplink->dataLen = htons(10 + 8);

    #if NET_DEBUG_CONFIG > 0
    printf("temp_uplink->dataLen = %d\r\n", htons(temp_uplink->dataLen));
    #endif

/*
    for(i = 0; i < temp_uplink->dataLen; i ++){
                printf("%x", udp_send_buff[i]);
                printf(" ");
    }
    printf("\r\n");*/

    udp_send(udp_send_buff,htons(temp_uplink->dataLen)); //按源数据返回应答数据
}

/**************************************************
* name     : recvice_server_decode
* funtion  : 接收后台数据解码函数
* parameter：buff-接收数据的地址指针， len- 接收到数据长度
* return   : NULL
***************************************************/
void recvice_server_decode(u8 *buff, u16 len)
{
    if(len < SOCK_REV_LEN)
    {
        if(buff[0] == SVR_START)  //起始位
        {
            switch(buff[5])        //命令字
            {
                case CFG_STEP:              //配置工步
                    cfg_step_decode(buff,len);
                break;

                case SYS_PARM:              //配置系统参数
                    cfg_sys_parm(buff, len);
                break;

                case CLR_OR_READ_STEP:      //读取或清除工步
                    clr_or_read_step_decode(buff,len);
                break;

                case READ_GROUP_RUN_DATA:   //读取组节点运行数据
                    read_group_run_data_decode(buff,len);
                break;

                case SIGNAL_BAT_CAP:      //下发单点容量
                    send_signal_bat_cap(buff, len);
                break;

                case READ_SINGAL_BAT_CAP:     //读取单点容量
                    read_bat_cap(buff, len);
                break;

                case NODE_ENABLE:       //节点使能
                    node_enable_decode(buff,len);
                break;

                case SET_STEP_GLOBAL:   //工步全局配置下发
                    set_step_global_decode(buff, len);
                break;

                case READ_STEP_GLOBAL:  //工步全局配置读取
                    read_step_global_decode(buff, len);
                break;

                case READ_STEP_STATUS:       //读取组工步状态
                    read_step_status_decode(buff,len);
                break;

                case SEARCH_HISTORY_DATA:   //查询节点是否存在历史数据
                    read_history_status_decode(buff,len);
                break;

                case READ_NODE_HIS_DATA:       //读取各组历史数据
                     read_history_data_decode(buff,len);
                break;

                case CTR_PUMP:              //气缸控制
                     ctr_pump_decode(buff,len);
                break;

                case READ_BAT_EN:            //读电池使能情况
                    read_bat_enabling_decode(buff,len);
                break;

                case CONTROL_LED:              //控制指示灯
                    ctr_len_decode(buff,len);
                break;

                case START_STOP_CMD:            //启动/停止控制
                    start_or_stop_decode(buff,len);
                break;

                case CMP_DIVIDE_CAP:              //完成分容
                    cmp_divide_capacity_decode(buff,len);
                break;

                case READ_BAT_EM:               //读电池组储能
                    read_battery_em(buff,len);   //读取电池
                break;

                case CLR_BAT_ERR:           //清除故障还原节点状态
                    clr_battery_err(buff,len);
                break;

                default:
                break;
            }
        }
    }

}

/**************************************************
* name     : handle_pipe
* funtion  : TCP发送失败响应函数
* parameter：
* return   :
***************************************************/
void handle_pipe(int sig)
{
    close(sock_fd);
    #if TCP_CONFIG > 0
    tcp_communication_init();                 //初始化udp网络通信
    #else
    udp_communication_init();
    #endif
}


/**************************************************
* name     : server_pthread_task
* funtion  :  后台服务 通信线程
* parameter：创建线程时需要传递到参数
* return   : NULL
***************************************************/
void server_pthread_task(void)
{
    u8 i;
    static netCount = 0;    //网络连接统计
    struct sigaction action;
    action.sa_handler = handle_pipe;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGPIPE, &action, NULL);
    sem_init(&save_step_sem, 0,0);
    #if TCP_CONFIG > 0
    tcp_communication_init();                 //初始化udp网络通信
    #else
    udp_communication_init();
    #endif
    while(1)
    {
        #if TCP_CONFIG > 0
        udp_rcv_num = recv(sock_fd, udp_rcv_buff, sizeof(udp_rcv_buff), 0);
        #else
        udp_rcv_num = recvfrom(sock_fd, udp_rcv_buff, sizeof(udp_rcv_buff), 0, (struct sockaddr*)&client_addr, (socklen_t *)&client_len);
        #endif
        if(udp_rcv_num > 0)
        {
            netCount = 0;
            bst.net_status = NET_ONLINE;
            #if NET_DEBUG_CONFIG > 0
            printf("udp_rcv_num = %d\r\n", udp_rcv_num);
            printf("data = ");
            for(i = 0; i < udp_rcv_num; i ++){
                printf("%x", udp_rcv_buff[i]);
                printf(" ");
            }
            printf("\r\n");
            #endif

            recvice_server_decode(udp_rcv_buff,udp_rcv_num);
        }else if(udp_rcv_num == -1){
            if(netCount > 30){
                if(bst.net_status == NET_ONLINE){
                    printf("net discount\r\n");
                    bst.net_status = NET_LEFT;
                }
                netCount = 0;
            }
            netCount++;
        }

        usleep(100);
    }
    close(sock_fd);
    exit(1);
}

/**************************************************
* name     : disconnect_udp
* funtion  : UDP断开函数
* parameter：
* return   :
***************************************************/
void disconnect_udp(void)
{
    close(sock_fd);
}
