/************************************************
*Copyright (C) 公司名称:深圳市菊水皇家科技有限公司
*文件名     ：lcd.c
*文件功能描述: lcd通信应用程序
*创建标识   ：xuchangwei 20170819
//
*修改标识   :20170819 初次创建
*修改描述   :20170819 初次创建

***************************************************/

#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include <unistd.h>
#include <semaphore.h>  //信号量
#include "sys_sem.h"
#include "uart.h"
#include "lcd_ui.h"

#define DGUS_HEADER 0xA55A

#define CMD_W_REG 0x80
#define CMD_R_REG 0x81
#define CMD_W_VAR 0x82
#define CMD_R_VAR 0x83

#define LCD_EVT_RX      0x01        //收到数据事件
#define LCD_EVT_SWITCH  0x02        //页面切换事件
#define LCD_EVT_CRC     0x04        //屏幕CRC应答
#define LCD_RX_BUF_LEN  128

#define PAGE_ID(x)    ((u8)(x >> 8))
#define IS_BUTTON(x)  (x == 0xffff)

//DGUS屏通讯数据结构
typedef struct
{
	u16 header;     //帧头
	u8  f_lenth;    //帧长度
	u8  cmd;        //指令
	u8  data[1];    //数据
}dgus_fh;

typedef struct
{
	u16 var_addr;   //变量地址
	u8  data[1];    //数据
}var_w_fh;

typedef struct
{
	u16 var_addr;  //变量长度
	u8  data_len;  //数据长度
	u8  data[1];   //数据
}var_r_fh;

typedef struct
{
	u8  reg_addr;   //寄存器地址
	u8  data[1];    //数据
}rge_w_fh;

typedef struct
{
	u8  reg_addr;   //寄存器地址
	u8  data_len;   //数据长度
	u8  data[1];    //数据
}reg_r_fh;

typedef struct
{
	u16* var;                      //变量
	u8   var_lenth;                //var数组大小
}page_var_t;

typedef struct
{
  u8 data_err;      //数据错误,主要指数据丢失一部分,数据长度不够
  u16  rxin;
  u16  rxout;
  u8 buff[LCD_RX_BUF_LEN];
}tsLCD_UART;


u8 lcd_rx_buf[LCD_RX_BUF_LEN];  //串口接收缓存
u8 lcd_rx_len;

u8 dgus_buf[LCD_RX_BUF_LEN];
u8 dgus_buf_len;

//引进的变量或者函数
extern int      setsem_wait(sem_t *sem,int nano_sec,int sec);
extern page_t*  UI_data[];

//函数声明
static unsigned short   dgus_crc16(unsigned char* buf, unsigned short len);
static void             dgus_switch_page(u8 page_id);
static void             update_page (void);
static void             ui_process(var_r_fh* _fh);
       int              dgus_write_page(u8 page_id);

//局部变量定义
static  char volatile   lcd_uart_rev_flag = 0;
static  tsLCD_UART      lcd_uart;
static  sem_t           lcd_rx_sem,lcd_tx_sem,switch_page_sem;   //LCD接收 发送  校验信号量
static  sem_t           sem,lcd_crc_sem;       //信号定义 分别为:接收到数据 数据 发送完成 CRC校验成功
struct  timespec        ts;
        pthread_t       lcd_rev_thread;

static  u8              Current_page,switch_page_id;      //当前 显示的页面 ID号
        u8              lcd_rx_buf_temp[LCD_RX_BUF_LEN];
        u8              lcd_rx_signal_buf[LCD_RX_BUF_LEN];
        u8              send_buf[200] = {(u8)DGUS_HEADER, (u8)(DGUS_HEADER >> 8)};
static  u16             len_temp;
static  u32             page_poll_cnt;  //页面查询轮询次数
        u32             poll_time   = 0xffff;

static  int             cnt         = 0;
static  int             rev_len     = 0;


/**************************************************
* name      :  lcd_uart_signal_handler_IO
* funtion   :  连接lcd串口软中断函数,当串口接收到数据时,
               系统自动调用该函数
* parameter :  NULL
* return    :  NULL
***************************************************/
void lcd_uart_signal_handler_IO(s32 status,siginfo_t *info,void *myact)
{
//    if(info->si_code == POLL_OUT)
//    {
//        printf("send data ok!\n");
//    }
//    else  if(info->si_code == POLL_IN)
//    {
//        lcd_uart_rev_flag = 1;
//        rev_len = read(lcd_uart_dev.fd,lcd_rx_signal_buf,LCD_RX_BUF_LEN);
//        printf("rev data rev_len= %d ",rev_len);
//
//        for(cnt = 0; cnt < rev_len;cnt++)
//        {
//            printf("%x ",lcd_rx_signal_buf[cnt]);
//            //保存数据
//            len_temp = lcd_uart.rxin;
//            len_temp ++;
//            len_temp  = len_temp % LCD_RX_BUF_LEN;
//            if(len_temp != lcd_uart.rxout) //接收数据存储空间未满
//            {
//                lcd_uart.rxin = len_temp;
//                lcd_uart.buff[lcd_uart.rxin] = lcd_rx_signal_buf[cnt];
//            }
//        }
//        printf("\n");
//    }
}

