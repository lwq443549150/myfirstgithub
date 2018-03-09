/********************************************************************
*  文件名:    sqlite_opera.c
*  文件描述:  数据库相关操作函数的实现
*  作者:
*  创建时间:  2017年10月 创建第一版本
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sqlite_opera.h"
#include "sys_cfg.h"
#include "debug.h"
#include "sys_logic_control.h"




/***************************宏定义*********************************/
#define SQLITE_DATA_DB                "sqlite_data.db"
#define SQLITE_HISTROY_DATA_DB         "/Dbdata/history_data.db"
#define DB_DATA_FILE_PATH              "/Dbdata"


#define SQLITE_SYS_CONFIG_TABLE       "system_table"
#define RUNNING_DATA_CURRENT          "running_data_table_current"
#define RUNNING_DATA_HISTORY          "running_data_table_history"
#define SQLITE_RUNNING_STATUS_TABLE   "running_status_table"

#define DATA_MUN                       4096         //暂时保存记录为条
#define STEP_TABLE_MUN                 16

/***************************全局变量********************************/
sqlite3 *db = NULL;
sqlite3 *history_data_db = NULL;

static int create_table_flag;    //创建表格标志位，如果创建表格失败，则不进行相关的数据库操作
static double history_data_mun = 1;     //记录历史数据的条数

