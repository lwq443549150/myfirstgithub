/********************************************************************
*  文件名:    sqlite_opera.c
*  文件描述:  数据库相关操作函数的实现
*  作者:
*  创建时间:  2017年10月 创建第一版本
********************************************************************/

#ifndef _SQLITE_OPERA_H
#define _SQLITE_OPERA_H

#include "sqlite3.h"
#include "sys_cfg.h"
#include "server.h"

#define  SQLITE3_STATIC


extern int creat_data_db_and_table();
extern int init_sysconfig_opera();
extern int set_sys_config_values(sys_cfg_def sys_cfg_default);
extern int update_net_config_values(sys_cfg_def sys_cfg_default);
extern int update_sys_config_values(sys_cfg_def sys_cfg_default);
extern int save_step_mun(int mun, int step);
extern int recovery_step(int mun);
extern int save_global_group_data(int group);
extern int insert_running_data(int group);
extern int recovery_gloabal_info();
extern int open_sqlite_db();
extern int net_off_save_data();
extern int search_hisdata_fun(u16 mun);
extern int read_hisdata_fun(realtime_data_style_new *, u16 mun);


extern void delete_step_table(int mun);
extern void start_commit();
extern void stop_commit();
extern void getNowTime();
extern void read_step_run_status(void);
extern void delete_bat_runningdata(int group);
extern void delete_bat_runningdata_table();
extern void close_sqlite_db();
extern void Vacuum_data();
extern void getstimeval();
extern void recover_stepcfg_fun();


#endif