/**************************************************
* name      :  lcd_usart_send
* funtion   :  发送  超时时间50ms
* parameter :  buf: 数据地址 lenth: 数据长度
* return    :  1 发送成功   0 失败
***************************************************/
static int lcd_usart_send(u8* buf, u8 lenth)
{
    if(lcd_uart_rev_flag)
    {
        usleep(10*1000);
        lcd_uart_rev_flag = 0;
    }
    write(lcd_uart_dev.fd,buf, lenth);
	return 0;
}

/**************************************************
* name      :  lcd_switch_to
* funtion   :  页面切换
* parameter :  page_id 页面地址ID
* return    :
***************************************************/
void lcd_switch_to(u8 page_id)
{
	if(page_id == Current_page)
	{
		return;
	}
	switch_page_id = page_id;
}

/**************************************************
* name      :  get_current_lcd_data_len
* funtion   :  获取lcd接收到到当前数据长度
* parameter :
* return    :  当前数据长度
***************************************************/
u16 get_current_lcd_data_len(void)
{
    u16 len;

    len = 0;
    if( lcd_uart.rxin > lcd_uart.rxout)
    {
        len = lcd_uart.rxin - lcd_uart.rxout;
    }
    else if( lcd_uart.rxin < lcd_uart.rxout)
    {
        len = LCD_RX_BUF_LEN - lcd_uart.rxout + lcd_uart.rxin;
    }

    return len;
}

/**************************************************
* name      :  lcd_uart_data_poll
* funtion   :  查询是否接收到完整的帧数据
* parameter :
* return    :   0 表示查询数据成功 -1 表示不成功
***************************************************/
int lcd_uart_data_poll(void)
{
    dgus_fh *fh;
    int ret;
    u16 rxout_next, rxout_next2, i, t, \
        lcd_head, len_cur, temp, out_temp;

    ret = 0;

    for( i = 0; i < LCD_RX_BUF_LEN; i++)
    {
        len_cur = get_current_lcd_data_len();
        if( len_cur < 7)
        {
            return -1;  //长度小于7 直接返回
        }

        out_temp =  lcd_uart.rxout;
        for( t = 0 ; t < len_cur;t++)
        {
            out_temp++;
            out_temp = out_temp %LCD_RX_BUF_LEN;
        }

        rxout_next  = (lcd_uart.rxout + 1) % LCD_RX_BUF_LEN;
        rxout_next2 = (rxout_next + 1) % LCD_RX_BUF_LEN;
        lcd_head    = lcd_uart.buff[rxout_next2];
        lcd_head    = (lcd_head << 8) + lcd_uart.buff[rxout_next];       //计算head

        if(lcd_head == DGUS_HEADER)  //查询数据帧头 0xA55A
        {
            temp = rxout_next;

            for(cnt = 0; cnt < len_cur; cnt++)
            {
                temp = temp % LCD_RX_BUF_LEN;
                lcd_rx_buf_temp[cnt] = lcd_uart.buff[temp++];       //将现有数据备份下来进行判断
            }

            fh = (dgus_fh*)lcd_rx_buf_temp;
            if((len_cur >= 7) && (fh->f_lenth == 2))  //CRC应答帧长为7,f_lenth长度固定为2
            {
                temp = *((u16*)&lcd_rx_buf_temp[5]);  //当前帧为CRC帧
                temp = htons(temp);
                if((temp == dgus_crc16(&lcd_rx_buf_temp[3], 2)) && (lcd_rx_buf_temp[4] == 0xff))  //返回正确的CRC校验应答 说明上次发送到数据正确接收
                {
                    lcd_uart.rxout = lcd_uart.rxout + 7;
                    lcd_uart.rxout = lcd_uart.rxout % LCD_RX_BUF_LEN;
                    sem_post(&lcd_crc_sem);
                }
                else
                {
                    //校验不过
                    lcd_uart.rxout++;
                    lcd_uart.rxout = lcd_uart.rxout % LCD_RX_BUF_LEN;
                }
            }
            else
            {
                //其他帧
                if(len_cur >= (fh->f_lenth + 3))
                {
                     //对其他数据帧进行校验
                    if(dgus_crc16(&fh->cmd, fh->f_lenth - 2) != htons(*(u16*)&fh->data[fh->f_lenth -3]))  //CRC错误
                    {
                        //校验不过
                        lcd_uart.rxout++;
                        lcd_uart.rxout = lcd_uart.rxout % LCD_RX_BUF_LEN;
                    }
                    else
                    {
                        //校验通过
                        lcd_rx_len = fh->f_lenth + 3;
                        if(lcd_rx_len < LCD_RX_BUF_LEN)
                        {
                            memcpy(lcd_rx_buf,lcd_rx_buf_temp,lcd_rx_len);
                            lcd_uart.rxout = lcd_uart.rxout + lcd_rx_len;
                            lcd_uart.rxout = lcd_uart.rxout % LCD_RX_BUF_LEN;      //更新指针
                            sem_post(&lcd_rx_sem);  //isr_evt_set(LCD_EVT_RX, lcd_task_id);
                        }
                    }
                }
                else
                {
                    lcd_uart.data_err++;
                    if(lcd_uart.data_err > 5)
                    {
                        //次数超过5,认为当前的数据无效,清理计数器
                        lcd_uart.data_err = 0;
                        lcd_uart.rxin = 0;
                        lcd_uart.rxout = 0;
                    }
                    break;
                }
            }
        }
        else
        {
            lcd_uart.rxout++;
            lcd_uart.rxout = lcd_uart.rxout % LCD_RX_BUF_LEN;
        }
    }
    return ret;
}