/******************************************************************
* 函数名: create_dir
* 功能:   判断文件路劲包含的文件夹是否存在，不存在则建立
* 参数：  path 文件夹路劲
* 返回值:
******************************************************************/
void create_dir(char *path)
{
   int ret,ret2;

    ret = access(path,F_OK);
    if(ret == -1)
    {
        printf("Db data file no exist \n");
        ret2 = mkdir(path,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
        if(ret2 != -1)
        {
            printf("create file ok!\n");
        }
    }
    else
    {
         printf("Db data file have exist\n");
    }
}


void getstimeval()
{
    char buf[28];
    struct timeval tv;
    struct tm      tm;
    size_t         len = 28;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", &tm);
    len = strlen(buf);
    sprintf(buf + len, ".%06.6d", (int)(tv.tv_usec));
    printf("%s\n",buf);
}

/******************************************************************
* 函数名: getNowTime
* 功能:   获取本地时间
* 参数：
* 返回值:
******************************************************************/
void getNowTime()
{
    time_t timer;//time_t就是long int 类型
    struct tm *tblock;
    timer = time(NULL);
    tblock = localtime(&timer);
    //tblock = gmtime(&timer);
    //printf("tblock->tm_sec = %d\r\n", tblock->tm_sec);
    printf("Local time is: %s\n", asctime(tblock));
}

/******************************************************************
* 函数名: creat_step_table
* 功能:   创建存储工步的表
* 参数：  mun 相应组的工步
* 返回值: 无
******************************************************************/
int creat_step_table(int mun)
{
    int ret;
    char *errmsg = 0;
    char STEP_TABLE[STEP_TABLE_MUN + 1][16] = {
        "0",
        "step_table1",
        "step_table2",
        "step_table3",
        "step_table4",
        "step_table5",
        "step_table6",
        "step_table7",
        "step_table8",
        "step_table9",
        "step_table10",
        "step_table11",
        "step_table12",
        "step_table13",
        "step_table14",
        "step_table15",
        "step_table16",
   };

    char cmd[384];

    sprintf(cmd, "CREATE TABLE IF NOT EXISTS %s (id integer PRIMARY KEY AUTOINCREMENT,Action_type INT1 NOT NULL, cloum INT2 NOT NULL,step INT2 NOT NULL, parameter1 INT2 NOT NULL, \
       parameter2 INT2 NOT NULL, conditions1 INT NOT NULL, conditions2 INT NOT NULL, conditions3 INT NOT NULL, conditions4 INT NOT NULL, conditions5 INT1 NOT NULL, \
       conditions6 INT1 NOT NULL);", STEP_TABLE[mun]);

    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(SQLITE_OK != ret){
        #if SQLITE_DEBUG_CONFIG > 0
        fprintf(stderr, "Create table fail: %s\n", errmsg);
        #endif
        sqlite3_free(errmsg);
        //sqlite3_close(db);
        create_table_flag = 1;
        return 1;
    }else{
        #if SQLITE_DEBUG_CONFIG > 0
        printf("create table sucess\r\n");
        #endif
        sqlite3_free(errmsg);
    }
    return 0;
}

/******************************************************************
* 函数名: creat_runningData_table
* 功能:   创建保存运行数据的表格
* 参数：  mun 相应的组
* 返回值: 无
******************************************************************/
void creat_runningData_table()
{
    int ret;
    char *errmsg;
    char cmd[416];

    sprintf(cmd, "CREATE TABLE IF NOT EXISTS "RUNNING_DATA_CURRENT" (id integer PRIMARY KEY AUTOINCREMENT, cyc INT2 NOT NULL, sonCyc INT1 NOT NULL, now_step INT1 NOT NULL, work_type INT1 NOT NULL, \
       v INT2 NOT NULL, i INT NOT NULL, capacity INT NOT NULL, em INT NOT NULL, cc_cv INT2 NOT NULL, platform_time INT2 NOT NULL, time INT NOT NULL, node_status INT1 NOT NULL, \
       rflag INT1 NOT NULL, rio INT1 NOT NULL);");

    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(SQLITE_OK != ret){
        #if SQLITE_DEBUG_CONFIG > 0
        fprintf(stderr, "Create table fail: %s\n", errmsg);
        #endif
        sqlite3_free(errmsg);
      //sqlite3_close(db);
        create_table_flag = 1;
        return ;
    }else{
        #if SQLITE_DEBUG_CONFIG > 0
        printf("create table sucess\r\n");
        #endif
        sqlite3_free(errmsg);
    }
}

/******************************************************************
* 函数名: creat_runningData_table_history
* 功能:   创建保存运行历史数据的表格
* 参数：  mun 相应的组
* 返回值: 无
******************************************************************/
void creat_runningData_table_history()
{
    int ret;
    char *errmsg;
    char cmd[512];

    sprintf(cmd, "CREATE TABLE IF NOT EXISTS "RUNNING_DATA_HISTORY" (id integer PRIMARY KEY AUTOINCREMENT, x INT1 NOT NULL, cyc INT2 NOT NULL, sonCyc INT1 NOT NULL, now_step INT1 NOT NULL, work_type INT1 NOT NULL, \
       v INT2 NOT NULL, i INT NOT NULL, capacity INT NOT NULL, em INT NOT NULL, cc_cv INT2 NOT NULL, platform_time INT2 NOT NULL, time INT NOT NULL, node_status INT1 NOT NULL, \
       rflag INT1 NOT NULL, rio INT1 NOT NULL);");

    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(SQLITE_OK != ret){
        #if SQLITE_DEBUG_CONFIG > 0
        fprintf(stderr, "Create table fail: %s\n", errmsg);
        #endif
        sqlite3_free(errmsg);
       //sqlite3_close(db);
        create_table_flag = 1;
        return ;
    }else{
        #if SQLITE_DEBUG_CONFIG > 0
        printf("create table sucess\r\n");
        #endif
        sqlite3_free(errmsg);
    }

/*
    sprintf(cmd, "DELETE FROM "RUNNING_DATA_HISTORY"");
    sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(ret != SQLITE_OK){
        printf("delete history data  error!\r\n");
    }
    sqlite3_free(errmsg);*/
}

/******************************************************************
* 函数名: creat_stepconfig_parm
* 功能:   创建保存工步全局参数数据表
* 参数：
* 返回值: 返回0表示创建失败 返回1表示创建成功
******************************************************************/
int creat_stepconfig_parm()
{
    int ret;
    char *errmsg = NULL;
    char cmd[512] = {0};

    sprintf(cmd, "CREATE TABLE IF NOT EXISTS step_cfg_parm_table (id integer PRIMARY KEY AUTOINCREMENT, node INT NOT NULL, mainCyc INT2 NOT NULL, platV INT2 NOT NULL, testStyle INT1 NOT NULL,\
                                                                  divcapStartStep INT1 NOT NULL, divcapEndStep INT1 NOT NULL, rechargeStartStep INT1 NOT NULL, recharge_end_step \
                                                                  INT1 NOT NULL, rechargeRatio INT1 NOT NULL, rechargeCap INT NOT NULL);" );

    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(SQLITE_OK != ret){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create db error\r\n");
        #endif
        //sqlite3_close(db);
        return 0;
    }
     sqlite3_free(errmsg);
     return 1;
}

/******************************************************************
* 函数名: creat_data_db
* 功能:   初始化创建数据库
* 参数：  无
* 返回值: 无
******************************************************************/
int creat_data_db_and_table()
{
    int ret, i;
    char *errmsg = 0;

    const char *sql_cmd_creat_table_sys = "CREATE TABLE IF NOT EXISTS "SQLITE_SYS_CONFIG_TABLE" (id integer PRIMARY KEY AUTOINCREMENT, lip1 INT1 NOT NULL, lip2 INT1 NOT NULL, \
    lip3 INT1 NOT NULL, lip4 INT1 NOT NULL, drip1 INT1 NOT NULL, drip2 INT1 NOT NULL, drip3 INT1 NOT NULL, drip4 INT1 NOT NULL, \
    netmask1 INT1 NOT NULL, netmask2 INT1 NOT NULL, netmask3 INT1 NOT NULL, netmask4 INT1 NOT NULL, rip1 INT1 NOT NULL, rip2 INT1 NOT NULL, \
    rip3 INT1 NOT NULL, rip4 INT1 NOT NULL, rport INT2 NOT NULL, led_lighting_mode INT1 NOT NULL, low_volt INT2 NOT NULL, high_volt INT2 NOT NULL, protect_low_v INT2 NOT NULL, \
    protect_high_v INT2 NOT NULL, protect_low_i INT2 NOT NULL, protect_high_i INT2 NOT NULL, capacityMode INT1 NOT NULL);";

    const char *sql_cmd_creat_table_status = "CREATE TABLE IF NOT EXISTS "SQLITE_RUNNING_STATUS_TABLE" (id integer PRIMARY KEY AUTOINCREMENT, working_status INT1 NOT NULL, cyc INT NOT NULL, now_cyc INT NOT NULL, \
    sub_now_cyc INT1 NOT NULL, step_sum INT1 NOT NULL, now_step INT1 NOT NULL, run_status INT1 NOT NULL, info INT1 NOT NULL, step_end INT1 NOT NULL, cc_cv INT2 NOT NULL, run_time INT NOT NULL, plat_time INT2 NOT NULL, platform_v INT2 NOT NULL, em INT NOT NULL, \
    cc_capacity INT NOT NULL, capacity INT NOT NULL, discapacity INT NOT NULL, real_capacity INT NOT NULL);";

    create_dir(DB_DATA_FILE_PATH);
    ret = sqlite3_open(SQLITE_HISTROY_DATA_DB, &db);
    sqlite3_exec(db, "PRAGMA synchronous = OFF; ", 0,0,0);

    start_commit();
    if(SQLITE_OK != ret){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create db error\r\n");
        #endif
        sqlite3_close(db);
        return 0;
    }

    ret = sqlite3_exec(db, sql_cmd_creat_table_sys, NULL, NULL, &errmsg);
    if(SQLITE_OK != ret){
        #if SQLITE_DEBUG_CONFIG > 0
        fprintf(stderr, "Create table fail: %s\n", errmsg);
        #endif
        sqlite3_free(errmsg);
        sqlite3_close(db);
        create_table_flag = 1;
        return 1;
    }else{
        #if SQLITE_DEBUG_CONFIG > 0
        printf("create table sucess 1\r\n");
        #endif
        sqlite3_free(errmsg);
    }

    ret = sqlite3_exec(db, sql_cmd_creat_table_status, NULL, NULL, &errmsg);
    if(SQLITE_OK != ret){
        #if SQLITE_DEBUG_CONFIG > 0
        fprintf(stderr, "Create table fail: %s\n", errmsg);
        #endif
        sqlite3_free(errmsg);
        sqlite3_close(db);
        create_table_flag = 1;
        return 1;
    }else{
        #if SQLITE_DEBUG_CONFIG > 0
        printf("create table sucess\r\n");
        #endif
        sqlite3_free(errmsg);
    }
   //
    //创建保存工步的16个表
    for(i = 1; i < STEP_TABLE_MUN + 1; i++){
        creat_step_table(i);
    }

    creat_runningData_table();
    //sqlite3_exec(db, "DELETE FROM sqlite_sequence WHERE name='running_data_table_history';", NULL, NULL, &errmsg);
    //sqlite3_free(errmsg);
    creat_runningData_table_history();
    for(i = 0; i < MAX_NODE; i++){
        bst.node[i].record_id = 0;  //初始化掉线记录条数
    }

    ret = creat_stepconfig_parm();  //创建工步全局变量参数数据表

    if(ret == 0){
        return;
    }

    stop_commit();
    Vacuum_data();  //清sqlite内存
    getNowTime();
    sqlite3_close(db);

    return 1;
}

/******************************************************************
* 函数名: init_sysconfig_opera
* 功能:   初始化系统参数
* 参数：  无
* 返回值: 1 有参数  0 无参数
******************************************************************/
int init_sysconfig_opera()
{
    int openDbFlag = 0;
    const char *search_sysconfig = "SELECT * FROM "SQLITE_SYS_CONFIG_TABLE" where id = 1;";
    sqlite3_stmt *stmt = NULL;

    openDbFlag = open_sqlite_db();

    if(create_table_flag == 1 && openDbFlag == 0){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }

    if(sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL) == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            sys_cfg.lip1 = (u8)sqlite3_column_int(stmt, 1);
            sys_cfg.lip2 = (u8)sqlite3_column_int(stmt, 2);
            sys_cfg.lip3 = (u8)sqlite3_column_int(stmt, 3);
            sys_cfg.lip4 = (u8)sqlite3_column_int(stmt, 4);

            sys_cfg.drip1 = (u8)sqlite3_column_int(stmt, 5);
            sys_cfg.drip2 = (u8)sqlite3_column_int(stmt, 6);
            sys_cfg.drip3 = (u8)sqlite3_column_int(stmt, 7);
            sys_cfg.drip4 = (u8)sqlite3_column_int(stmt, 8);

            sys_cfg.netmask1 = (u8)sqlite3_column_int(stmt, 9);
            sys_cfg.netmask2 = (u8)sqlite3_column_int(stmt, 10);
            sys_cfg.netmask3 = (u8)sqlite3_column_int(stmt, 11);
            sys_cfg.netmask4 = (u8)sqlite3_column_int(stmt, 12);

            sys_cfg.rip1 = (u8)sqlite3_column_int(stmt, 13);
            sys_cfg.rip2 = (u8)sqlite3_column_int(stmt, 14);
            sys_cfg.rip3 = (u8)sqlite3_column_int(stmt, 15);
            sys_cfg.rip4 = (u8)sqlite3_column_int(stmt, 16);

            sys_cfg.rport = (u16)sqlite3_column_int(stmt, 17);

            sys_cfg.led_lighting_mode = (u8)sqlite3_column_int(stmt, 18);
            sys_cfg.low_volt = (u16)sqlite3_column_int(stmt, 19);
            sys_cfg.high_volt = (u16)sqlite3_column_int(stmt, 20);
            sys_cfg.protect_low_v = (u16)sqlite3_column_int(stmt, 21);
            sys_cfg.protect_high_v = (u16)sqlite3_column_int(stmt, 22);
            sys_cfg.protect_low_i = (u16)sqlite3_column_int(stmt, 23);
            sys_cfg.protect_high_i = (u16)sqlite3_column_int(stmt, 24);
            sys_cfg.capacityMode = (u8)sqlite3_column_int(stmt, 25);

           // sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return 1;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

/******************************************************************
* 函数名: set_sys_config_values
* 功能:   设置系统参数
* 参数：  sys_cfg_def 系统参数结构体
* 返回值:
******************************************************************/
int set_sys_config_values(sys_cfg_def sys_cfg_default)
{
    int openDbFlag = 0;
    char *sql = "insert into "SQLITE_SYS_CONFIG_TABLE"(id, lip1, lip2, lip3, lip4, drip1, drip2, drip3, drip4, netmask1, netmask2, netmask3, netmask4 ,rip1, rip2, rip3, rip4, rport, \
        led_lighting_mode, low_volt, high_volt, protect_low_v, protect_high_v, protect_low_i, protect_high_i, capacityMode)values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    sqlite3_stmt *stmt;
    int rc;

    openDbFlag = open_sqlite_db();

    if(create_table_flag == 1 && openDbFlag == 0){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }

    rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("set_sys_config_values error\r\n");
        #endif
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, 1);
    sqlite3_bind_int(stmt, 2, sys_cfg_default.lip1);
    sqlite3_bind_int(stmt, 3, sys_cfg_default.lip2);
    sqlite3_bind_int(stmt, 4, sys_cfg_default.lip3);
    sqlite3_bind_int(stmt, 5, sys_cfg_default.lip4);
    sqlite3_bind_int(stmt, 6, sys_cfg_default.drip1);
    sqlite3_bind_int(stmt, 7, sys_cfg_default.drip2);
    sqlite3_bind_int(stmt, 8, sys_cfg_default.drip3);
    sqlite3_bind_int(stmt, 9, sys_cfg_default.drip4);
    sqlite3_bind_int(stmt, 10, sys_cfg_default.netmask1);
    sqlite3_bind_int(stmt, 11, sys_cfg_default.netmask2);
    sqlite3_bind_int(stmt, 12, sys_cfg_default.netmask3);
    sqlite3_bind_int(stmt, 13, sys_cfg_default.netmask4);
    sqlite3_bind_int(stmt, 14, sys_cfg_default.rip1);
    sqlite3_bind_int(stmt, 15, sys_cfg_default.rip2);
    sqlite3_bind_int(stmt, 16, sys_cfg_default.rip3);
    sqlite3_bind_int(stmt, 17, sys_cfg_default.rip4);
    sqlite3_bind_int(stmt, 18, sys_cfg_default.rport);
    sqlite3_bind_int(stmt, 19, sys_cfg_default.led_lighting_mode);
    sqlite3_bind_int(stmt, 20, sys_cfg_default.low_volt);
    sqlite3_bind_int(stmt, 21, sys_cfg_default.high_volt);
    sqlite3_bind_int(stmt, 22, sys_cfg_default.protect_low_v);
    sqlite3_bind_int(stmt, 23, sys_cfg_default.protect_high_v);
    sqlite3_bind_int(stmt, 24, sys_cfg_default.protect_low_i);
    sqlite3_bind_int(stmt, 25, sys_cfg_default.protect_high_i);
    sqlite3_bind_int(stmt, 26, sys_cfg_default.capacityMode);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}

/******************************************************************
* 函数名: update_net_config_values
* 功能:   更新网络参数
* 参数：  sys_cfg_def 系统参数结构体
* 返回值:
******************************************************************/
int update_net_config_values(sys_cfg_def sys_cfg_default)
{
    int ret;
    char *errmsg;
    char cmd[512] = {0};

    ret = open_sqlite_db();

    if(ret == 0){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("open update_net_config_values db error!\r\n");
        #endif
        sqlite3_free(errmsg);
        return 0;
    }

    sprintf(cmd, "update "SQLITE_SYS_CONFIG_TABLE" set lip1 = %d,lip2 = %d ,lip3 = %d, lip4 = %d, drip1 = %d, drip2 = %d,\
            drip3 = %d, drip4 = %d, netmask1 = %d, netmask2 = %d, netmask3 = %d, netmask4 = %d, rip1 = %d, rip2 = %d, \
            rip3 = %d, rip4 = %d, rport = %d where id = 1;", sys_cfg_default.lip1, sys_cfg_default.lip2, sys_cfg_default.lip3, \
            sys_cfg_default.lip4, sys_cfg_default.drip1, sys_cfg_default.drip2, sys_cfg_default.drip3, sys_cfg_default.drip4, \
            sys_cfg_default.netmask1, sys_cfg_default.netmask2, sys_cfg_default.netmask3, sys_cfg_default.netmask4, sys_cfg_default.rip1, \
            sys_cfg_default.rip2, sys_cfg_default.rip3, sys_cfg_default.rip4, sys_cfg_default.rport);

    sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    sqlite3_free(errmsg);
    close_sqlite_db();
    return 1;
}


/******************************************************************
* 函数名: update_sys_config_values
* 功能:   设置系统参数
* 参数：  sys_cfg_def 系统参数结构体
* 返回值:
******************************************************************/
int update_sys_config_values(sys_cfg_def sys_cfg_default)
{
    char *errmsg;
    char cmd[512];
    int openDbFlag = 0;

    sprintf(cmd, "update "SQLITE_SYS_CONFIG_TABLE" set led_lighting_mode = %d,low_volt = %d, high_volt = %d, protect_low_v = %d, \
        protect_high_v = %d, protect_low_i = %d, protect_high_i = %d, capacityMode = %d where id = 1", sys_cfg_default.led_lighting_mode,sys_cfg_default.low_volt,sys_cfg_default.high_volt, \
        sys_cfg_default.protect_low_v, sys_cfg_default.protect_high_v, sys_cfg_default.protect_low_i, sys_cfg_default.protect_high_i, sys_cfg_default.capacityMode);

    openDbFlag = open_sqlite_db();

    if(create_table_flag == 1 && openDbFlag == 0){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }

    sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    sqlite3_free(errmsg);
    sqlite3_close(db);
    return 1;
}

/******************************************************************
* 函数名: update_step_config_values
* 功能:   更新工步
* 参数：  group 工步表  mun 工步总数
* 返回值:
******************************************************************/
int update_step_config_values(int group, int mun)
{
    int i;
    char *errmsg;
    int nRow;
    char cmd[512];
    sqlite3_stmt *stmt = NULL;
    char STEP_TABLE[STEP_TABLE_MUN + 1][16] = {
        "0",
        "step_table1",
        "step_table2",
        "step_table3",
        "step_table4",
        "step_table5",
        "step_table6",
        "step_table7",
        "step_table8",
        "step_table9",
        "step_table10",
        "step_table11",
        "step_table12",
        "step_table13",
        "step_table14",
        "step_table15",
        "step_table16",
   };

    sprintf(cmd, "select * from %s where cloum = %d;", STEP_TABLE[(group / 16 + 1)], group);
    if(sqlite3_prepare_v2(db, cmd, -1, &stmt, NULL) == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            nRow = sqlite3_column_int(stmt, 0);
            break;
        }
        //sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    for(i = 1; i < (MAX_STEP +1); i ++){
        if(i < mun + 1){
           // printf("group = %d, type = %d id = %d, \r\n", group, bst.node[group].step[i -1].type, i + nRow - 1);
            sprintf(cmd, "update %s set Action_type = %d,step = %d, parameter1 = %d, parameter2 = %d, conditions1 = %d, \
                conditions2 = %d, conditions3 = %d, conditions4 = %d, conditions5 = %d, conditions6 = %d where id = %d;", STEP_TABLE[group / 16 + 1],(bst.node[group].step[i -1].type), (bst.node[group].step_num), \
                (bst.node[group].step[i -1].parm1), (bst.node[group].step[i -1].parm2), (bst.node[group].step[i -1].cond1), (bst.node[group].step[i -1].cond2), (bst.node[group].step[i -1].cond3), \
                (bst.node[group].step[i -1].cond4), (bst.node[group].step[i -1].jump_step), (bst.node[group].step[i -1].sub_cyc), (i + nRow - 1));
            sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
            sqlite3_free(errmsg);
        }else{
           // printf("group = %d, type = %d id = %d, \r\n", group, bst.node[group].step[i -1].type, i + nRow - 1);
            sprintf(cmd, "update %s set Action_type = %d,step = %d, parameter1 = %d, parameter2 = %d, conditions1 = %d, \
                conditions2 = %d, conditions3 = %d, conditions4 = %d, conditions5 = %d, conditions6 = %d where id = %d;", STEP_TABLE[group / 16 + 1], 0xf0 ,0, 0, 0, 0, 0, 0, 0, 0, 0, (i + nRow - 1));
            sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
            sqlite3_free(errmsg);
        }
    }

    return 1;
}

