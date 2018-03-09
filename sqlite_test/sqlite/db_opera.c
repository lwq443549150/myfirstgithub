#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <string.h>
#include "sqlite3.h"
#include "sqlite3ext.h"
#include "db_opera.h"
#include "sys_cfg.h"


#define READ_MUN    128

const sys_cfg_def sys_cfg_default_1 =
{
	  0xAAAA,
		192,  //本机IP
		168,
		1,
		10,

		192,  //网关IP
		168,
		1,
		1,

		255,    //子网掩码
		255,
		255,
		0,

		192,  //远程IP
		168,
		1,
		1,

		8080,  //远程端口

	  1,         	//亮灯模式
		3315,				//合格低电压   单位 1mv
		3360, 			//合格高电压    单位 1mv
		0, 					//最低保护电压   单位 1mv
		3700, 			//最高保护电压   单位 1mv
		50,         //最低保护电流   单位 1ma
		9500        //最高保护电流  单位 1ma
};

int Create_db()
{


}

//
static void write_para(int fd, char *name, int len, char* write_temp)
{
    write(fd, name, len);
    write(fd, write_temp, strlen(write_temp));
    write(fd, "]\n", 2);
}

//
static void getIp_address(char *string_ip, u8 ad1, u8 ad2, u8 ad3, u8 ad4)
{
    u8 temp_ip[4];
    int i;
    temp_ip[0] = ad1;
    temp_ip[1] = ad2;
    temp_ip[2] = ad3;
    temp_ip[3] = ad4;

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

//
void find_data(char *str1, char *str2, char *data)
{
    char *temp = NULL;
    char *dest = str1;
    char *src = str2;


    while(*dest != '\0'){
        while((*dest != '\0') && (*src != '\0') && (*dest == *src)){
            dest++;
            src++;
        }
        if(*src == '\0'){
            printf("dest = %s\r\n", dest);
        }
    }
}
//
int init_sysconfig_file()
{
    int fd;
    int i = 0, k = 0;
    int len;
    char read_buff[128];
    char search_data[8];
    char write_temp[4];

    u8* addr = (u8*)&sys_cfg;

    sys_cfg_def sys_config;

    len = sizeof(sys_cfg_def);

    fd = open(SYS_CONFIG_DIR, O_CREAT | O_RDWR, S_IRWXU);
    if(fd < 0){
        printf("creat init_sys_file failt\r\n");
        return 0;
    }

    if(0 == read(fd, read_buff, READ_MUN)){

        memcpy(&sys_config, &sys_cfg_default_1, len);  //设置为默认参数

        sprintf(write_temp, "%d", sys_config.led_lighting_mode);
        write_para(fd, "led_mode[",       9,   write_temp);
        sprintf(write_temp, "%d", sys_config.low_volt);
        write_para(fd, "low_volt[",       9,   write_temp);
        sprintf(write_temp, "%d", sys_config.high_volt);
        write_para(fd, "high_volt[",      10,  write_temp);
        sprintf(write_temp, "%d", sys_config.protect_low_v);
        write_para(fd, "protect_low_v[",  14,  write_temp);
        sprintf(write_temp, "%d", sys_config.protect_high_v);
        write_para(fd, "protect_high_v[", 15,  write_temp);
        sprintf(write_temp, "%d", sys_config.protect_low_i);
        write_para(fd, "protect_low_i[",  14,  write_temp);
        sprintf(write_temp, "%d", sys_config.protect_high_i);
        write_para(fd, "protect_high_i[", 15,  write_temp);

    }else{
        printf("read_buff = %s\r\n", read_buff);
        find_data(read_buff, "led_model[", search_data);

    }
    close(fd);
}

//
int init_netconfig_file()
{
    int fd;
    int len;
    char read_buff[128];
    char write_temp[20];

    sys_cfg_def sys_config;

    len = sizeof(sys_cfg_def);

    fd = open(NET_CONFIG_DIR, O_CREAT | O_RDWR, S_IRWXU);
    if(fd < 0){
        printf("creat init_net_file failt\r\n");
        return 0;
    }

    if(0 == read(fd, read_buff, READ_MUN)){

        memcpy(&sys_config, &sys_cfg_default_1, len);  //设置为默认参数

        getIp_address(write_temp, sys_config.lip1, sys_config.lip2, sys_config.lip3, sys_config.lip4);
        write_para(fd, "local_IP[", 9,   write_temp);
        getIp_address(write_temp, sys_config.drip1, sys_config.drip2, sys_config.drip3, sys_config.drip4);
        write_para(fd, "geteway_IP[", 10,   write_temp);
        getIp_address(write_temp, sys_config.netmask1, sys_config.netmask2, sys_config.netmask3, sys_config.netmask4);
        write_para(fd, "subnet_mask[", 12,  write_temp);
        getIp_address(write_temp, sys_config.rip1, sys_config.rip2, sys_config.rip3, sys_config.rip4);
        write_para(fd, "server_IP[", 10,  write_temp);
        sprintf(write_temp, "%d", sys_config.rport);
        write_para(fd, "port[", 5,  write_temp);

    }else{
        printf("have data\r\n");
    }
    close(fd);
}