/**************************************************
* name      :  lcd_rev_pthread_task
* funtion   :  lcd 通信线程
* parameter :
* return    :
***************************************************/
void lcd_rev_pthread_task(void)
{
    while(1)
    {

        rev_len = read(lcd_uart_dev.fd,lcd_rx_signal_buf,LCD_RX_BUF_LEN);
        if(rev_len >0)
        {
            lcd_uart_rev_flag = 1;
            // printf("rev data rev_len= %d ",rev_len);
                for(cnt = 0; cnt < rev_len;cnt++)
                {
                  //  printf("%x ",lcd_rx_signal_buf[cnt]);
                    //保存数据
                    len_temp = lcd_uart.rxin;
                    len_temp ++;
                    len_temp  = len_temp % LCD_RX_BUF_LEN;
                    if(len_temp != lcd_uart.rxout) //接收数据存储空间未满
                    {
                        lcd_uart.rxin = len_temp;
                        lcd_uart.buff[lcd_uart.rxin] = lcd_rx_signal_buf[cnt];
                    }
                }
             //   printf("\n");
        }
        lcd_uart_data_poll();
        usleep(1*1000);
    }
}

/**************************************************
* name     : lcd_pthread_task
* funtion  : lcd 通信线程
* parameter：创建线程时需要传递到参数
* return   : NULL
***************************************************/
void lcd_pthread_task(void)
{
    var_r_fh    *_var_r_fh;
    dgus_fh     *fh         = (dgus_fh*)dgus_buf;

    lcd_uart.data_err   = 0;
    lcd_uart.rxin       = 0;
    lcd_uart.rxout      = 0;

	sem_init(&lcd_rx_sem,0, 0);
	sem_init(&lcd_tx_sem, 0,1);
	sem_init(&switch_page_sem, 0,0);
    sem_init(&lcd_crc_sem, 0,0);
    sem_init(&sem, 0,0);

    pthread_create(&lcd_rev_thread,NULL,(void *(*)(void *))lcd_rev_pthread_task,NULL);  //lcd 通信线程
	usleep(10*1000);
	poll_time = UI_data[0]->poll_time;  //初始化LOGO显示
    dgus_switch_page(0);
    usleep(1*1000);

    while(1)
    {
        usleep(1*1000);
        if(sem_trywait(&switch_page_sem) == 0)  //查询是否需要强行更新页面
        {
            dgus_switch_page(switch_page_id);   //切换指定页面
        }
		else if(setsem_wait(&lcd_rx_sem,100*1000000,0) == 0)    // 接收到数据
		{
            if(lcd_rx_len == 0)
            {
                continue;
            }

            memcpy(dgus_buf, lcd_rx_buf, lcd_rx_len);  //保存数据
            lcd_rx_len = 0;
            memset(lcd_rx_buf,0,lcd_rx_len);

            if(dgus_crc16(&fh->cmd, fh->f_lenth - 2) != htons(*(u16*)&fh->data[fh->f_lenth -3]))  //CRC错误
            {
                continue;
            }
            _var_r_fh = (var_r_fh*)fh->data;
            _var_r_fh->var_addr = htons(_var_r_fh->var_addr);  //
            ui_process(_var_r_fh);	//处理数据
		}
		else  //无操作
		{
            update_page();  //刷新显示
		}
    }
}