/******************************************************************
* 函数名: insert_step_config_values
* 功能:   将工步插入表中
* 参数：  group 组  mun 工步总数
* 返回值:
******************************************************************/
int insert_step_config_values(int group, int mun)
{
    char *errmsg = NULL;
    char **azResult;
    int nColumn, nRow;

    char sql[512];
    char STEP_TABLE[STEP_TABLE_MUN + 1][16] = {
        "0",
        "step_table1",
        "step_table2",
        "step_table3",
        "step_table4",
        "step_table5",
        "step_table6",
        "step_table7",
        "step_table8",
        "step_table9",
        "step_table10",
        "step_table11",
        "step_table12",
        "step_table13",
        "step_table14",
        "step_table15",
        "step_table16",
   };
    sqlite3_stmt *stmt;
    int rc;
    int i;

    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create insert_step_config_values fail\r\n");
        #endif
        return -1;
    }

    sprintf(sql, "select * from %s;", STEP_TABLE[(group / 16 + 1)]);
    sqlite3_get_table(db, sql, &azResult, &nRow, &nColumn, &errmsg);

    sprintf(sql, "insert into %s (id, Action_type, cloum, step,parameter1, parameter2, conditions1, conditions2, conditions3, conditions4, conditions5, conditions6 )values(?,?,?,?,?,?,?,?,?,?,?,?);",STEP_TABLE[group / 16 + 1]);
    for(i = 1; i < (MAX_STEP+1); i++){
        if(i < mun + 1){        //如果在下发的工步总数范围内则保存相关工步，否则进行对齐补零
            rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
            if(rc != SQLITE_OK){
                printf("insert_step_config_values error\r\n");
                sqlite3_free_table(azResult);
                sqlite3_free(errmsg);
                return 0;
            }
           // printf("bst.node[%d].step[i -1].type = %d\r\n", group, bst.node[group].step[i -1].type);
            sqlite3_bind_int(stmt, 1, i + nRow);
            sqlite3_bind_int(stmt, 2, (bst.node[group].step[i -1].type));
            sqlite3_bind_int(stmt, 3, group);
            sqlite3_bind_int(stmt, 4, mun);
            sqlite3_bind_int(stmt, 5, (bst.node[group].step[i -1].parm1));
            sqlite3_bind_int(stmt, 6, (bst.node[group].step[i -1].parm2));
            sqlite3_bind_int(stmt, 7, (bst.node[group].step[i -1].cond1));
            sqlite3_bind_int(stmt, 8, (bst.node[group].step[i -1].cond2));
            sqlite3_bind_int(stmt, 9, (bst.node[group].step[i -1].cond3));
            sqlite3_bind_int(stmt, 10, (bst.node[group].step[i -1].cond4));
            sqlite3_bind_int(stmt, 11, (bst.node[group].step[i -1].jump_step));
            sqlite3_bind_int(stmt, 12, (bst.node[group].step[i -1].sub_cyc));

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }else{
            rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
            if(rc != SQLITE_OK){
                printf("insert_step_config_values error\r\n");
                sqlite3_free_table(azResult);
                sqlite3_free(errmsg);
                return 0;
            }
           // printf("bst.node[group].step[i -1].type_1 = %d\r\n", bst.node[group].step[i -1].type);
            sqlite3_bind_int(stmt, 1, i + nRow);
            sqlite3_bind_int(stmt, 2, 0xf0);
            sqlite3_bind_int(stmt, 3, group);
            sqlite3_bind_int(stmt, 4, 0);
            sqlite3_bind_int(stmt, 5, 0);
            sqlite3_bind_int(stmt, 6, 0);
            sqlite3_bind_int(stmt, 7, 0);
            sqlite3_bind_int(stmt, 8, 0);
            sqlite3_bind_int(stmt, 9, 0);
            sqlite3_bind_int(stmt, 10,0);
            sqlite3_bind_int(stmt, 11, 0);
            sqlite3_bind_int(stmt, 12, 0);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    sqlite3_free_table(azResult);
    sqlite3_free(errmsg);
    return 1;
}

/******************************************************************
* 函数名: save_step_mun
* 功能:   保存对应组的工步
* 参数：  mun 相应的组 setp 工步总数
* 返回值:
******************************************************************/
int save_step_mun(int mun, int step)
{
//    insert_step_replace_values(mun, step);
#if 1
    int opera_flag = 0;
    int ret;
    char STEP_TABLE[STEP_TABLE_MUN + 1][16] = {
            "0",
            "step_table1",
            "step_table2",
            "step_table3",
            "step_table4",
            "step_table5",
            "step_table6",
            "step_table7",
            "step_table8",
            "step_table9",
            "step_table10",
            "step_table11",
            "step_table12",
            "step_table13",
            "step_table14",
            "step_table15",
            "step_table16",
   };
    char search_sysconfig[256];
    sqlite3_stmt *stmt = NULL;

    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }
    sprintf(search_sysconfig,"SELECT * FROM %s where cloum = %d;", STEP_TABLE[(mun / 16 + 1)],mun);
    ret = sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL);
    if( ret == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            if(sqlite3_column_int(stmt, 2) == mun){
                opera_flag = 2;    //有数据 进行更新操作
                break;
            }else{
                opera_flag = 1;    //没有数据 进行插入
            break;
            }
        }
        if(opera_flag != 2){
            opera_flag = 1;     //没有数据 进行插入
        }
        //sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    if(opera_flag == 1){
        insert_step_config_values(mun, step);
    }else if(opera_flag == 2){
        update_step_config_values(mun, step);
    }

    return 1;
    #endif
}

