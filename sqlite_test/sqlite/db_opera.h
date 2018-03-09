#ifndef _DB_OPERA_
#define _DB_OPERA_

#define SYS_CONFIG_DIR    "/mnt/project/bat_test/datafile/sys_config.ini"
#define NET_CONFIG_DIR    "/mnt/project/bat_test/datafile/net_config.sh"
#define STEP_CONFIG_DIR   "/mnt/project/bat_test/datafile/step_config.sh"




int Create_db();

int init_sysconfig_file();
int init_netconfig_file();
#endif