/**************************************************
* name      :  dgus_get_current_page_id
* funtion   :  获取当前ID号
* parameter :
* return    :  当前ID号
***************************************************/
u8 dgus_get_current_page_id()
{
	return Current_page;
}

/**************************************************
* name      :   dgus_read_addr
* funtion   :   读地址的数据
* parameter :   addr 地址 buf 数据 len 长度
* return    :   成功返回 1  失败返回 0
***************************************************/
int dgus_read_addr(u16 addr, u8* buf, u16 len)
{
    u8          err_cnt = 3;
	u16         temp;
	dgus_fh     *data_fh;
	var_r_fh    *var_fh;

	send_buf[2]         = 6;
	send_buf[3]         = CMD_R_VAR;
	send_buf[4]         = addr >> 8;
	send_buf[5]         = (u16)addr;
	send_buf[6]         = len;
	temp                = dgus_crc16(send_buf + 3, 4);
    *(u16*)&send_buf[7] = htons(temp);

	do
	{
        lcd_usart_send(send_buf, 9);
        if((setsem_wait(&lcd_crc_sem, 80*1000000, 0) == 0) && (setsem_wait(&lcd_rx_sem, 80*1000000, 0) == 0))
		{
			//这里返回的数据包括两个数据帧  第一个为发送给屏的读命令的CRC应答帧   后面一帧为变量数据帧
			//当有数据返回时说明发送的命令是正确的 可以把CRC应答帧忽略  只处理后面的数据帧
			data_fh = (dgus_fh*)&lcd_rx_buf[0];  //忽略CRC应答帧   直接读数据帧
			temp    = *((u16*)&data_fh->data[data_fh->f_lenth - 3]);  //数据帧CRC
			temp    = htons(temp);

			if(temp == dgus_crc16(&data_fh->cmd, data_fh->f_lenth - 2))  //返回正确的CRC校验应答  说明上次发送的数据正确接收
			{
				var_fh = (var_r_fh*)data_fh->data;
				memcpy(buf, var_fh->data, len * 2); //保存变量数据
                lcd_rx_len = 0;
                memset(lcd_rx_buf, 0, lcd_rx_len);
				return 1;
			}
		}
		 usleep(5 * 1000);
	}while(--err_cnt);
	return 0;
}

/**************************************************
* name      :   dgus_read_addr
* funtion   :   读地址的数据
* parameter :   addr 地址 buf 数据 len 长度
* return    :   成功返回 1  失败返回 0
***************************************************/
static int dgus_read_reg(u8 reg, u8 *buf, u8 len)
{
    u8          err_cnt = 3;
	u16         crc;
    dgus_fh     *data_fh;

    send_buf[2]         = 5;    //长度
	send_buf[3]         = CMD_R_REG;
	send_buf[4]         = reg;  //地址
	send_buf[5]         = len;
	crc                 = dgus_crc16(&send_buf[3], 3);  //计算CRC
	*(u16*)&send_buf[6] = htons(crc);

	do
	{
		lcd_usart_send(send_buf, 8);
		if((setsem_wait(&lcd_crc_sem,200*1000000,0) == 0) && (setsem_wait(&lcd_rx_sem,200*1000000,0) == 0))
		{
            //这里返回的数据包括两个数据帧  第一个为发送给屏的读命令的CRC应答帧   后面一帧为数据帧
			//当有数据返回时说明发送的命令是正确的 可以把CRC应答帧忽略  只处理后面的数据帧
			data_fh         = (dgus_fh*)&lcd_rx_buf[0];  //忽略CRC应答帧   直接读数据帧
			reg_r_fh *temp  = (reg_r_fh*)&lcd_rx_buf[4];
			crc             = *(u16*)&lcd_rx_buf[lcd_rx_len - 2];
			crc             = htons(crc);

			if(crc == dgus_crc16(&data_fh->cmd, data_fh->f_lenth - 2))
			{
				if(temp->reg_addr == reg)
				{
					memcpy(buf, temp->data, len); //保存变量数据
					memset(lcd_rx_buf,0,lcd_rx_len);
					lcd_rx_len = 0;
					return 1;
				}
			}
		}
		 usleep(5 * 1000);
	}while(--err_cnt);

	return 0;
}

/**************************************************
* name      :   dgus_read_addr
* funtion   :   写地址的数据
* parameter :   addr 地址 buf 数据 len 长度
* return    :   成功返回 1  失败返回 0
***************************************************/
static int dgus_write_reg(u8 reg, u8 *buf, u8 len)
{
    u8  err_cnt = 3;
	u16 crc;


	send_buf[2] = 2 + len + 2;  //长度
	send_buf[3] = CMD_W_REG;
	send_buf[4] = reg;  //地址
	memcpy(&send_buf[5], buf, len);
	crc                         = dgus_crc16(&send_buf[3], 2 + len);  //CRC校验
	*(u16*)&send_buf[5 + len]   = htons(crc);

	do
	{
		lcd_usart_send(send_buf, 7 + len);
		if(setsem_wait(&lcd_crc_sem,80*1000000,0) == 0)
        {
            return 1;
        }
         usleep(5 * 1000);
	}while(--err_cnt);
	return 0;
}