/******************************************************************
* 函数名: recovery_step
* 功能:   恢复工步
* 参数：  mun 相应的组
* 返回值:
******************************************************************/
int recovery_step(int mun)
{
    int i = 0;
    int get_flag = 0;
    char STEP_TABLE[STEP_TABLE_MUN + 1][16] = {
        "0",
        "step_table1",
        "step_table2",
        "step_table3",
        "step_table4",
        "step_table5",
        "step_table6",
        "step_table7",
        "step_table8",
        "step_table9",
        "step_table10",
        "step_table11",
        "step_table12",
        "step_table13",
        "step_table14",
        "step_table15",
        "step_table16",
    };
    char search_sysconfig[128];
    sprintf(search_sysconfig, "SELECT * FROM %s  where cloum = %d;", STEP_TABLE[(mun / 16 + 1)], mun);
    sqlite3_stmt *stmt = NULL;

    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }

    if(sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL) == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            bst.node[mun].step[i].type = (sqlite3_column_int(stmt, 1));
           //printf("type = %d\r\n", bst.node[mun].step[i].type);
            if(get_flag == 0){
                get_flag = 1;
                bst.node[mun].step_num = (sqlite3_column_int(stmt, 3));
              //  printf("bst.node[%d].step_num = %d\r\n", mun, bst.node[mun].step_num);
            }
            bst.node[mun].step[i].parm1     = (sqlite3_column_int(stmt, 4));
            bst.node[mun].step[i].parm2     = (sqlite3_column_int(stmt, 5));
            bst.node[mun].step[i].cond1     = (sqlite3_column_int(stmt, 6));
            bst.node[mun].step[i].cond2     = (sqlite3_column_int(stmt, 7));
            bst.node[mun].step[i].cond3     = (sqlite3_column_int(stmt, 8));
            bst.node[mun].step[i].cond4     = (sqlite3_column_int(stmt, 9));
            bst.node[mun].step[i].jump_step = (sqlite3_column_int(stmt, 10));
            bst.node[mun].step[i].sub_cyc   = (sqlite3_column_int(stmt, 11));

            if(i > 10)
            {
                break;
            }
            i++;

        }
      //  sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    if(i != 0){
        return 0;
    }

    return 1;
}

/******************************************************************
* 函数名: read_step_run_status
* 功能:   读取工步状态数据
* 参数：
* 返回值:
******************************************************************/
void read_step_run_status(void)
{
	u16 group;
	int ret;
    int openDbFlag = 0;

    openDbFlag = open_sqlite_db();
    printf("openDbFlag = %d\r\n", openDbFlag);

    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return;
    }

	for(group = 0; group < MAX_NODE;group++)
	{
        ret = recovery_step(group);
	}
	sqlite3_close(db);
}

