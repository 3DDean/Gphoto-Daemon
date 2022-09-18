#include "C_files/GPhoto2DUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* trim_str(const char *str)
{
	size_t length = strlen(str);

	char* newStr = (char*)malloc(sizeof(char) * length + 1);
	strcpy(newStr, str);
	return newStr;
}

void printLog(char *msg, ...)
{
	char *timestamp;
	va_list args;
	va_start(args, msg);
	int nofile = 0;
	FILE *fp;
	vfprintf(stdout, msg, args);

	// if (cfg_stru[c_log_file] != 0 && cfg_val[c_log_size] != 0) {
	// 	nofile = (access(cfg_stru[c_log_file], F_OK ) == -1 );
	// 	fp = fopen(cfg_stru[c_log_file], "a");
	// } else {
	// 	fp = stdout;
	// }
	// if (fp != NULL) {
	// 	clock_gettime(CLOCK_REALTIME, &currTime);
	// 	localTime = localtime (&(currTime.tv_sec));
	// 	makeName(&timestamp, "{%Y/%M/%D %h:%m:%s} ");
	// 	fprintf(fp, "%s",timestamp);
	// 	vfprintf(fp, msg, args);
	// 	if (cfg_stru[c_log_file] != 0) {
	// 		fclose(fp);
	// 		if (nofile) chmod(cfg_stru[c_log_file], 0777);
	// 	}
	// 	free(timestamp);
	// }
	va_end(args);
}