/**************************************************
* name      :   dgus_switch_page
* funtion   :   切换到指定页面
* parameter :   page_id 页面ID号
* return    :
***************************************************/
void dgus_switch_page(u8 page_id)
{
	page_t* next_page;
	u16     temp = htons(page_id);

	page_poll_cnt   = 0;  //当前轮询次数清0
	next_page       = UI_data[page_id];

    if(next_page == NULL)
    {
        return;
    }
	next_page->prev_id = Current_page;  //保存前一级到页面id号
	if(next_page->page_init != NULL)  //初始化显示数据
	{
		next_page->page_init();
	}
	if(next_page->wo)  //切换之前先更新页面数据
	{
		dgus_write_page(page_id);
	}

	if(dgus_write_reg(0x03, (u8*)&temp, 2))  //切换成功
	{
		Current_page = page_id;
	}

	if(next_page->poll_handler != NULL)  //若页面定义了刷新事件
	{
		poll_time = next_page->poll_time;  //刷新时间
        if(poll_time == 0)
		{
			poll_time = 0xffff;  //不刷新
		}
	}
	else
	{
		poll_time = 0xffff;  //不刷新
	}
}

/**************************************************
* name      :   dgus_trigger_key
* funtion   :   触发一个建码
* parameter :   key 建码
* return    :
***************************************************/
void dgus_trigger_key(u8 key)
{
	u8 temp[1];
	temp[0] = key;

	dgus_write_reg(0x4F, temp, 1);
}

/**************************************************
* name      :   dgus_show_text
* funtion   :   显示文本
* parameter :   addr 变量地址 code 字符串 中文为GBK
* return    :   1 成功 0 失败
***************************************************/
int dgus_show_text(u16 addr,char *str)
{
    u8  err_cnt = 3;
	u8  len     = strlen((const char*)str);
	u16 crc;

	send_buf[2] = 5 + len + 2;  //在字符串后面加0x0000
	send_buf[3] = CMD_W_VAR;
	send_buf[4] = (u8)(addr >> 8);  //变量地址
	send_buf[5] = (u8)addr;

	memcpy(send_buf + 6, str, len);

	//在字符串前面0x0000
	send_buf[6 + len]           =   0;
	send_buf[6 + len + 1]       =   0;
	len                         +=  2;
	crc                         =   dgus_crc16(send_buf + 3, len + 3);
	crc                         =   htons(crc);
	*(u16*)&send_buf[len + 6]   =   crc;  //填充CRC

	do
	{
		lcd_usart_send(send_buf, len + 8);  //发送数据
		if(setsem_wait(&lcd_crc_sem, 80*1000000, 0) == 0)
		{
			return 1;
		}
		 usleep(5 * 1000);
	}while(--err_cnt);

	return 0;
}

/**************************************************
* name      :   dgus_write_page
* funtion   :   把页面的只写变量数据发到DGUS屏中
* parameter :   page_id  页面编号
* return    :   1 成功 0 失败
***************************************************/
int dgus_write_page(u8 page_id)
{
    u8      err_cnt = 3;
	u16     crc;
	page_t* page    = UI_data[page_id];

    if(page == NULL)
    {
        return 0;
    }

	send_buf[2] = 5 + page->wo_len;
	send_buf[3] = CMD_W_VAR;
	send_buf[4] = page_id;  //页面ID
	send_buf[5] = 0;

	memcpy(send_buf + 6, page->wo, page->wo_len);
	crc = dgus_crc16(send_buf + 3, page->wo_len + 3);
	crc = htons(crc);
	*(u16*)&send_buf[page->wo_len + 6] = crc;  //填充CRC

	do
	{
		lcd_usart_send(send_buf, page->wo_len + 8);  //发送数据
		if(setsem_wait(&lcd_crc_sem, 80*1000000, 0) == 0)
        {
            return 1;
        }
         usleep(5 * 1000);
	}while(--err_cnt);

	return 0;
}