/******************************************************************
* 函数名: save_global_group_data
* 功能:   保存组的全局变量
* 参数：  int group 保存的组
* 返回值:
******************************************************************/
int save_global_group_data(int group)
{
    char sql_cmd_global[384];
    char *errmsg = NULL;
    int ret, opera_flag;

    sqlite3_stmt *stmt = NULL;
    int h, l;
    char **azResult;

    memset(sql_cmd_global, 0, 384);
    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }

    sprintf(sql_cmd_global, "SELECT * FROM "SQLITE_RUNNING_STATUS_TABLE" WHERE ID = %d;", group + 1);
    sqlite3_get_table(db, sql_cmd_global, &azResult, &h, &l, &errmsg);
    if(h != 0 && l != 0){
        opera_flag = 2;    //有数据 进行更新操作
    }else{
        opera_flag = 1;     //没有数据 进行插入
    }
    sqlite3_free_table(azResult);
    sqlite3_free(errmsg);
    memset(sql_cmd_global, 0, 384);

    if(opera_flag == 2){      //更新数据处理
        sprintf(sql_cmd_global, "update "SQLITE_RUNNING_STATUS_TABLE" set working_status = %d, cyc = %d, now_cyc = %d, sub_now_cyc = %d,\
            step_sum = %d, now_step = %d, run_status = %d, info = %d, step_end = %d,cc_cv = %d, run_time = %d, plat_time = %d, platform_v = %d, \
            em = %d, cc_capacity = %d, capacity = %d, discapacity = %d, real_capacity = %d where id = %d", \
            save_real_data[group].working_status, save_real_data[group].cyc, save_real_data[group].now_cyc, save_real_data[group].cyc_son, save_real_data[group].step_sum, save_real_data[group].step_mun,  \
            save_real_data[group].run_status, save_real_data[group].info, save_real_data[group].step_end, save_real_data[group].cc_cv, save_real_data[group].run_time, \
            save_real_data[group].plat_time, save_real_data[group].platform_v, save_real_data[group].em, save_real_data[group].cc_capacity, \
            save_real_data[group].capacity, save_real_data[group].discapacity, save_real_data[group].real_capacity, (group+1));
            sqlite3_exec(db, sql_cmd_global, NULL, NULL, &errmsg);
            sqlite3_free(errmsg);
            memset(sql_cmd_global, 0, 384);
    }else if(opera_flag == 1){  //插入数据处理
        sprintf(sql_cmd_global, "insert into  "SQLITE_RUNNING_STATUS_TABLE"(id , working_status, cyc , now_cyc , sub_now_cyc, step_sum, now_step, run_status , info , step_end, \
          cc_cv, run_time, plat_time, platform_v, em, cc_capacity, capacity, discapacity, real_capacity )values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        ret = sqlite3_prepare(db, sql_cmd_global, strlen(sql_cmd_global), &stmt, NULL);
        memset(sql_cmd_global, 0, 384);

        if(ret != SQLITE_OK){
            printf("save_global_group_data error\r\n");
            return 0;
        }

        sqlite3_bind_int(stmt, 1,   group + 1);
        sqlite3_bind_int(stmt, 2,   save_real_data[group].working_status);
        sqlite3_bind_int(stmt, 3,   save_real_data[group].cyc);
        sqlite3_bind_int(stmt, 4,   save_real_data[group].now_cyc);
        sqlite3_bind_int(stmt, 5,   save_real_data[group].cyc_son);
        sqlite3_bind_int(stmt, 6,   save_real_data[group].step_sum);
        sqlite3_bind_int(stmt, 7,   save_real_data[group].step_mun);
        sqlite3_bind_int(stmt, 8,   save_real_data[group].run_status);
        sqlite3_bind_int(stmt, 9,   save_real_data[group].info);
        sqlite3_bind_int(stmt, 10,  save_real_data[group].step_end);
        sqlite3_bind_int(stmt, 11,  save_real_data[group].cc_cv);
        sqlite3_bind_int(stmt, 12,  save_real_data[group].run_time);
        sqlite3_bind_int(stmt, 13,  save_real_data[group].plat_time);
        sqlite3_bind_int(stmt, 14,  save_real_data[group].platform_v);
        sqlite3_bind_int(stmt, 15,  save_real_data[group].em);
        sqlite3_bind_int(stmt, 16,  save_real_data[group].cc_capacity);
        sqlite3_bind_int(stmt, 17,  save_real_data[group].capacity);
        sqlite3_bind_int(stmt, 18,  save_real_data[group].discapacity);
        sqlite3_bind_int(stmt, 19,  save_real_data[group].real_capacity);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    memset(sql_cmd_global, 0, 384);
    return 1;
}

/******************************************************************
* 函数名: insert_running_data
* 功能:   插入要保存的运行数据
* 参数：  group 保存的组 x 模块 y 模块对应的组
* 返回值:
*******************************************************************/
int insert_running_data(int group)
{
    int ret;
    u8 opera_flag;
    u32 capacity;
    sqlite3_stmt *stmt;
    char *errmsg;
    char cmd[320];
    int h, l;
    char **azResult;

    if(bst.node[group].work_type == CV || bst.node[group].work_type == HCV){
        capacity = (save_real_data[group].capacity );
    }else if(bst.node[group].work_type == DCV){
        capacity = (save_real_data[group].discapacity );
    }else{
        capacity = 0;
    }

    sprintf(cmd, "SELECT * FROM "RUNNING_DATA_CURRENT" WHERE ID = %d;", group+1);
    sqlite3_get_table(db, cmd, &azResult, &h, &l, &errmsg);
    if(h != 0 && l != 0){
        opera_flag = 2;    //有数据 进行更新操作
    }else{
        opera_flag = 1;     //没有数据 进行插入
    }
    sqlite3_free_table(azResult);
    sqlite3_free(errmsg);

    /*
    sprintf(cmd, "SELECT * FROM "RUNNING_DATA_CURRENT" WHERE ID = %d;", group+1);

    if(sqlite3_prepare_v2(db, cmd, -1, &stmt, NULL) == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
           opera_flag = 2;    //有数据 进行更新操作
           break;
        }
        if(opera_flag != 2){
            opera_flag = 1;     //没有数据 进行插入
        }
    }*/

    if(opera_flag == 2){      //更新数据处理
        sprintf(cmd, "update "RUNNING_DATA_CURRENT" set cyc = %d, sonCyc = %d, now_step = %d, work_type = %d,\
            v = %d, i = %d, capacity = %d, em = %d,cc_cv = %d, platform_time = %d, time = %d, node_status = %d, rflag = %d, rio = %d where id = %d;", \
        save_real_data[group].cyc, save_real_data[group].cyc_son, save_real_data[group].step_mun, save_real_data[group].step_style, save_real_data[group].v, save_real_data[group].i, capacity, save_real_data[group].em, save_real_data[group].cc_cv,\
        save_real_data[group].plat_time, save_real_data[group].run_time, save_real_data[group].node_status, save_real_data[group].r_flag,save_real_data[group].r_rio,(group+1));
        sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
        sqlite3_free(errmsg);

    }else if(opera_flag == 1){  //插入数据处理
        sprintf(cmd, "insert into  "RUNNING_DATA_CURRENT" (id, cyc, sonCyc, now_step, work_type, v, i, capacity, em, cc_cv, platform_time, time, node_status, \
            rflag, rio)values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        ret = sqlite3_prepare(db, cmd, strlen(cmd), &stmt, NULL);
        if(ret != SQLITE_OK){
            printf("save_global_group_data error_1\r\n");
           // sqlite3_close(db);
            return 0;
        }
        sqlite3_bind_int(stmt, 1,  group + 1);
        sqlite3_bind_int(stmt, 2,  save_real_data[group].cyc);
        sqlite3_bind_int(stmt, 3,  save_real_data[group].cyc_son);
        sqlite3_bind_int(stmt, 4,  save_real_data[group].step_mun);
        sqlite3_bind_int(stmt, 5,  save_real_data[group].step_style);
        sqlite3_bind_int(stmt, 6,  save_real_data[group].v);
        sqlite3_bind_int(stmt, 7,  save_real_data[group].i);
        sqlite3_bind_int(stmt, 8,  capacity);
        sqlite3_bind_int(stmt, 9,  save_real_data[group].em);
        sqlite3_bind_int(stmt, 10, save_real_data[group].cc_cv);
        sqlite3_bind_int(stmt, 11, save_real_data[group].plat_time);
        sqlite3_bind_int(stmt, 12, save_real_data[group].run_time);
        sqlite3_bind_int(stmt, 13, save_real_data[group].node_status);
        sqlite3_bind_int(stmt, 14, save_real_data[group].r_flag);
        sqlite3_bind_int(stmt, 15, save_real_data[group].r_rio);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return 1;
}

/******************************************************************
* 函数名: insert_running_history_data
* 功能:   插入要保存的运行数据
* 参数：  group 保存的组 x 对应标号的电池
* 返回值:
******************************************************************/
int insert_running_history_data(int group)
{
    u32 capacity;
    int ret;
    sqlite3_stmt *stmt;
    char *errmsg;
    char cmd[320];

    if(bst.node[group].work_type == CV || bst.node[group].work_type == HCV){
        capacity = (save_real_data[group].capacity / 3600);
    }else if(bst.node[group].work_type == DCV){
        capacity = (save_real_data[group].discapacity / 3600);
    }else{
        capacity = 0;
    }

    //如果表内的记录超过一定的条数 则进行删除
    if(history_data_mun > 240000){
        history_data_mun = 1;
        bst.node[group].record_id = 1;
        sprintf(cmd, "delete from "RUNNING_DATA_HISTORY"");
        sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
        sqlite3_free(errmsg);
    }else{
       // bst.node[group].record_id++;
    }

    sprintf(cmd, "insert into %s (id, x, cyc, sonCyc, now_step, work_type, v, i, capacity, em, cc_cv, \
        platform_time, time, node_status, rflag, rio)values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", RUNNING_DATA_HISTORY);

    ret = sqlite3_prepare(db, cmd, strlen(cmd), &stmt, NULL);
    if(ret != SQLITE_OK){
        printf("insert_running_data error\r\n");
        return 0;
    }
    sqlite3_bind_int(stmt, 1,  history_data_mun);
    sqlite3_bind_int(stmt, 2,  group + 1);
    sqlite3_bind_int(stmt, 3,  save_real_data[group].cyc);
    sqlite3_bind_int(stmt, 4,  save_real_data[group].cyc_son);
    sqlite3_bind_int(stmt, 5,  save_real_data[group].step_mun);
    sqlite3_bind_int(stmt, 6,  save_real_data[group].step_style);
    sqlite3_bind_int(stmt, 7,  save_real_data[group].v);
    sqlite3_bind_int(stmt, 8,  save_real_data[group].i);
    sqlite3_bind_int(stmt, 9,  save_real_data[group].charge);
    sqlite3_bind_int(stmt, 10, save_real_data[group].em);
    sqlite3_bind_int(stmt, 11, save_real_data[group].cc_cv);
    sqlite3_bind_int(stmt, 12, save_real_data[group].plat_time);
    sqlite3_bind_int(stmt, 13, save_real_data[group].run_time);
    sqlite3_bind_int(stmt, 14, save_real_data[group].node_status);
    sqlite3_bind_int(stmt, 15, save_real_data[group].r_flag);
    sqlite3_bind_int(stmt, 16, save_real_data[group].r_rio);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    history_data_mun++;
    return 0;
}

/******************************************************************
* 函数名: recovery_global
* 功能:   恢复全局变量参数
* 参数：  mun 恢复的组
* 返回值:
******************************************************************/
int recovery_global(int mun)
{
    char search_sysconfig[128];
    sprintf(search_sysconfig, "SELECT * FROM %s WHERE id = %d;", SQLITE_RUNNING_STATUS_TABLE, mun+1);
    sqlite3_stmt *stmt = NULL;

    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }

    if(sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL) == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            save_real_data[mun].working_status    =  sqlite3_column_int(stmt, 1);
            bst.node[mun].cyc                     =  sqlite3_column_int(stmt, 2);
            bst.node[mun].now_cyc           =  sqlite3_column_int(stmt, 3);
            bst.node[mun].sub_now_cyc       =  sqlite3_column_int(stmt, 4);
            //bst.node[mun].step_num          =  sqlite3_column_int(stmt, 5);//工步恢复的时候会计算工步总数 ，这边不用处理
            bst.node[mun].now_step          =  sqlite3_column_int(stmt, 6);
            bst.node[mun].run_status        =  sqlite3_column_int(stmt, 7);
            bst.node[mun].info              =  sqlite3_column_int(stmt, 8);
            bst.node[mun].step_end          =  sqlite3_column_int(stmt, 9);
            bst.node[mun].cc_cv             =  sqlite3_column_int(stmt, 10);
            bst.node[mun].time              =  sqlite3_column_int(stmt, 11);
            bst.node[mun].platform_time     =  sqlite3_column_int(stmt, 12);
            bst.node[mun].platform_v        =  sqlite3_column_int(stmt, 13);
            bst.node[mun].em                =  sqlite3_column_int(stmt, 14);
            bst.node[mun].cc_capacity       =  sqlite3_column_int(stmt, 15);
            bst.node[mun].capacity          =  sqlite3_column_int(stmt, 16) * 3600;
            bst.node[mun].discapacity       =  sqlite3_column_int(stmt, 17) * 3600;
            bst.node[mun].real_capacity     =  sqlite3_column_int(stmt, 18);
            bst.node[mun].time_tmp_1        =  bst.node[mun].time;
        }
    }else{
            bst.node[mun].time_tmp_1        =   0;
    }
   // printf("save_real_data[mun].working_status = %x\r\n", save_real_data[mun].working_status);
    if(save_real_data[mun].working_status == 0xAA){
            bst.node[mun].run_status = SYS_PAUSE;       //如果上一次为完成工步 则重启后置为暂停
            bst.node[mun].node_status = 5;
            bst.node[mun].work_type = REST;
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 1;
}

/******************************************************************
* 函数名: recovery_runningData
* 功能:   恢复全局变量参数
* 参数：  mun 恢复的组
* 返回值:
******************************************************************/
int recovery_runningData(int mun)
{
    int x, y; //38
    char search_sysconfig[128];
    char RDATA_TABLE[9][24] = {
        "0",
        "running_data_table_1",
        "running_data_table_2",
        "running_data_table_3",
        "running_data_table_4",
        "running_data_table_5",
        "running_data_table_6",
        "running_data_table_7",
        "running_data_table_8",
    };

    sprintf(search_sysconfig, "SELECT * FROM %s ;", RDATA_TABLE[mun]);
    sqlite3_stmt *stmt = NULL;

    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        sqlite3_close(db);
        return -1;
    }
    if(sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL) == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            x                                                            = (u8)sqlite3_column_int(stmt, 1);
            y                                                            = (u8)sqlite3_column_int(stmt, 2);
            /*
            Global_info[mun].module_status[x].FWsta[y]                   = (u16)sqlite3_column_int(stmt, 3);
            Global_info[mun].module_status[x].node_info[y]               = (u8)sqlite3_column_int(stmt, 4);
            Global_info[mun].module_status[x].v[y]                       = (u16)sqlite3_column_int(stmt, 5);
            Global_info[mun].module_status[x].i[y]                       = (u16)sqlite3_column_int(stmt, 6);
            Global_info[mun].module_status[x].Btime[y]                   = (u16)sqlite3_column_int(stmt, 7);
            Global_info[mun].module_status[x].tCC[y]                     = sqlite3_column_int(stmt, 8);
            Global_info[mun].module_status[x].capacity[y]                = sqlite3_column_int(stmt, 9);
            Global_info[mun].module_status[x].discapacity[y]             = sqlite3_column_int(stmt, 10);
            Global_info[mun].module_status[x].DisCharge_Platform_Time[y] = (u16)sqlite3_column_int(stmt, 11);
            Global_info[mun].module_status[x].node_status[y]             = (u8)sqlite3_column_int(stmt, 12);
        */
        }
    }
    return 1;
}

