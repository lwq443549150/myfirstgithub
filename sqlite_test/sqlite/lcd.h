/************************************************
*Copyright (C) ��˾����:�����о�ˮ�ʼҿƼ����޹�˾
*�ļ���     �� lcd.h
*�ļ���������: lcdͨѶͷ�ļ�
*������ʶ   ��xuchangwei 20170819
*�޸ı�ʶ   : 20170819 ���δ���
*�޸�����   : 20170819 ���δ���
***************************************************/

#ifndef __LCD_H_
#define __LCD_H_
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>  //�ź���
#include "sys_cfg.h"


typedef struct PAGE
{
	u8 (*poll_handler)(u32 poll_cnt); //ҳ����ѯ������
	void (*vchange_handler)(u8 offset, u32 data);  //
	u8 (*button_handler)(u8 offset);
	void (*page_init)();  //ҳ���ʼ��ҳ��
	void* wo;              //ֻд����
	void* ro;              //ֻ������
	u16 poll_time;   //��ѯʱ���� ��λms
	u8  wo_len;		 //ֻд��������
	u8  ro_len;		 //ֻ����������
	u8 prev_id;   //ǰһ��ҳ�� ���ڴӵ�ǰҳ�淵��ǰһ��
}page_t;


//DGUS ʱ��
typedef struct
{
	u8 YY;  //�� 2000 + YY
	u8 MM;  //��
	u8 DD;  //��
	u8 WW;  //��
	u8 h;   //ʱ
	u8 m;   //min
    u8 s;   //second
	}dgus_time_t;

//��Чҳ��
#define PAGE_ID_INVALID 0xff

#define PAGE_PREV 0xfe


extern  void    lcd_uart_signal_handler_IO(s32 status,siginfo_t *info,void *myact);     //����lcd���ڽ����жϺ���
extern  void    lcd_pthread_task(void);
extern  int     dgus_show_text(u16 addr, char *str);
extern  int     dgus_write_var(u16 addr, u8 len);
extern  void    dgus_fill_rec(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
extern  void    dgus_copy_rec(u16 page_id,u16 addr, u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3);

#endif

/***************end file*******************************/