/**************************************************
* name      :   dgus_init_ro_var
* funtion   :   初始化页面的只读变量数据 显示默认参数
* parameter :   page_id  页面编号
* return    :   1 成功 0 失败
***************************************************/
int dgus_init_ro_var(u8 page_id)
{
    u8      err_cnt = 3;
	u16     crc;
	page_t* page    = UI_data[page_id];

    if(page == NULL)
    {
        return 0;
    }

	send_buf[2] = 5 + page->ro_len;
	send_buf[3] = CMD_W_VAR;
	send_buf[4] = page_id;  //页面ID
	send_buf[5] = 0x80;     //只读

	memcpy(send_buf + 6, page->ro, page->ro_len);
	crc = dgus_crc16(send_buf + 3, page->ro_len + 3);
	crc = htons(crc);
	*(u16*)&send_buf[page->ro_len + 6] = crc;

	do
	{
		lcd_usart_send(send_buf, page->ro_len + 8);
        if(setsem_wait(&lcd_crc_sem, 80*1000000, 0) == 0)
        {
            return 1;
        }
         usleep(5 * 1000);
	}while(--err_cnt);

	return 0;
}

/**************************************************
* name      :   dgus_write_current_page
* funtion   :   刷新当前页面变量显示
* parameter :   page_id  页面编号
* return    :
***************************************************/
void dgus_write_current_page(void)
{
	dgus_write_page(Current_page);
}

/**************************************************
* name      :   dgus_read_page
* funtion   :   从DGUS屏中读取变量数据
* parameter :   page_id  页面编号
* return    :
***************************************************/
int dgus_read_page(u8 page_id)
{
    u8          err_cnt = 3;
	u16         temp;
	dgus_fh     *data_fh;
	var_r_fh    *var_fh;
	page_t      *page = UI_data[page_id];

    if(page == NULL)
    {
        return 0;
    }

	send_buf[2] = 6;
	send_buf[3] = CMD_R_VAR;
	send_buf[4] = page_id;
	send_buf[5] = 0x80;  //只读
	send_buf[6] = page->ro_len / 2;
	temp        = dgus_crc16(send_buf + 3, 4);
	*(u16*)&send_buf[7] = htons(temp);

	do
	{
		lcd_usart_send(send_buf, 9);
        if(setsem_wait(&lcd_rx_sem,50*1000000,0) == 0)
		{
			//这里返回的数据包括两个数据帧 第一个为发送给屏的读命令的CRC应答帧 后一帧为变量数据帧
			//当有数据返回时 说明发送的命令时正的 可以把CRC应答帧忽略 只处理后面的数据帧
			data_fh = (dgus_fh*)&lcd_rx_buf[0];  //忽略CRC应答帧 直接读取数据帧

            temp = *((u16*)&data_fh->data[data_fh->f_lenth - 3]);  //数据帧CRC
			temp = htons(temp);
			if(data_fh->f_lenth>2)
			{
				if(temp == dgus_crc16(&data_fh->cmd, data_fh->f_lenth - 2))  //返回正确的CRC校验应答 说明上次发送的数据正确接收
				{
					var_fh = (var_r_fh*)data_fh->data;
					memcpy(page->ro, var_fh->data, page->ro_len); //保存变量数据
					memset(lcd_rx_buf,0,lcd_rx_len);
					lcd_rx_len = 0;
					return 1;
				}
			}
		}
        usleep(50*1000);
	}while(--err_cnt);

	return 0;
}

/**************************************************
* name      :   dgus_write_var
* funtion   :   写变量地址
* parameter :   addr 地址 len 长度
* return    :   成功返回 1 失败返回 0
***************************************************/
int dgus_write_var(u16 addr, u8 len)
{
//    int i;
    u8  err_cnt = 3;
	u16 crc;

	send_buf[2] = 5 + len;
	send_buf[3] = CMD_W_VAR;
	send_buf[4] = (u8)(addr >> 8);
	send_buf[5] = (u8)addr;

	crc = dgus_crc16(send_buf + 3, len + 3);
	crc = htons(crc);
	*(u16*)&send_buf[len + 6] = crc;  //填充CRC
/*
	for(i = 0; i < len + 8; i++){
        printf("%x  ", send_buf[i]);
	}
	printf("\r\n");
*/
	do
	{
		lcd_usart_send(send_buf, len + 8);  //发送数据
        if(setsem_wait(&lcd_crc_sem,80*1000000,0) == 0)
        {
            return 1;
        }
         usleep(5 * 1000);
	}while(--err_cnt);

	return 0;
}