/******************************************************************
* 函数名: recovery_gloabal_info
* 功能:   插入要保存的运行数据
* 参数：  恢复全局变量参数信息
* 返回值:
******************************************************************/
int recovery_gloabal_info()
{
    int i;
    int openDbFlag = 0;

    openDbFlag = open_sqlite_db();

    if(openDbFlag == 0){
        return -1;
    }
    for(i = 0; i < MAX_NODE; i++){
        recovery_global(i);
    }

    bst.net_status = 0x01;    //默认网络在线
    sqlite3_close(db);
    return 0;
}

/******************************************************************
* 函数名: delete_step_table
* 功能:   删除相应保存工步表中的数据
* 参数：  mun 电池编号
* 返回值:
******************************************************************/
void delete_step_table(int mun)
{
    char search_sysconfig[128];
    char STEP_TABLE[STEP_TABLE_MUN + 1][16] = {
        "0",
        "step_table1",
        "step_table2",
        "step_table3",
        "step_table4",
        "step_table5",
        "step_table6",
        "step_table7",
        "step_table8",
        "step_table9",
        "step_table10",
        "step_table11",
        "step_table12",
        "step_table13",
        "step_table14",
        "step_table15",
        "step_table16",
    };


    printf("delete_step_table = %d\r\n", mun);
    memset(search_sysconfig, 0, 128);
    sprintf(search_sysconfig, "DELETE FROM %s WHERE cloum=%d;", STEP_TABLE[(mun / 16 + 1)], mun);
    sqlite3_exec(db,search_sysconfig,0,0,0);

}

/******************************************************************
* 函数名: insert_step_config_values
* 功能:   将工步插入表中(存在则更新)
* 参数：  group 组  mun 工步总数
* 返回值:
******************************************************************/
int insert_step_replace_values(int group, int mun)
{
    char *errmsg = NULL;
    char **azResult;
    int nColumn, nRow;

    char sql[512];
    char STEP_TABLE[STEP_TABLE_MUN + 1][16] = {
        "0",
        "step_table1",
        "step_table2",
        "step_table3",
        "step_table4",
        "step_table5",
        "step_table6",
        "step_table7",
        "step_table8",
        "step_table9",
        "step_table10",
        "step_table11",
        "step_table12",
        "step_table13",
        "step_table14",
        "step_table15",
        "step_table16",
   };
    sqlite3_stmt *stmt;
    int rc;
    int i;

    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create insert_step_config_values fail\r\n");
        #endif
        return -1;
    }

    sprintf(sql, "select * from %s;", STEP_TABLE[(group / 16 + 1)]);
    sqlite3_get_table(db, sql, &azResult, &nRow, &nColumn, &errmsg);

    sprintf(sql, "insert or replace into %s (id, Action_type, cloum, step, parameter1, parameter2, conditions1, conditions2, conditions3, conditions4, conditions5, conditions6 )values(?,?,?,?,?,?,?,?,?,?,?,?)",STEP_TABLE[group / 16 + 1]);
    for(i = 1; i < (MAX_STEP+1); i++){
        if(i < mun + 1){        //如果在下发的工步总数范围内则保存相关工步，否则进行对齐补零
            rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
            if(rc != SQLITE_OK){
                printf("insert_step_config_values error\r\n");
                sqlite3_close(db);
                //sqlite3_step(stmt);
                sqlite3_finalize(stmt);
                return 0;
            }
            printf("bst.node[%d].step[i -1].type = %d\r\n", group, bst.node[group].step[i -1].type);
            sqlite3_bind_int(stmt, 1, i + nRow);
            sqlite3_bind_int(stmt, 2, (bst.node[group].step[i -1].type));
            sqlite3_bind_int(stmt, 3, group);
            sqlite3_bind_int(stmt, 4, mun);
            sqlite3_bind_int(stmt, 5, htons(bst.node[group].step[i -1].parm1));
            sqlite3_bind_int(stmt, 6, htons(bst.node[group].step[i -1].parm2));
            sqlite3_bind_int(stmt, 7, htons(bst.node[group].step[i -1].cond1));
            sqlite3_bind_int(stmt, 8, htons(bst.node[group].step[i -1].cond2));
            sqlite3_bind_int(stmt, 9, htons(bst.node[group].step[i -1].cond3));
            sqlite3_bind_int(stmt, 10, htons(bst.node[group].step[i -1].cond4));
            sqlite3_bind_int(stmt, 11, (bst.node[group].step[i -1].jump_step));
            sqlite3_bind_int(stmt, 12, (bst.node[group].step[i -1].sub_cyc));

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }else{
            rc = sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);
            if(rc != SQLITE_OK){
                printf("insert_step_config_values error\r\n");
                sqlite3_close(db);
                //sqlite3_step(stmt);
                sqlite3_finalize(stmt);
                return 0;
            }

            sqlite3_bind_int(stmt, 1, i + nRow);
            sqlite3_bind_int(stmt, 2, 0x0f);
            sqlite3_bind_int(stmt, 3, group);
            sqlite3_bind_int(stmt, 4, mun);
            sqlite3_bind_int(stmt, 5, 0);
            sqlite3_bind_int(stmt, 6, 0);
            sqlite3_bind_int(stmt, 7, 0);
            sqlite3_bind_int(stmt, 8, 0);
            sqlite3_bind_int(stmt, 9, 0);
            sqlite3_bind_int(stmt, 10,0);
            sqlite3_bind_int(stmt, 11, 0);
            sqlite3_bind_int(stmt, 12, 0);

            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    sqlite3_free_table(azResult);
    sqlite3_free(errmsg);
    return 1;
}

