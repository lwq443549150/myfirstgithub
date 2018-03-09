/************************************************
*Copyright (C) ��˾����:�����о�ˮ�ʼҿƼ����޹�˾
*�ļ���     ��lcd_ui.c
*�ļ���������: LCD�������
*������ʶ   ��xuchangwei 20170819
//
*�޸ı�ʶ   :20170819 ���δ���
*�޸�����   :20170819 ���δ���

***************************************************/

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lcd.h"
#include "sys_cfg.h"
#include "server.h"
#include "lcd_ui.h"
#include "sys_logic_control.h"
#include "evc.h"
#include "sqlite_opera.h"


#define POLL_T 200
#define PAGE_SUM    (MAX_NODE / 16 - 1)

typedef struct{
	u16 set_v;
	u16 set_i;
	u8 mode;
}PAGE2_CTR_DEF;


extern u8   send_buf[200];
extern void read_module_ver(u8 opt);
extern int  dgus_read_page(u8 page_id);
extern int  dgus_write_page(u8 page_id);
extern void dgus_bl_config(u8 bl);
extern void dgus_write_current_page(void);
extern void dgus_rtc_config(u8 YY, u8 MM, u8 DD, u8 h, u8 m);
extern int  dgus_rtc_read(struct tm *time);
extern int  dgus_init_ro_var(u8 page_id);
extern void dgus_trigger_key(u8 key);
extern void dgus_fill_rec(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
extern void dgus_copy_rec(u16 page_id,u16 addr, u16 x1, u16 y1, u16 x2, u16 y2, u16 x3, u16 y3);
extern u8   dgus_get_prev_id(void);

static u8 page1_init_flag; // �����ʼ����־
static u8 module_cnt = 0;
static u8 switch_page_flag = 0;         //�л������־
static u8 page9_step_check_page;    //������Ϣ��ѯ�л���־
static u8 page6_group_select;           //��6ҳ��ѡ��
       u8 send_v[8], send_i[8], on_off[8];
       u8 read_single_page_num;    //��¼��ǰ�����ҳ��
       u8 page10_cnt_poll;         //��¼ʵʱ�鿴������ݵķ�ҳ��¼
       u8 opera_flag = 0;          //�ֶ���ŵ������־
static u16 page8_temp_low_volt,page8_temp_high_volt;    //ϵͳ��������
static u16 page8_temp_protect_low_v,page8_temp_protect_high_v;
static u16 page8_temp_protect_low_i,page8_temp_protect_high_i;
static u16 slip_mun;                    //�������ؽ����ֵ
static u16 passwad_cod[4]={1,9,9,3};    //���ؽ��������
       u16 stage[16];
static int step_group_select = 0;           //������ѡ��
       int page7_offset = 0;
       int flag = 1;

//������״̬
struct page1_wo_var_s
{
	u16 stage1;     //��1
	u16 stage2;     //��2
	u16 stage3;     //��3
	u16 stage4;     //��4
	u16 stage5;     //��5
	u16 stage6;     //��6
	u16 stage7;     //��7
	u16 stage8;     //��8
    u16 stage9;     //��9
	u16 stage10;    //��10
	u16 stage11;    //��11
	u16 stage12;    //��12
	u16 stage13;    //��13
	u16 stage14;    //��14
	u16 stage15;    //��15
	u16 stage16;    //��16
}page1_wo_var;

struct page2_ro_var_s
{
	u16 Set_Vlotage;
	u16 Set_Current;
}page2_ro_var;

PAGE2_CTR_DEF page2_ctr;    //����ʵʱ����ҳ��Ѳ�����ز���

struct page2_wo_var_s
{
    u16 v;              //unit: MV
    u16 i;              //unit: MA
    u16 capacity;       //charge capacity, unit mah
    u16 discapacity;    // discharge  capacity, unit:mah
    u16 time;           //unit: s(second)
    u16 status;
}page2_wo_var;

//IP��ַ����
struct page5_ro_var_s
{
    u32 id;

    u16 rip1;  //Զ��IP
    u16 rip2;
    u16 rip3;
    u16 rip4;

    u16 rport;  //Զ�̶˿�

    u16 lip1;  //����IP
    u16 lip2;
    u16 lip3;
    u16 lip4;

    u16 drip1;  //����IP
    u16 drip2;
    u16 drip3;
    u16 drip4;

    u16 netmask1;  //��������
    u16 netmask2;
    u16 netmask3;
    u16 netmask4;
}page5_ro_var;

//��ŵ�ģ����Ϣ
struct page7_wo_var_s	 //ģ����Ϣ
{
    u16 batNUM[16];
}page7_wo_var;

//ϵͳ��������
struct page8_wo_var_s
{
	u16 led_lighting_mode;		//��ѹ������� ģʽ 0��  1����
	u16 low_volt;				//�ϸ�͵�ѹ
	u16 high_volt;				//�ϸ�ߵ�ѹ
	u16 peotect_low_v;		    //��ͱ�����ѹ
	u16 peotect_high_v;         //��߱�����ѹ
	u16 peotect_low_i;          //��ͱ�������
	u16 peotect_high_i;         //��߱�������
}page8_wo_var;

struct page8_ro_var_s
{
	u16 led_lighting_mode;			//��ѹ������� ģʽ 0��  1����
	u16 low_volt;					//�ϸ�͵�ѹ
	u16 high_volt;					//�ϸ�ߵ�ѹ
}page8_ro_var;

//������ѯ
struct page9_wo_var_s
{
  struct{
		u16 num;				//������
		u16 type;
		u16 up_v;               //���޵�ѹ
		u16 dowm_v;             //���޵�ѹ
		u16 i;                  //����
		u16 stop_i;             //��ֹ����
		u16 stop_t;             //��ֹʱ��
		u16 stop_cap;           //��ֹ����
	}step[5];
}page9_wo_var;

//lcd_ui
struct
{
    char to_string[50];       //lcd ��Ҫ��ʾ���ַ�����
}lcd_ui_info;

struct page10_wo_var_s
{
	u16 bat[8][5];		//���״̬��Ϣ
	u16 batNUM[8];		//��ر��
}page10_wo_var;

struct page14_ro_var_s
{
	u16 passwad_1;
	u16 passwad_2;
	u16 passwad_3;
	u16 passwad_4;
}page14_ro_var;

struct page14_wo_var_s
{
    u16 passwad_1;
	u16 passwad_2;
	u16 passwad_3;
	u16 passwad_4;
}page14_wo_var;

struct page15_ro_var_s
{
	u16 bat_max_node;
}page15_ro_var;


/**************************************************
* name     :   Iptostring
* funtion  :   IPת��Ϊ�ַ���
* parameter��  string_ip �ַ���IP int_ip ��ת��IP
               mode ת����IP���ͣ�IP, �������룬 ���أ�
* return   :
***************************************************/
void Iptostring(char *string_ip, sys_cfg_def int_ip, int mode)
{
    u8 temp_ip[4];
    int i;

    if(mode == 0){              //IP
        temp_ip[0] = int_ip.lip1;
        temp_ip[1] = int_ip.lip2;
        temp_ip[2] = int_ip.lip3;
        temp_ip[3] = int_ip.lip4;
    }

    if(mode == 1){              //����
        temp_ip[0] = int_ip.drip1;
        temp_ip[1] = int_ip.drip2;
        temp_ip[2] = int_ip.drip3;
        temp_ip[3] = int_ip.drip4;
    }

    if(mode == 2){              //��������
        temp_ip[0] = int_ip.netmask1;
        temp_ip[1] = int_ip.netmask2;
        temp_ip[2] = int_ip.netmask3;
        temp_ip[3] = int_ip.netmask4;
    }

    for(i = 0; i < 4; i++){
        if((temp_ip[i] / 100) != 0 ){
            *string_ip = (temp_ip[i] / 100) + '0';
            if(temp_ip[i] == 100 || temp_ip[i] == 200){
                string_ip++;
                *string_ip = '0';
                temp_ip[i] = temp_ip[i] % 100;
            }else{
                temp_ip[i] = temp_ip[i] % 100;
            }
            string_ip++;
        }else{
          //  *string_ip = '0';
          //  string_ip++;
        }
        if(temp_ip[i] / 10 != 0){
            *string_ip = (temp_ip[i] / 10) + '0';
            temp_ip[i] = temp_ip[i] % 10;
            string_ip++;
        }else{
        //    *string_ip = '0';
        //    string_ip++;
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
* name     :   read_stage_state
* funtion  :   ��ѯ��״̬
* parameter��  group ��
* return   :
***************************************************/
u8 read_stage_state(u16 group)
{
	u8 module;
	u8 now_step,step_type;

	now_step    = 0;
	step_type   = 0;
	module      = group / 4;

    if(bst.module_info[module].link_status == MODULE_LINK_OFF)
    {
        return STAGE_OFFLINE;		//����
    }

    bst.node[group].info |= 1;
    if((bst.node[group].info & 0x01) == 0 || bst.node[group].run_status == SYS_DISABLE){  //����
        return STAGE_DISABLE ;
    }

    if((bst.node[group].info & 0x80) != 0){    //�й���
        if(bst.node[group].node_status = 10){   //�ϰ汾����
            return STAGE_ALARM;
        }else if(bst.node[group].node_status = 11){ //��������
            return STAGE_SAMPLE;
        }else if(bst.node[group].node_status = 12){  //Ӳ������
            return STAGE_HARD;
        }
    }

    if(bst.node[group].run_status == SYS_COMP )	//���
	{
		return STAGE_ALLDONE;
	}
	else if(bst.node[group].run_status == SYS_PAUSE )	//��ͣ
	{
		return STAGE_PAUSE;
	}
	else if(bst.node[group].run_status == SYS_RUNNING)	//ϵͳ������
	{
			now_step = bst.node[group].now_step;
			step_type = bst.node[group].step[now_step].type;	//��������
			if(step_type == CV || step_type == HCV)
			{
				return 13;		//�����
			}
			else 	if(step_type == DCV)
			{
				return 14;		//�ŵ���
			}
			else if(step_type == REST)
			{
				return 15;		//����
			}
			else
			{
				return STAGE_OTHER;		//��������
			}
	}
	else if(bst.node[group].run_status == SYS_IDLE)		//
	{
		if(bst.node[group].step_num < 2) //δ����
		{
            return STAGE_CFG;
		}
		return STAGE_IDLE;          //����
	}
	else if(bst.node[group].run_status == SYS_START)
	{
		return STAGE_WORKING;		//������
	}
	else if(bst.node[group].run_status == SYS_DIVCAP)  //������
	{
		return STAGE_DIVCAP;
	}
	return 0xFF;
}

/**************************************************
* name     : page0_pool_handler
* funtion  : LOGO��ʾҳ��
* parameter��
* return   :
***************************************************/
u8 page0_pool_handler(u32 poll_cnt)  //logoҳ��
{
	u8 i;

	for(i = 1; i < 56; i++)
	{
		//��ʾ������
		dgus_fill_rec(30, 212, i * 8, 222, 0xffff);
		usleep(20*1000);
	}
	dgus_copy_rec(0x0001, 0x0050, 15, 6, 100, 45, 15, 6);
	return ALL_MONNTORING_PAGE;
}

//ҳ��0����  LOGOҳ��
page_t page0 =
{
    page0_pool_handler,     //poll_handler
    NULL,                   //vchange_handler
    NULL,                   //button_handler
    NULL,                   //page_init
    NULL,                   //wo
    NULL,                   //ro
    POLL_T,                 //poll_time
    0,                      //wo_len
    0,                      //ro_len
};

/**************************************************
* name     : page1_poll_handler
* funtion  : ʵʱˢ��������
* parameter��
* return   :
***************************************************/
u8 page1_poll_handler(u32 poll_cnt)
{
    u8 i;
	u16 temp;

	if(poll_cnt % 15){
        if(page1_init_flag == 0){
            page1_init_flag = 1;
            for(i = 0 ; i < 16; i++)
            {
                *(u16*)&send_buf[6+i*2] =  htons(i + 1 + switch_page_flag * 16);
            }
            dgus_write_var(0x0110,32);
        }

        if((poll_cnt % 5)==0)
        {
            for(i = 0; i < 16; i++){
                temp = read_stage_state(i + switch_page_flag * 16);
                stage[i] = htons(temp);
            }
            dgus_write_current_page();
        }


        if(slip_mun >= 95 && slip_mun <=101){   //������ֵ��95��101֮�䣬��������ؽ���
            slip_mun = 0;
            return CHECK_PASSWAD_PAGE;
        }
	}
	return PAGE_ID_INVALID;
}

/**************************************************
* name     : page1_button_handler
* funtion  : �������������
* parameter��
* return   :
***************************************************/
u8 page1_button_handler(u8 offset)
{
    u8 i;
    static u8 addmun;

    printf("offset = 0x%x \n",offset);
	switch(offset)
	{
        case 0x00://����ϵͳ����
            addmun++;
            printf("addmun = %d\r\n", addmun);
            return SYS_OPTION_PAGE;

        case 0x01:
			read_single_page_num = (1 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x02:
			read_single_page_num = (2 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x03:
			read_single_page_num = (3 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x04:
			read_single_page_num = (4 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x05:
            //exit(-1);
			read_single_page_num = (5 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x06:
			read_single_page_num = (6 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x07:
            read_single_page_num = (7 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x08:
			read_single_page_num = (8 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x09:
			read_single_page_num = (9 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x0A:
			read_single_page_num = (10 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x0B:
			read_single_page_num = (11 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x0C:
			read_single_page_num = (12 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x0D:
			read_single_page_num = (13 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x0E:
			read_single_page_num = (14 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x0F:
			read_single_page_num = (15 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;

		case 0x10:
			read_single_page_num = (16 + switch_page_flag * 16);
			return SINGLE_MONNTORING_PAGE;
		case 0x11:
            if(switch_page_flag != 0){
                switch_page_flag--;
            }
            for(i = 0 ; i < 16; i++)
            {
                *(u16*)&send_buf[6+i*2] =  htons(i + 1 + switch_page_flag*16);
            }

            dgus_write_var(0x0110,32);
            return PAGE_ID_INVALID;

		case 0x12:
            if(switch_page_flag != PAGE_SUM){
                switch_page_flag++;
            }
            for(i = 0 ; i < 16; i++)
            {
                *(u16*)&send_buf[6+i*2] =  htons(i + 1 + switch_page_flag*16);
            }

            dgus_write_var(0x0110,32);
            return PAGE_ID_INVALID;
        case 0x13:
            return NODE_REAL_DATA;
		default:
			return PAGE_ID_INVALID;
			break;

	}
	return PAGE_ID_INVALID;
}


void page1_vchange_handler(u8 offset, u32 data)
{
	u16 value = htons((u16)data);
	slip_mun = value;
#if LCD_DEBUG_CONFIG > 0
    printf("value = %d offset = %d\r\n", value, offset);
#endif

}

/**************************************************
* name     : page1_init
* funtion  : �������ʼ��
* parameter��
* return   :
***************************************************/
void page1_init()
{
    page1_init_flag = 0;
}

//����ؽ���
page_t page1 =
{
		page1_poll_handler,  	//poll_handler
		page1_vchange_handler, //NULL, 					//vchange_handler
		page1_button_handler,   //button_handler
		page1_init,  			//page_init
		&stage,                 //wo
		NULL,  				    //ro
		20,//POLL_T,    				//poll_time
		sizeof(page1_wo_var),   //wo_len
		0,     				    //ro_len
};

/**************************************************
* name     : page2_init
* funtion  : ������״̬
* parameter��
* return   :
***************************************************/
void page2_init()
{
	dgus_show_text(0x0210, "");
	dgus_show_text(0x0220, "");
}

/**************************************************
* name     : page2_poll_handler
* funtion  : ����״̬ʵʱˢ��
* parameter��
* return   :
***************************************************/
u8 page2_poll_handler(u32 poll_cnt)
{
    u8 node_status;
    u16 temp;

    if(poll_cnt % 8== 0){
        temp = read_stage_state(read_single_page_num - 1);
        page2_wo_var.status         = htons(temp);
        page2_wo_var.v              = htons(bst.node[read_single_page_num - 1].v);
        page2_wo_var.i              = htons(bst.node[read_single_page_num - 1].i);
        page2_wo_var.capacity       = htons((u16)(bst.node[read_single_page_num - 1].capacity / 3600));
        page2_wo_var.discapacity    = htons((u16)(bst.node[read_single_page_num - 1].discapacity / 3600));
        page2_wo_var.time           = htons(bst.node[read_single_page_num - 1].time / 60);

        dgus_write_current_page();

    node_status = bst.node[read_single_page_num - 1].node_status;

    if(bst.node[read_single_page_num - 1].run_status == SYS_IDLE)
    {
        node_status = 0;
    }

    switch(node_status)
    {
        case 0:	dgus_show_text(0x0224,"״̬δ֪"); break;

        case 1:	dgus_show_text(0x0224,"������"); break;
        case 2:	dgus_show_text(0x0224,"��ѹ�����"); break;
        case 3:	dgus_show_text(0x0224,"���������"); break;
        case 4:	dgus_show_text(0x0224,"�����ŵ���"); break;
        case 5:	dgus_show_text(0x0224,"��ͣ����"); break;

        case 10:dgus_show_text(0x0224,"��Դ����"); break;
        case 11:dgus_show_text(0x0224,"��������"); break;
        case 12:dgus_show_text(0x0224,"Ӳ������"); break;
        //case 11:dgus_show_text(0x0224,"��Դ���ϵ�ѹ������쳣"); break;
        //case 12:dgus_show_text(0x0224,"��Դ���Ϲ�ѹ���������"); break;
        case 13:dgus_show_text(0x0224,"�ڵ����"); break;

        case 20:dgus_show_text(0x0224,"�ﵽ��ֹʱ��ֹͣ"); break;
        case 21:dgus_show_text(0x0224,"�ﵽ��ֹ��ѹֹͣ"); break;
        case 22:dgus_show_text(0x0224,"�ﵽ��ֹ����ֹͣ"); break;
        case 23:dgus_show_text(0x0224,"�ﵽ��ֹ����ֹͣ"); break;

        case 24:dgus_show_text(0x0224,"����������ֹͣ"); break;
        case 25:dgus_show_text(0x0224,"����ѹƫ��ֹͣ"); break;

        case 26:dgus_show_text(0x0224,"����ѹ�����쳣ֹͣ"); break;
        case 27:dgus_show_text(0x0224,"������쳣��Ӵ��쳣"); break;
        case 28:dgus_show_text(0x0224,"�ﵽ��������ֵֹͣ"); break;
        case 29:dgus_show_text(0x0224,"����ѹƫСֹͣ"); break;

        case 30:dgus_show_text(0x0224,"��طŵ��ѹ����ֹͣ"); break;
        case 31:dgus_show_text(0x0224,"�ŵ�����쳣ֹͣ"); break;
        case 32:dgus_show_text(0x0224,"�Ǻ����ŵ�ֹͣ"); break;
        case 33:dgus_show_text(0x0224,"����ѹ��������"); break;
        case 34:dgus_show_text(0x0224,"�ﵽ��������ֵֹͣ"); break;
        default :dgus_show_text(0x0224,"δ֪"); break;
    }
  }
    return PAGE_ID_INVALID;
}

/**************************************************
* name     : page2_vchange_handler
* funtion  : ����״̬��ȡ���õ�ѹ�͵����Ĳ���
* parameter��
* return   :
***************************************************/
void page2_vchange_handler(u8 offset, u32 data)
{
	u16 value = htons((u16)data);

#if LCD_DEBUG_CONFIG > 0
    printf("value = %d offset = %d\r\n", value, offset);
#endif

    if(offset == 80)		//���õ�ѹ
	{
        page2_ro_var.Set_Vlotage = value;
	}else if(offset == 81)		//���õ���
	{
        page2_ro_var.Set_Current = value;
	}
}

/**************************************************
* name     : page2_button_handler
* funtion  : ����ʵʱ״̬������������
* parameter��
* return   :
***************************************************/
u8 page2_button_handler(u8 offset)
{
    u8 module;
    u16 set_v,set_i;

    module  = (read_single_page_num - 1);  //��ʾ��ǰ�ڵ��

    if(offset == 0){
        if(opera_flag == 1 || opera_flag == 2){
            opera_flag = 0;
            bst.node[module].node_status = 1;   //����
            bst.node[module].work_type = REST;
            bst.node[module].set_v = 0;
            bst.node[module].set_i = 0;
            page2_ro_var.Set_Vlotage = 0;
            page2_ro_var.Set_Current = 0;
        }

        return ALL_MONNTORING_PAGE;
    }else if(offset == 1){
        opera_flag = 1;
        set_v = (page2_ro_var.Set_Vlotage);
        set_i = (page2_ro_var.Set_Current);
        if(bst.node[read_single_page_num - 1].run_status != SYS_RUNNING){
            bst.node[module].node_status = 3;   //�������
            bst.node[module].set_v = set_v;
            bst.node[module].set_i = set_i;
            bst.node[module].work_type = CV;
            printf("chong %d %d %d \r\n", module, bst.node[module].set_v, bst.node[module].set_i);
        }else{
            dgus_show_text(0x0210, "�豸���������У��޷����г�ŵ���ԣ�");
        }
    }else if(offset == 2){
        opera_flag = 2;
        set_v = (page2_ro_var.Set_Vlotage);
        set_i = (page2_ro_var.Set_Current);
        if(bst.node[read_single_page_num - 1].run_status != SYS_RUNNING){
            bst.node[module].node_status = 4;   //�����ŵ�
            bst.node[module].set_v = set_v;
            bst.node[module].set_i = set_i;
            printf("fang %d %d \r\n", bst.node[module].set_v, bst.node[module].set_i);
            bst.node[module].work_type = DCV;
        }else{
            dgus_show_text(0x0210, "�豸���������У��޷����г�ŵ���ԣ�");
        }
    }else if(offset == 3){
        if(bst.node[module].info & NODE_ERR_BIT ){
           bst.node[module].info &= ~NODE_ERR_BIT;
        }
        printf("clear\r\n");
    }

    return PAGE_ID_INVALID;
}

//�����ؼ��״̬
page_t page2 =
{
		page2_poll_handler,      //poll_handler
		page2_vchange_handler,   //vchange_handler
		page2_button_handler,    //button_handler
		page2_init,              //page_init
		(u8*)&page2_wo_var,      //wo
		(u8*)&page2_ro_var,      //ro
		1000,     			     //poll_time
		sizeof(page2_wo_var),    //wo_len
		sizeof(page2_ro_var),
};

/**************************************************
* name     : page4_button_handler
* funtion  : ϵͳ���ý���ѡ��
* parameter��offset  ѡ��Ľ���
* return   :
***************************************************/
u8 page4_button_handler(u8 offset)
{
    if(offset == 0){
        return ALL_MONNTORING_PAGE;
    }else if(offset == 1){
        return NET_CONFIG_PAGE;
    }else if(offset == 2){
        return STEP_CONFIG_PAGE;
    }else if(offset == 3){
        return MODULE_INFO_PAGE;
    }else if(offset == 4){
        return SYS_CONFIG_PAGE;
    }

    return PAGE_ID_INVALID;
}

page_t page4 =
{
		NULL,
		NULL,
		page4_button_handler,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
};

/**************************************************
* name     : recover_network_configure
* funtion  : IP��������
* parameter���������������ļ�
* return   :
***************************************************/
int recover_network_configure()
{
    int fd;
    int result;
    char cmd[256];
    char *ip;

    ip = (char *)malloc(sizeof(char) * 32);
    fd = open("/etc/network/interfaces", O_CREAT | O_RDWR, 0777);

    if(fd < 0){
        #if LCD_DEBUG_CONFIG > 0
        printf("open /etc/network/interfaces file error!\r\n");
        #endif
        return -1;
    }

    result = write(fd, "# interface file auto-generated by buildroot\n \nauto lo\niface lo inet loopback \n\
                    \nauto eth0\n#iface eth0 inet dhcp\niface eth0 inet static\n", 155);
    if(result == -1){
        #if LCD_DEBUG_CONFIG > 0
        printf("write 1  /etc/network/interfaces file error!\r\n");
        #endif
        return -1;
    }

    Iptostring(ip, sys_cfg, 0);
    printf("op = %s\r\n", ip);
    sprintf(cmd, "address %s\n", ip);

    result = write(fd, cmd, strlen(cmd));
    if(result == -1){
        #if LCD_DEBUG_CONFIG > 0
        printf("write 2  /etc/network/interfaces file error!\r\n");
        #endif
        return -1;
    }

    Iptostring(ip, sys_cfg, 2);
    sprintf(cmd, "netmask %s\n", ip);

    result = write(fd, cmd, strlen(cmd));
    if(result == -1){
        #if LCD_DEBUG_CONFIG > 0
        printf("write 3  /etc/network/interfaces file error!\r\n");
        #endif
        return -1;
    }

    Iptostring(ip, sys_cfg, 1);
    sprintf(cmd, "gateway %s\n", ip);

    result = write(fd, cmd, strlen(cmd));
    if(result == -1){
        #if LCD_DEBUG_CONFIG > 0
        printf("write 4  /etc/network/interfaces file error!\r\n");
        #endif
        return -1;
    }

    result = write(fd, " \niface ppp0 inet ppp\n#       provider gprs-dial\n#       provider 3g-dial\n#       provider quectel-dial\n        provider ppp-dial\n", 130);
    if(result == -1){
        #if LCD_DEBUG_CONFIG > 0
        printf("write 1  /etc/network/interfaces file error!\r\n");
        #endif
        return -1;
    }

    free(ip);
    close(fd);
    return 0;
}

/**************************************************
* name     : page5_button_handler
* funtion  : IP��������
* parameter��offset  Ҫ���õĲ���
* return   :
***************************************************/
u8 page5_button_handler(u8 offset)
{
	char *mun;
	char *net_cmd;
    printf("offset = %x\r\n", offset);

	if(offset == 0) 						 //����
	{
		return SYS_OPTION_PAGE;  //����ҳ��
	}
	else if(offset == 1)       //ȷ��
	{
		if(dgus_read_page(5))
		{
			sys_cfg.lip1 = htons(page5_ro_var.lip1);
			sys_cfg.lip2 = htons(page5_ro_var.lip2);
			sys_cfg.lip3 = htons(page5_ro_var.lip3);
			sys_cfg.lip4 = htons(page5_ro_var.lip4);			//IP��ַ

			sys_cfg.rport = htons(page5_ro_var.rport);	        //�˿ں�

			sys_cfg.drip1 = htons(page5_ro_var.drip1);
			sys_cfg.drip2 = htons(page5_ro_var.drip2);
			sys_cfg.drip3 = htons(page5_ro_var.drip3);
			sys_cfg.drip4 = htons(page5_ro_var.drip4);			 //����

			sys_cfg.netmask1 = htons(page5_ro_var.netmask1);
			sys_cfg.netmask2 = htons(page5_ro_var.netmask2);
			sys_cfg.netmask3 = htons(page5_ro_var.netmask3);
			sys_cfg.netmask4 = htons(page5_ro_var.netmask4);	//��������

			sys_cfg.rip1 = htons(page5_ro_var.rip1);
			sys_cfg.rip2 = htons(page5_ro_var.rip2);
			sys_cfg.rip3 = htons(page5_ro_var.rip3);
			sys_cfg.rip4 = htons(page5_ro_var.rip4);					//������IP

			update_net_config_values(sys_cfg);

			system("rm /etc/network/interfaces \n");

			recover_network_configure();

			system("ifdown eth0 \n");
			system("ifup eth0 \n");
			usleep(1000);
			udp_communication_init();

			return SYS_OPTION_PAGE;  //����ҳ��
		}
	}
	return PAGE_ID_INVALID;
}

/**************************************************
* name     : page5_init
* funtion  : IP���ó�ʼ��
* parameter��
* return   :
***************************************************/
void page5_init()
{
    page5_ro_var.id = htonl(1);
    page5_ro_var.lip1 = htons(sys_cfg.lip1);
    page5_ro_var.lip2 = htons(sys_cfg.lip2);
    page5_ro_var.lip3 = htons(sys_cfg.lip3);
    page5_ro_var.lip4 = htons(sys_cfg.lip4);

    page5_ro_var.drip1 = htons(sys_cfg.drip1);
    page5_ro_var.drip2 = htons(sys_cfg.drip2);
    page5_ro_var.drip3 = htons(sys_cfg.drip3);
    page5_ro_var.drip4 = htons(sys_cfg.drip4);

    page5_ro_var.netmask1 = htons(sys_cfg.netmask1);
    page5_ro_var.netmask2 = htons(sys_cfg.netmask2);
    page5_ro_var.netmask3 = htons(sys_cfg.netmask3);
    page5_ro_var.netmask4 = htons(sys_cfg.netmask4);

    page5_ro_var.rip1 = htons(sys_cfg.rip1);
    page5_ro_var.rip2 = htons(sys_cfg.rip2);
    page5_ro_var.rip3 = htons(sys_cfg.rip3);
    page5_ro_var.rip4 = htons(sys_cfg.rip4);

    page5_ro_var.rport = htons(sys_cfg.rport);
    dgus_init_ro_var(5);
}

//��������
page_t page5 =
{
		NULL, 					    //poll_handler
		NULL,  					    //vchange_handler
		page5_button_handler,      //button_handler
		page5_init,  	            //page_init
		NULL,                       //wo
		(u8*)&page5_ro_var,        //ro
		0,                          //poll_time
		0,                          //wo_len
		sizeof(page5_ro_var),      //ro_len
};

/**************************************************
* name     : page6_button_handler
* funtion  : ������ѯ��ѡ��
* parameter��offset  ѡ�����
* return   :
***************************************************/
u8 page6_button_handler(u8 offset)
{
    int i;
    printf("offset = %x\r\n", offset);
    if(offset == 0)			//����
	{
	    step_group_select = 0;
		return SYS_OPTION_PAGE;
	}else if(offset <= 16){        //ѡ����
        page6_group_select = offset + step_group_select * 16;
        return STEP_INFO_PAGE;
	}else if(offset == 17){         //��һҳ
	    if(step_group_select != 0){
            step_group_select--;
            for(i = 0 ; i < 16; i++)
            {
                *(u16*)&send_buf[6+i*2] =  htons(i + step_group_select * 16 + 1);
            }
            dgus_write_var(0x0600,32);

	    }

	}else if(offset == 18){         //��һҳ
	    if(step_group_select != STEP_GROUP_SELECT){
            step_group_select++;
            for(i = 0 ; i < 16; i++)
            {
                *(u16*)&send_buf[6+i*2] =  htons(i + step_group_select * 16 + 1);
            }
            dgus_write_var(0x0600,32);
	    }
	}

	return PAGE_ID_INVALID;
}

/**************************************************
* name     : page6_init
* funtion  : ������ѯ��ʼ��
* parameter��
* return   :
***************************************************/
void page6_init()
{
    int i;

    //step_group_select = 0;

    for(i = 0 ; i < 16; i++)
	{
        *(u16*)&send_buf[6+i*2] =  htons(i + step_group_select * 16 + 1);
	}
	dgus_write_var(0x0600,32);
}

//������ѡ��
page_t page6=
{
        NULL,
		NULL,
		page6_button_handler,
		page6_init,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
};

/**************************************************
* name     : page7_poll_handler
* funtion  : �汾��Ϣʵʱ��ѯ
* parameter��
* return   :
***************************************************/
u8 page7_poll_handler(u32 poll_cnt)
{
	if(poll_cnt % 15)
	{
        dgus_show_text(0x0710+(module_cnt %16 * 10), (char *)&bst.module_info[module_cnt].module_ver[0]);

        if(++module_cnt>= (page7_offset *16 + 16))
        {
            module_cnt = page7_offset *16;
        }

	}
	 return PAGE_ID_INVALID;
}

/**************************************************
* name     :  page7_button_handler
* funtion  :  �汾��Ϣ�������
* parameter�� offset ����ѡ��
* return   :
***************************************************/
u8 page7_button_handler(u8 offset)
{
    printf("offset = %x\r\n", offset);
	u8 i;
	if(offset == 0)  //ȡ��
	{
		return SYS_OPTION_PAGE;  //
	}
	if(offset == 1)  //��һҳ
	{
		if((page7_offset + 1) * 64 < MAX_NODE){
            page7_offset++;
		}
		//module_cnt = 16 * (page7_offset - 1);
		for( i = 0 ; i < 16; i++)
		{
			 dgus_show_text(0x0710+(i * 10),"         ");
		}
        for(i=0;i<16;i++)
        {
            page7_wo_var.batNUM[i] = htons(i+1 + page7_offset*16);
        }

        dgus_write_current_page();
	}
	if(offset == 2)  //��һҳ
	{
		if(page7_offset > 0){
            page7_offset--;
		}
		//module_cnt  = 0;
		for( i = 0 ; i < 16; i++)
	    {
			 dgus_show_text(0x0710+(i * 10),"         ");
		}
        for(i=0;i<16;i++)
        {
            page7_wo_var.batNUM[i]=htons(i+1 + page7_offset*16);
        }

        dgus_write_current_page();
	}
	module_cnt = 16 * (page7_offset - 1);
	return PAGE_ID_INVALID;
}

/**************************************************
* name     :  page7_init
* funtion  :  ��ȡ�汾��Ϣ��ʼ��
* parameter��
* return   :
***************************************************/
void page7_init()
{
	u8 i;
	page7_offset = 0;
	module_cnt = 0;
	for( i = 0 ; i < 16; i++)
	{
		 dgus_show_text(0x0710+(i * 10),"         ");
	}
    for(i=0;i<16;i++)
    {
        page7_wo_var.batNUM[i]=htons(i+1 + page7_offset*16);
    }

    dgus_write_current_page();

}

//�汾��Ϣ
page_t page7 =
{
		page7_poll_handler,  //poll_handler
		NULL,  //vchange_handler
		page7_button_handler,  //button_handler
		page7_init,  //page_init
		(u8*)&page7_wo_var,//&page18_wo_var,  //wo
		NULL,  //ro
		POLL_T,     //poll_time
		sizeof(page7_wo_var),     //wo_len
		0,
};

/**************************************************
* name     :  page8_vchange_handler
* funtion  :  ʵʱ��ȡ���õ�ϵͳ��������
* parameter��
* return   :
***************************************************/
void page8_vchange_handler(u8 offset, u32 data)
{
	u16 value = htons((u16)data);

	if(offset == 1)		//�͵�ѹ
	{
		page8_temp_low_volt = value;
	}
	else if(offset == 2)		//�ߵ�ѹ
	{
		page8_temp_high_volt = value;
	}

	else if(offset == 3)		//��ͱ�����ѹ
	{
		page8_temp_protect_low_v = value;
	}
	else if(offset == 4)		//��߱�����ѹ
	{
		page8_temp_protect_high_v = value;
	}
	else if(offset == 5)		//��ͱ�������
	{
		page8_temp_protect_low_i = value;
	}
	else if(offset == 6)		//��߱�������
	{
		page8_temp_protect_high_i = value;
	}

	page8_wo_var.low_volt          = htons(page8_temp_low_volt);
	page8_wo_var.high_volt 		   = htons(page8_temp_high_volt);
	page8_wo_var.peotect_low_v     = htons(page8_temp_protect_low_v);
	page8_wo_var.peotect_high_v    = htons(page8_temp_protect_high_v);
	page8_wo_var.peotect_low_i     = htons(page8_temp_protect_low_i);
	page8_wo_var.peotect_high_i    = htons(page8_temp_protect_high_i);
	dgus_write_current_page();
}

/**************************************************
* name     :  page8_button_handler
* funtion  :  ϵͳ�������ð�������
* parameter�� offset ����
* return   :
***************************************************/
u8 page8_button_handler(u8 offset)
{
    printf("offset = %x\r\n", offset);
    if(offset == 0)			//����
	{
		return SYS_OPTION_PAGE;
	}
	else if(offset == 1)	  	//��������
    {	if(page8_temp_low_volt <= page8_temp_high_volt && page8_temp_protect_low_v <= page8_temp_protect_high_v && \
          page8_temp_protect_low_i <= page8_temp_protect_high_i)
        {
            sys_cfg.led_lighting_mode= htons(	page8_wo_var.led_lighting_mode);
            sys_cfg.low_volt = page8_temp_low_volt;
            sys_cfg.high_volt = page8_temp_high_volt;

            sys_cfg.protect_low_v  = page8_temp_protect_low_v;
            sys_cfg.protect_high_v = page8_temp_protect_high_v;
            sys_cfg.protect_low_i  = page8_temp_protect_low_i;
            sys_cfg.protect_high_i = page8_temp_protect_high_i;

            update_sys_config_values(sys_cfg);
            return SYS_OPTION_PAGE;
        }
	}
	else if(offset == 2)	 //�л�����ģʽ
	{
		if(page8_wo_var.led_lighting_mode== 0)
		{
			 page8_wo_var.led_lighting_mode= htons(1);
		}
		else
		{
			 page8_wo_var.led_lighting_mode = htons(0);
		}
		dgus_write_current_page();
	}
	else
	{

	}
	return PAGE_ID_INVALID;
}

/**************************************************
* name     :  page8_init
* funtion  :  ϵͳ�������ó�ʼ��
* parameter��
* return   :
***************************************************/
void page8_init()
{
	page8_temp_low_volt = sys_cfg.low_volt;
	page8_temp_high_volt = sys_cfg.high_volt;
	page8_temp_protect_low_v      = (sys_cfg.protect_low_v);
	page8_temp_protect_high_v     = (sys_cfg.protect_high_v);
	page8_temp_protect_low_i      = (sys_cfg.protect_low_i);
	page8_temp_protect_high_i     = (sys_cfg.protect_high_i);

	page8_wo_var.led_lighting_mode = htons(sys_cfg.led_lighting_mode);
	page8_wo_var.low_volt          = htons(sys_cfg.low_volt);
	page8_wo_var.high_volt 		   = htons(sys_cfg.high_volt);

	page8_wo_var.peotect_low_v      = htons(sys_cfg.protect_low_v);
	page8_wo_var.peotect_high_v     = htons(sys_cfg.protect_high_v);
	page8_wo_var.peotect_low_i      = htons(sys_cfg.protect_low_i);
	page8_wo_var.peotect_high_i     = htons(sys_cfg.protect_high_i);
	dgus_write_current_page();
}

//ϵͳ��������
page_t page8 =
{
		NULL, //page7_poll_handler,      //poll_handler
		page8_vchange_handler,                    //vchange_handler
		page8_button_handler,    //button_handler
		page8_init,               //page_init
		(u8*)&page8_wo_var,       //wo
		(u8*)&page8_ro_var,       //ro
		1000,     							  //poll_time
		sizeof(page8_wo_var),     //wo_len
		sizeof(page8_ro_var),
};

/**************************************************
* name     :  page9_poll_handler
* funtion  :  ������Ϣ��ѯ
* parameter��
* return   :
***************************************************/
u8 page9_poll_handler(u32 poll_cnt)
{
	static u8  cnt;
	u8 group,i,step,type;
	group = page6_group_select-1;

    if(poll_cnt % 15 == 0)
    {
        cnt++;
        i = 0;

        if(bst.node[group].step_num !=0)
        {
            for(step = page9_step_check_page * 5 ; step < (page9_step_check_page + 1)*5; step++)
            {
                if(step >= (bst.node[group].step_num))
                {
                    page9_wo_var.step[i].num        = 0;
                    page9_wo_var.step[i].type       = 0;
                    page9_wo_var.step[i].up_v  		= 0;
                    page9_wo_var.step[i].dowm_v  	= 0;
                    page9_wo_var.step[i].i       	= 0;
                    page9_wo_var.step[i].stop_i  	= 0;
                    page9_wo_var.step[i].stop_t  	= 0;		//ʱ�� ��λΪ��
                    page9_wo_var.step[i].stop_cap   = 0;
                    i++;
                    continue;
                }

                type =  bst.node[group].step[step].type;
                page9_wo_var.step[i].num  = htons(step+1);

                if(type == 0x01) //��� �Ⱥ������ѹ
                {
                    page9_wo_var.step[i].type  = htons(1);
                }
                else if(type == 0x03)
                {
                    page9_wo_var.step[i].type  = htons(2);		//�ŵ�
                }
                else if(type == 0x02)
                {
                    page9_wo_var.step[i].type  = htons(3);  //����
                }
                else if (type == 0x04)  //�������
                {
                    page9_wo_var.step[i].type  = htons(4);
                }
                else
                {
                    page9_wo_var.step[i].type  = htons(5);  //  ��������
                }

                page9_wo_var.step[i].up_v  	  = htons(bst.node[group].step[step].parm1);
                page9_wo_var.step[i].dowm_v   = htons(bst.node[group].step[step].parm2);
                page9_wo_var.step[i].i        = htons(bst.node[group].step[step].cond1);
                page9_wo_var.step[i].stop_i   = htons(bst.node[group].step[step].cond2);
                page9_wo_var.step[i].stop_t   = htons(bst.node[group].step[step].cond3 / 6);		//ʱ�� ��λΪ��
                page9_wo_var.step[i].stop_cap = htons(bst.node[group].step[step].cond4);

                i++;
            }

        //	if(cnt <= 2){
            sprintf(lcd_ui_info.to_string, "��%d��,ƽ̨��ѹ: %.3fV,��������: %d,ѭ������: %d",\
                page6_group_select,(double)bst.node[group].platform_v/1000 ,\
                    bst.node[group].step_num, bst.node[group].cyc);
                      dgus_show_text(0x0930,lcd_ui_info.to_string);
        //	}else{
        	//    if(cnt >=20){
        	  //      cnt = 0;
        	 //   }
        	//}


            if(bst.node[group].recharge_ratio != 0xff)  //����
            {
                if(bst.node[group].divcap_start_step != 0xff)
                {
                    //���ݼӷ���
                    sprintf(lcd_ui_info.to_string, "��%d��,����%d~%d ����:%d~%d,����%d%",\
                        page6_group_select,bst.node[group].divcap_start_step,bst.node[group].divcap_end_step,\
                        bst.node[group].recharge_start_step ,bst.node[group].recharge_end_step,bst.node[group].recharge_ratio);
                }
                else
                {
                    //ֻ����
                    sprintf(lcd_ui_info.to_string, "��%d��,������� ���乤��Ϊ:%d~%d,����%d%",\
                        page6_group_select,bst.node[group].recharge_start_step ,bst.node[group].recharge_end_step,bst.node[group].recharge_ratio);
                }
            }
            else
            {
                //ֻ����
                sprintf(lcd_ui_info.to_string, "��%d�� ���ݲ���",page6_group_select);
            }

        	//}
        }
        else
        {
            sprintf(lcd_ui_info.to_string, "��%d��,��������Ϊ: %d",page6_group_select,bst.node[group].step_num);
            dgus_show_text(0x0930,lcd_ui_info.to_string);
        }

        dgus_write_current_page();

    }
	return PAGE_ID_INVALID;
}

/**************************************************
* name     :  page9_button_handler
* funtion  :  ������Ϣ�������
* parameter�� offset  ����
* return   :
***************************************************/
u8 page9_button_handler(u8 offset)
{
	u8 group;
    printf("offset = %x\r\n", offset);
	group = page6_group_select - 1;

    if(offset == 0)			//����
	{
		return STEP_CONFIG_PAGE;
	}
	else if(offset == 1)	  	//��һҳ
	{
        if(page9_step_check_page > 0)
        {
            page9_step_check_page--;
        }
	}
    else if(offset == 2)			//��һҳ
	{
	    printf("bst.node[group].step_num -2 = %d\r\n", bst.node[group].step_num);
	    printf("page9 = %d\r\n", (page9_step_check_page + 1)*5);
	   // page9_step_check_page++;
		if((page9_step_check_page + 1)*5 < (bst.node[group].step_num))
		{

			page9_step_check_page++;
			  printf("page9_step_check_page = %d\r\n", page9_step_check_page);
		}
	}
	return PAGE_ID_INVALID;
}

/**************************************************
* name     :  page9_init
* funtion  :  ������Ϣ�����ʼ��
* parameter��
* return   :
***************************************************/
void page9_init()
{
	char i;
	for(i = MAX_STEP; i > 0 ; i--)
	{
		page9_wo_var.step[i - 1].num        = 0;
		page9_wo_var.step[i - 1].type       = 0;
		page9_wo_var.step[i - 1].up_v  	    = 0;
		page9_wo_var.step[i - 1].dowm_v     = 0;
		page9_wo_var.step[i - 1].i        	= 0;
		page9_wo_var.step[i - 1].stop_i  	= 0;
		page9_wo_var.step[i - 1].stop_t  	= 0;		//ʱ�� ��λΪ��
		page9_wo_var.step[i - 1].stop_cap   = 0;
	}
	dgus_write_current_page();
	dgus_show_text(0x0930,"");
	page9_step_check_page = 0;
}

//������ѯ
page_t page9 =
{
		page9_poll_handler,      //poll_handler
		NULL,                    //vchange_handler
		page9_button_handler,    //button_handler
		page9_init,               //page_init
		(u8*)&page9_wo_var,       //wo
		NULL,       //ro
		10000,     							  //poll_time
		sizeof(page9_wo_var),     //wo_len
		0,
};

/**************************************************
* name     :  page10_poll_handler
* funtion  :  ʵʱ�鿴�����Ϣ
* parameter��
* return   :
***************************************************/
u8 page10_poll_handler(u32 poll_cnt)
{
	u16 temp, group;
	u8 bat,ch;

	bat = 0;

	if(poll_cnt % 15 ==0 )
	{
        for(group = page10_cnt_poll * 8; group < (page10_cnt_poll + 1)*8; group++)
        {
            page10_wo_var.bat[bat][0] = htons(bst.node[group].v);			//��ѹ
            page10_wo_var.bat[bat][1] = htons(bst.node[group].i);			//����

            temp = (u16)(bst.node[group].capacity/3600);
            page10_wo_var.bat[bat][2] = htons(temp);

            temp = (u16)(bst.node[group].discapacity/3600);
            page10_wo_var.bat[bat][3] = htons(temp);
            page10_wo_var.bat[bat][4] = htons(bst.node[group].time / 6);
            bat++;
        }

        for(ch = 0; ch < 8; ch++)
        {
            page10_wo_var.batNUM[ch] = htons((page10_cnt_poll)*8+1+ch);
        }
		dgus_write_current_page();
	}
	return PAGE_ID_INVALID;
}

/**************************************************
* name     :  page10_button_handler
* funtion  :  ʵʱ����������ݰ�������
* parameter��
* return   :
***************************************************/
u8 page10_button_handler(u8 offset)
{
    printf("page10_button_handler = %d\r\n", offset);
    u8 ch;
	if(offset == 0)  //����
	{
		return ALL_MONNTORING_PAGE;  //
	}
	if(offset == 1)  //��һҳ
	{
	    if(page10_cnt_poll > 0){
            page10_cnt_poll--;
            for(ch = 0; ch < 8; ch++)
            {
                page10_wo_var.batNUM[ch] = htons((page10_cnt_poll)*8+1+ch);
            }
            dgus_write_current_page();
	    }
	}
	if(offset == 2)  //��һҳ
	{
	    if(page10_cnt_poll < ((MAX_NODE / 8) - 1)){
            page10_cnt_poll++;
            for(ch = 0; ch < 8; ch++)
            {
                page10_wo_var.batNUM[ch] = htons((page10_cnt_poll)*8+1+ch);
            }
            dgus_write_current_page();
	    }
	}

	return PAGE_ID_INVALID;
}

/**************************************************
* name     :  page10_init
* funtion  :  ʵʱ����������ݳ�ʼ��
* parameter��
* return   :
***************************************************/
void page10_init()
{
	u8 ch;
    page10_cnt_poll = 0;
	for(ch = 0; ch < 8; ch++)
	{
		memset(&page10_wo_var.bat[ch][0],0,5);
		page10_wo_var.batNUM[ch] = htons((page10_cnt_poll)*8+1+ch);
	}
	dgus_write_current_page();
}

//��ʾ���е������
page_t page10 =
{
		page10_poll_handler,        //poll_handler
		NULL,                       //vchange_handler
		page10_button_handler,      //button_handler
		page10_init,                //page_init
		(u8*)&page10_wo_var,        //wo
		NULL,                       //ro
		10000,     				    //poll_time
		sizeof(page10_wo_var),      //wo_len
		0,
};

/**************************************************
* name     : page14_vchange_handler
* funtion  : �����ȡ
* parameter��
* return   :
***************************************************/
void page14_vchange_handler(u8 offset, u32 data)
{
	u16 value = htons((u16)data);
    printf("value = %d offset = %d\r\n", value, offset);

    if(offset == 0){
        page14_ro_var.passwad_1 = value;
    }else if(offset == 1){
        page14_ro_var.passwad_2 = value;
    }else if(offset == 2){
        page14_ro_var.passwad_3 = value;
    }else if(offset == 3){
        page14_ro_var.passwad_4 = value;
    }

}

/**************************************************
* name     : page2_button_handler
* funtion  : ����ʵʱ״̬������������
* parameter��
* return   :
***************************************************/
u8 page14_button_handler(u8 offset)
{
    if(offset == 0 && (passwad_cod[0] == page14_ro_var.passwad_1) && \
                      (passwad_cod[1] == page14_ro_var.passwad_2) && \
                      (passwad_cod[2] == page14_ro_var.passwad_3) && \
                      (passwad_cod[3] == page14_ro_var.passwad_4)){
        page14_wo_var.passwad_1 = 0;    //�������
        page14_wo_var.passwad_2 = 0;
        page14_wo_var.passwad_3 = 0;
        page14_wo_var.passwad_4 = 0;

        dgus_write_current_page();

        return OPEROTOR_PAGE;
    }
    if(offset == 1){
        return ALL_MONNTORING_PAGE;
    }

    return PAGE_ID_INVALID;
}

//����У��ҳ��
page_t page14 =
{
		NULL,                       //poll_handler
		page14_vchange_handler,     //vchange_handler
		page14_button_handler,      //button_handler
		NULL,                       //page_init
        (u8*)&page14_wo_var,        //wo
		(u8*)&page14_ro_var,        //ro
		1000,     			        //poll_time
		sizeof(page14_wo_var),    //wo_len
		sizeof(page14_ro_var),
};

/**************************************************
* name     : page14_vchange_handler
* funtion  : �����ȡ
* parameter��
* return   :
***************************************************/
void page15_vchange_handler(u8 offset, u32 data)
{
	u16 value = htons((u16)data);
    printf("value = %d offset = %d\r\n", value, offset);
    if(value == 96){    //������Ӧͨ�����ĳ��򡣹ر����ݿ⡣ɾ�����ݿ⣬�������� ����chang.sh�ű���ִ��
        system("./change.sh 96");
    }

    if(value == 144){
        system("./change.sh 144");
    }

    if(value == 128){
        system("./change.sh 128");
    }

    if(value == 256){
        system("./change.sh 256");
    }

}

/**************************************************
* name     : page2_button_handler
* funtion  : ����ʵʱ״̬������������
* parameter��
* return   :
***************************************************/
u8 page15_button_handler(u8 offset)
{
    if(offset == 0){    //�˳���������ļ�ϵͳ
        exit(-1);
    }else if(offset == 1){  //�����豸
        system("reboot");
    }else if(offset == 2){
        return ALL_MONNTORING_PAGE;
    }else if(offset == 4){  //ɾ�����ݿ��ļ��������ڴ�
        Vacuum_data();
        close_sqlite_db();
        system("rm /Dbdata/history_data.db");
        system("reboot");
    }else if(offset == 5){  //�������ݿ��ļ�
        if(access("/run/media/sda1",F_OK) != -1){
            printf("have tf card\r\n");
            system("cp /Dbdata/history_data.db /run/media/sda1 -rf");
        }else{
            printf("no tf card\r\n");
        }
    }


    return PAGE_ID_INVALID;
}
void page15_init()
{
    dgus_show_text(0x0F10,"����ͨ������ѡ���ͨ����96��128,144,256");
}

//����У��ҳ��
page_t page15 =
{
		NULL,                       //poll_handler
		page15_vchange_handler,     //vchange_handler
		page15_button_handler,      //button_handler
		page15_init,                //page_init
		NULL,                       //wo
		(u8*)&page15_ro_var,        //ro
		1000,     			        //poll_time
		NULL,    //wo_len
		sizeof(page15_ro_var),
};

//UI�ṹ��
page_t* UI_data[] =
{
    &page0,     // ��������
    &page1,     // ������
    &page2,     // �������״��
    NULL,
    &page4,     // ϵͳ����ѡ��
    &page5,     // ��������
    &page6,     // ������ѯ��ѡ��
    &page7,     // �汾��Ϣ
    &page8,     // ϵͳ����
    &page9,     // ������ѯ��Ϣ
    &page10,    // ��ʾ���е������
    NULL,
    NULL,
    NULL,
    &page14,    //����У��ҳ��
    &page15,    //������Ա���ƽ���
};

/***************end file****************************/