/**************************************************
* name      :   dgus_fill_rec
* funtion   :   填充一个矩形
* parameter :   x1 y1 左上角坐标, x2 y2 右下角坐标
                color  填充颜色
* return    :
***************************************************/
void dgus_fill_rec(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	send_buf[6] = 0x00;  // 矩形填充指令
	send_buf[7] = 0x04;

	send_buf[8] = 0x00;  //填充一个矩形
	send_buf[9] = 0x01;

	send_buf[10] = (u8)(x1 >> 8);  //左上角坐标x
	send_buf[11] = (u8)x1;
	send_buf[12] = (u8)(y1 >> 8);  //右上角坐标y
	send_buf[13] = (u8)y1;

	send_buf[14] = (u8)(x2 >> 8);  //左下角坐标x
	send_buf[15] = (u8)x2;
	send_buf[16] = (u8)(y2 >> 8);  //右下角坐标y
	send_buf[17] = (u8)y2;

	send_buf[18] = (u8)(color >> 8);  //填充颜色
	send_buf[19] = (u8)color;

	send_buf[20] = 0xff;  //指令结束
	send_buf[21] = 0x00;

	dgus_write_var(0x0020, 16);
}

/**************************************************
* name      :   dgus_copy_rec
* funtion   :   复制一个矩形
* parameter :   x1 y1 复制左上角坐标, x2 y2 复制右上角坐标
                x3 y3 粘贴左上角坐标
* return    :
***************************************************/
void dgus_copy_rec(u16 page_id,u16 addr, u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3)
{
	send_buf[6] = 0x00;  //// 矩形填充指令
	send_buf[7] = 0x06;

	send_buf[8] = 0x00; //填充一个矩形
	send_buf[9] = 0x01;

	send_buf[10] = (u8)(page_id>>8);  //复制的图片所在的页面
	send_buf[11] = (u8)page_id;

	send_buf[12] = (u8)(x1 >> 8);  //左上角坐标X
	send_buf[13] = (u8)x1;
	send_buf[14] = (u8)(y1 >> 8);  //左上角坐标Y
	send_buf[15] = (u8)y1;

	send_buf[16] = (u8)(x2 >> 8);  //右上角坐标X
	send_buf[17] = (u8)x2;
	send_buf[18] = (u8)(y2 >> 8);  //右上角坐标Y
	send_buf[19] = (u8)y2;

	send_buf[20] = (u8)(x3 >> 8);  //左上角坐标X
	send_buf[21] = (u8)x3;
	send_buf[22] = (u8)(y3 >> 8);  //左上角坐标Y
	send_buf[23] = (u8)y3;

	send_buf[24] = 0xff;  //指令结束
	send_buf[25] = 0x00;
	dgus_write_var(addr, 20);
}

/**************************************************
* name      :   dgus_bl_config
* funtion   :   屏幕亮度设置
* parameter :   bl  0 ~ 8级亮度
* return    :
***************************************************/
void dgus_bl_config(u8 bl)
{
	u8 temp = 19 + (bl + 1) * 5;

	dgus_write_reg(0x01, &temp, 1);
}

/**************************************************
* name      :   dec2hex
* funtion   :   十进制转十六进制 dec2hex(15) == 0x15
* parameter :   dec 十进制值
* return    :   返回对应的十六进制的值
***************************************************/
u8 dec2hex(u8 dec)
{
	u8 hex;
	hex = (dec % 10) | ((dec / 10) << 4);
	return hex;
}

/**************************************************
* name      :   dec2hex
* funtion   :   十六进制转十进制 hwx2dec(0x15) == 15
* parameter :   dec 十六进制值
* return    :   返回对应的十进制的值
***************************************************/
u8 hex2dec(u8 hex)
{
	u8 dec;
	dec = (hex >> 4) * 10 + (hex & 0x0f);
	return dec;
}

/**************************************************
* name      :   dgus_rtc_config
* funtion   :   设置RTC
* parameter :   YY 年, MM 月, DD 日, h 时, m 分
* return    :
***************************************************/
void dgus_rtc_config(u8 YY, u8 MM, u8 DD, u8 h, u8 m)
{
	u8 temp[8];

	temp[0] = 0x5A;
	temp[1] = dec2hex(YY);
	temp[2] = dec2hex(MM);
	temp[3] = dec2hex(DD);
	temp[4] = 0;
	temp[5] = dec2hex(h);
	temp[6] = dec2hex(m);
	temp[7] = 0;

	dgus_write_reg(0x1F, temp, 8);
}

/**************************************************
* name      :   dgus_rtc_read
* funtion   :   读取rtc
* parameter :   TIME_type 结构
* return    :   1:成功  0:失败
***************************************************/
int dgus_rtc_read(struct tm *time)
{
	dgus_time_t d_time;
	if(dgus_read_reg(0x20, (u8*)&d_time, 7))
	{
		time->tm_year = hex2dec(d_time.YY) + 2000;
		time->tm_mon  = hex2dec(d_time.MM);
		time->tm_mday = hex2dec(d_time.DD);
		time->tm_hour = hex2dec(d_time.h);
		time->tm_min  = hex2dec(d_time.m);
		time->tm_sec  = hex2dec(d_time.s);
		time->tm_wday = d_time.WW;

		printf("nian = %d,mon =%d,day=%d.h=%d,min =%d,second = %d \n",time->tm_year ,time->tm_mon,time->tm_mday,time->tm_hour,\
          time->tm_min,time->tm_sec);
		return 1;
	}

	return 0;
}