/******************************************************************
* 函数名: start_commit
* 功能:   开始事务
* 参数：
* 返回值:
******************************************************************/
void start_commit()
{
    sqlite3_exec(db,"BEGIN;",0,0,0);
}

/******************************************************************
* 函数名: stop_commit
* 功能:   结束事务
* 参数：
* 返回值:
******************************************************************/
void stop_commit()
{
    sqlite3_exec(db,"COMMIT;",0,0,0);
}

/******************************************************************
* 函数名: delete_bat_runningdata
* 功能:   删除节点的运行数据的运行状态
* 参数：
* 返回值:
******************************************************************/
void delete_bat_runningdata(int group)
{
    char cmd[256];
    char *errmsg = NULL;
    int ret;

    sprintf(cmd, "DELETE FROM "RUNNING_DATA_CURRENT" WHERE id=%d", (group + 1));
    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(ret != SQLITE_OK){
        printf("delete running data error!\r\n");
    }

    usleep(5 * 1000);

    sprintf(cmd, "DELETE FROM "SQLITE_RUNNING_STATUS_TABLE" WHERE id=%d", (group + 1));
    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(ret != SQLITE_OK){
        printf("delete running statues error!\r\n");
    }

    sprintf(cmd, "DELETE FROM "RUNNING_DATA_HISTORY"");
    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(ret != SQLITE_OK){
        printf("delete history data  error!\r\n");
    }

    sqlite3_free(errmsg);
}

void delete_bat_runningdata_table()
{
    char cmd[256];
    char *errmsg;
    int ret;

    sprintf(cmd, "DELETE FROM "RUNNING_DATA_CURRENT"");

    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(ret != SQLITE_OK){
        printf("delete running data error!\r\n");
    }

    usleep(5 * 1000);

    sprintf(cmd, "DELETE FROM "SQLITE_RUNNING_STATUS_TABLE"");
    sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(ret != SQLITE_OK){
        printf("delete running statues error!\r\n");
    }

    sprintf(cmd, "DELETE FROM "RUNNING_DATA_HISTORY"");
    sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(ret != SQLITE_OK){
        printf("delete history data  error!\r\n");
    }
    sqlite3_free(errmsg);
}

/******************************************************************
* 函数名: open_sqlite_db
* 功能:   打开数据库
* 参数：
* 返回值:
******************************************************************/
int open_sqlite_db()
{
    int ret;

    ret = sqlite3_open(SQLITE_HISTROY_DATA_DB, &db);

    if(SQLITE_OK != ret){
        sqlite3_close(db);
        return 0;
    }

    return 1;
}

/******************************************************************
* 函数名: close_sqlite_db
* 功能:   关闭数据库
* 参数：
* 返回值:
******************************************************************/
void close_sqlite_db()
{
    //sqlite3_exec(history_data_db,"COMMIT;",0,0,0);
   // sqlite3_close(history_data_db);
    //printf("close db\r\n");
    sqlite3_close(db);
}

void Vacuum_data()
{
    char *errmsg;
    char cmd[16];
    int ret;

    sprintf(cmd, "VACUUM");
    ret = sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    if(ret != SQLITE_OK){
        printf("delete VACUUM  error!\r\n");
    }
    sqlite3_free(errmsg);
}

/******************************************************************
* 函数名: netOffSaveData
* 功能:   掉线时保存历史数据
* 参数：  int group 保存的组
* 返回值:
******************************************************************/
int net_off_save_data()
{
    u8 j;
    static int i = 0;
    char sql_cmd_global[384];
    int ret, opera_flag;
    sqlite3_stmt *stmt = NULL;

    memset(sql_cmd_global, 0, 384);
    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }

    sprintf(sql_cmd_global, "insert into %s (x, cyc, sonCyc, now_step, work_type, v, i, capacity, em, cc_cv, \
        platform_time, time, node_status, rflag, rio)values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", RUNNING_DATA_HISTORY);

    ret = sqlite3_prepare_v2(db, sql_cmd_global, strlen(sql_cmd_global), &stmt, NULL);
    if(ret != SQLITE_OK){
        printf("insert_running_data error\r\n");
        sqlite3_finalize(stmt);
        return 0;
    }

    for(j = 0; j < MAX_NODE; j++){
        if((bst.node[j].run_status == SYS_RUNNING && bst.node[j].step_end != 1)){
            for(i = 0; i < 10; i ++){
                sqlite3_bind_int(stmt, 1,  j + 1);
                sqlite3_bind_int(stmt, 2,  save_history_data[j][i].cyc);
                sqlite3_bind_int(stmt, 3,  save_history_data[j][i].sub_now_cyc);
                sqlite3_bind_int(stmt, 4,  save_history_data[j][i].now_step);
                sqlite3_bind_int(stmt, 5,  save_history_data[j][i].work_type);
                sqlite3_bind_int(stmt, 6,  save_history_data[j][i].v);
                sqlite3_bind_int(stmt, 7,  save_history_data[j][i].i);
                sqlite3_bind_int(stmt, 8,  save_history_data[j][i].charge);
                sqlite3_bind_int(stmt, 9,  save_history_data[j][i].em);
                sqlite3_bind_int(stmt, 10, save_history_data[j][i].cc_cv);
                sqlite3_bind_int(stmt, 11, save_history_data[j][i].platform_time);
                sqlite3_bind_int(stmt, 12, save_history_data[j][i].time);
                sqlite3_bind_int(stmt, 13, save_history_data[j][i].node_status);
                sqlite3_bind_int(stmt, 14, save_history_data[j][i].recharge_status);
                sqlite3_bind_int(stmt, 15, save_history_data[j][i].recharge_ratio);

                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
        }
    }

    sqlite3_finalize(stmt);
    memset(sql_cmd_global, 0, 384);
    save_history_flag = 0;
    return 1;
}

/******************************************************************
* 函数名: insert_stepcfg_fun
* 功能:   插入工步全局变量
* 参数：  u8 node 插入的节点
* 返回值:
******************************************************************/
void insert_stepcfg_fun(u8 node)
{
    char *errmsg = NULL;
    char sql[512];

    sqlite3_stmt *stmt;
    int rc;
    int i;

    if(create_table_flag == 1 || node > MAX_NODE){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("open insert_stepcfg_fun fail\r\n");
        #endif
        return -1;
    }

    sprintf(sql, "insert into step_cfg_parm_table (node, mainCyc, platV, testStyle, divcapStartStep, divcapEndStep, \
            rechargeStartStep, recharge_end_step, rechargeRatio, rechargeCap)values(?,?,?,?,?,?,?,?,?,?);");

    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    printf("rc = %d\r\n", rc);
    if(rc != SQLITE_OK){
        printf("insert_stepcfg_fun error\r\n");
        return 0;
    }

   // for(i = 0; i < (node+1); i++){
        sqlite3_bind_int(stmt, 1, node);
        sqlite3_bind_int(stmt, 2, (bst.node[node].cyc));
        sqlite3_bind_int(stmt, 3, (bst.node[node].platform_v));
        sqlite3_bind_int(stmt, 4, (bst.node[node].test_style));
        sqlite3_bind_int(stmt, 5, (bst.node[node].divcap_start_step));
        sqlite3_bind_int(stmt, 6, (bst.node[node].divcap_end_step));
        sqlite3_bind_int(stmt, 7, (bst.node[node].recharge_start_step));
        sqlite3_bind_int(stmt, 8, (bst.node[node].recharge_end_step));
        sqlite3_bind_int(stmt, 9, (bst.node[node].recharge_ratio));
        sqlite3_bind_int(stmt, 10, (bst.node[node].recharge_cap));

        sqlite3_step(stmt);
       // sqlite3_reset(stmt);
   // }
    sqlite3_finalize(stmt);
    sqlite3_free(errmsg);
    return 1;
}

