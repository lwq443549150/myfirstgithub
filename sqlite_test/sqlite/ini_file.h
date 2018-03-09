#ifndef _INI_FILE_H_
#define  _INI_FILE_H_

#define CONF_FILE_PATH	"config.ini"
#define  MAX_PATH 100

extern void inifile_opt_init(void);
//extern int GetCurrentPath(char buf[],char *pFileName);
extern char *GetIniKeyString(char *title,char *key);
extern int GetIniKeyInt(char *title,char *key);
#endif