/**************************************************
* name      :   dgus_get_prev_id
* funtion   :   获取前一级页面ID 用于从当前页面返回前一级
* parameter :
* return    :   页面ID
***************************************************/
u8 dgus_get_prev_id()
{
	return UI_data[Current_page]->prev_id;
}

/*
 *  fh->var_addr DGUS屏数据变量地址定义
 *  当_fh->var_addr不为0xffff时_fh->var_addr表示如下:
 *  bit15 ~ bit8 (8bit) ---  page_id
 *  bit7 ~ bit0 (8bit) ----  变量在当前页面偏移  bit7为1时变量为只读  为0时为只写
 *
 *  当_fh->var_addr为0xffff时,表示为按键返回,_fh->data为按键码
 *  _fh->data[0]  为page id  _fh->data[1] 在当前页面偏移
 */
/**************************************************
* name      :   ui_process
* funtion   :   UI 处理
* parameter :   见上面分析
* return    :
***************************************************/
static void ui_process(var_r_fh* _fh)
{
	page_t *page;
	u8 next_page_id = PAGE_ID_INVALID;
    page = NULL;

	if(_fh->var_addr == 0xffff)  //无效地址
	{
		if(_fh->data[0] != Current_page)
		{
		    //TODO 检查page id
			//当接收到两个以上一样的按键命令时，由于处理第一个按键命令时页面可能已经切换，再处理第二个时导致_fh->data[0] != Current_page
			//也有可能是CPU重启了,而DGUS屏没有重启造成 Current_page 与真实显示的page id不相等
			//此时从DGUS屏中读取当前显示的page id

			u16 temp;
			if(dgus_read_reg(0x03, (u8*)&temp, 2))
			{
				Current_page = htons(temp);
			}
			return;
		}

		page = UI_data[Current_page];

        if(page != NULL)
        {
            if(page->button_handler != NULL)
            {
                next_page_id = page->button_handler(_fh->data[1]);  //调用相应页面的按键处理函数
            }
        }
		if(next_page_id == PAGE_ID_INVALID)
		{
			return;  //无需切换页面
		}
		dgus_switch_page(next_page_id);
	}
	else  //数据
	{
		 //  bit15 ~ bit8 (8bit) ---  page_id
		 //  bit7 ~ bit0 (8bit) ---- 变量在当前页面偏移 bit7为1时变量为只读.为0时为只写
		page = UI_data[_fh->var_addr >> 8];
		if(page != NULL)
		{
            if(page->vchange_handler != NULL)
            {
                u32 temp = *((u32*)&_fh->data[0]);
                printf("temp = %x\r\n", temp);
                page->vchange_handler(_fh->var_addr & 0x007f, temp);
            }
		}
	}
}

/**************************************************
* name      :   update_page
* funtion   :   页面刷新
* parameter :
* return    :
***************************************************/
void update_page (void)
{
	page_t* page;

	page = UI_data[Current_page];

    if(page != NULL)
    {
        if (page->poll_handler != NULL)
        {
            u8 next_page_id = page->poll_handler(page_poll_cnt);  //页面轮询处理函数

            page_poll_cnt++;  //每调用加1

            if(next_page_id == PAGE_ID_INVALID)
            {
                return;  //无需切换页面
            }
            else
            {
                dgus_switch_page(next_page_id);//切换到下一页
            }
        }
    }
}

//CRC校验查询数组
const unsigned char dgus_auchCRCHi[] =
{0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40};

const unsigned char dgus_auchCRCLo[] =
{0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04,
 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8,
 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10,
 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C,
 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0,
 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C,
 0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54,
 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98,
 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
 0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40};

/**************************************************
* name      :   dgus_crc16
* funtion   :   CRC校验
* parameter :   buf 待校验的数据 len 数据的长度
* return    :   校验码
***************************************************/
unsigned short dgus_crc16(unsigned char* buf, unsigned short len)
{
	unsigned char uchCRCHi = 0xff;
	unsigned char uchCRCLo = 0xff;
	unsigned uIndex;
	while(len--)
	{
		uIndex   = uchCRCHi ^ *buf++;
		uchCRCHi = uchCRCLo ^ dgus_auchCRCHi[uIndex];
		uchCRCLo = dgus_auchCRCLo[uIndex];
	}
	return (unsigned short)(uchCRCHi << 8 | uchCRCLo);
}

/*******************end file*************************/