/******************************************************************
* 函数名: update_stepcfg_fun
* 功能:   更新工步全局变量
* 参数：  u8 node 更新的节点
* 返回值:
******************************************************************/
void update_stepcfg_fun(u8 node)
{
    int i;
    char *errmsg;
    int nRow;
    char cmd[512];

    if(create_table_flag == 1 || node > MAX_NODE){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Open table fail\r\n");
        #endif
        return -1;
    }

    sprintf(cmd, "update step_cfg_parm_table set mainCyc = %d,platV = %d, testStyle = %d, divcapStartStep = %d, divcapEndStep = %d, \
            rechargeStartStep = %d, recharge_end_step = %d, rechargeRatio = %d, rechargeCap = %d where node = %d;", (bst.node[node].cyc), (bst.node[node].platform_v), \
            (bst.node[node].test_style), (bst.node[node].divcap_start_step), (bst.node[node].divcap_end_step), (bst.node[node].recharge_start_step), (bst.node[node].recharge_end_step), \
            (bst.node[node].recharge_ratio), (bst.node[node].recharge_cap), node);
    sqlite3_exec(db, cmd, NULL, NULL, &errmsg);
    sqlite3_free(errmsg);

    return 1;
}

/******************************************************************
* 函数名: save_stepcfg_fun
* 功能:   保存工步全局变量配置到数据库
* 参数：  u8 node 保存的节点
* 返回值:
******************************************************************/
void save_stepcfg_fun(u8 node)
{
    int opera_flag = 0;
    int ret;

    char search_sysconfig[256];
    sqlite3_stmt *stmt = NULL;

    if(create_table_flag == 1){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("Create table fail\r\n");
        #endif
        return -1;
    }
    sprintf(search_sysconfig,"SELECT * FROM step_cfg_parm_table where node = %d;", node);
    ret = sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL);
    if( ret == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            if(sqlite3_column_int(stmt, 1) == node){
                printf("update\r\n");
                opera_flag = 2;    //有数据 进行更新操作
                break;
            }else{
                printf("insert\r\n");
                opera_flag = 1;    //没有数据 进行插入
            break;
            }
        }
        if(opera_flag != 2){
            opera_flag = 1;     //没有数据 进行插入
        }
      // sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    if(opera_flag == 1){
        insert_stepcfg_fun(node);
    }else if(opera_flag == 2){
        update_stepcfg_fun(node);
    }

    return 1;
}

/******************************************************************
* 函数名: recover_stepcfg_fun
* 功能:   恢复工步全局变量参数
* 参数：
* 返回值:
******************************************************************/
void recover_stepcfg_fun()
{
    int i;
    int ret = 0;
    char *errmsg;
    char search_sysconfig[128];
    sqlite3_stmt *stmt = NULL;

    ret = open_sqlite_db();

    if(ret == 0){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("open recover_stepcfg db error!");
        #endif
        return;
    }

    ret = sqlite3_exec(db, "select node from step_cfg_parm_table order by node asc;", NULL, NULL, &errmsg);
    if(SQLITE_OK != ret){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("asc recover_stepcfg db error!\r\n");
        #endif
        sqlite3_free(errmsg);
        close_sqlite_db();
        return;
    }

    for(i = 0; i < MAX_NODE; i++)
    {
        sprintf(search_sysconfig, "SELECT * FROM  step_cfg_parm_table where node = %d;", i);

        if(sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL) == SQLITE_OK){
            while(sqlite3_step(stmt) == SQLITE_ROW){
                bst.node[i].cyc                 = (sqlite3_column_int(stmt, 2));
                bst.node[i].platform_v          = (sqlite3_column_int(stmt, 3));
                bst.node[i].test_style          = (sqlite3_column_int(stmt, 4));
                bst.node[i].divcap_start_step   = (sqlite3_column_int(stmt, 5));
                bst.node[i].divcap_end_step     = (sqlite3_column_int(stmt, 6));
                bst.node[i].recharge_start_step = (sqlite3_column_int(stmt, 7));
                bst.node[i].recharge_end_step   = (sqlite3_column_int(stmt, 8));
                bst.node[i].recharge_ratio      = (sqlite3_column_int(stmt, 9));
                bst.node[i].recharge_cap        = (sqlite3_column_int(stmt, 10));
            }
            //sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        sqlite3_free(errmsg);
    }

    close_sqlite_db();
    return;
}

/******************************************************************
* 函数名: search_hisdata_fun
* 功能:   查找历史数据
* 参数：   u16 mun 节点数
* 返回值:
******************************************************************/
int search_hisdata_fun(u16 mun)
{
    int ret;
    char search_sysconfig[256];
    sqlite3_stmt *stmt = NULL;

    ret = open_sqlite_db();
    if(ret == 0){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("open search_hisdata db error!");
        #endif
        return 0;
    }

    sprintf(search_sysconfig,"SELECT * FROM running_data_table_history where x = %d;", mun);
    ret = sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL);
    if( ret == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            if(sqlite3_column_int(stmt, 1) == mun){
                sqlite3_finalize(stmt);
                close_sqlite_db();
                return 1;   //有历史数据返回1
            }
        }
    }

    sqlite3_finalize(stmt);
    close_sqlite_db();
    return 0;   //无历史数据返回0
}

/******************************************************************
* 函数名: read_hisdata_fun
* 功能:   读取单点历史数据
* 参数：   u16 mun 节点数
* 返回值:
******************************************************************/
int read_hisdata_fun(realtime_data_style_new *temp_data, u16 mun)
{
    int idR;    //要删除的ID
    int ret;
    char search_sysconfig[256];
    sqlite3_stmt *stmt = NULL;
    char *errmsg;

    ret = open_sqlite_db();
    if(ret == 0){
        #if SQLITE_DEBUG_CONFIG > 0
        printf("open search_hisdata db error!");
        #endif
        return 0;
    }

    sprintf(search_sysconfig,"SELECT * FROM running_data_table_history where x = %d;", mun);
    ret = sqlite3_prepare_v2(db, search_sysconfig, -1, &stmt, NULL);
    if( ret == SQLITE_OK){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            if(sqlite3_column_int(stmt, 1) == mun){
                idR =  (sqlite3_column_int(stmt, 0));
                temp_data->cycMun       =  htons(sqlite3_column_int(stmt, 2));
                temp_data->sub_cycMUn   =  (sqlite3_column_int(stmt, 3));
                temp_data->stepMun      =  (sqlite3_column_int(stmt, 4));
                temp_data->stepStyle    =  (sqlite3_column_int(stmt, 5));
                temp_data->V            =  htons(sqlite3_column_int(stmt, 6));
                temp_data->I            =  htonl(sqlite3_column_int(stmt, 7));
                temp_data->cap          =  htonl(sqlite3_column_int(stmt, 8));
                temp_data->power        =  htonl(sqlite3_column_int(stmt, 9));
                temp_data->ccRatio      =  htons(sqlite3_column_int(stmt, 10));
                temp_data->platTime     =  htons(sqlite3_column_int(stmt, 11));
                temp_data->runTime      =  htonl(sqlite3_column_int(stmt, 12));
                temp_data->noteStatue   =  (sqlite3_column_int(stmt, 13));
                temp_data->returnFlag   =  (sqlite3_column_int(stmt, 14));
                temp_data->returnRatio  =  (sqlite3_column_int(stmt, 15));

                //读完一条删除一条
                sprintf(search_sysconfig, "DELETE FROM running_data_table_history WHERE id=%d", idR);
                ret = sqlite3_exec(db, search_sysconfig, NULL, NULL, &errmsg);
                printf("ret = %d\r\n", ret);

                sqlite3_finalize(stmt);
                sqlite3_free(errmsg);
                close_sqlite_db();
                return 1;   //有历史数据返回1
            }
        }
    }

    sqlite3_finalize(stmt);
    close_sqlite_db();
    return 0;   //无历史数据返回0
}

