#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gphoto.h"
#include <gphoto2/gphoto2-camera.h>

#include <fstream>
#include <iostream>
#include <string>

// Sample Camera

// static GPPortInfoList *portinfolist = NULL;
// static CameraAbilitiesList *abilities = NULL;

// SAMPLE CONTEXT
static void
ctx_error_func(GPContext *context, const char *str, void *data)
{
	fprintf(stderr, "\n*** Contexterror ***              \n%s\n", str);
	fflush(stderr);
}

static void
ctx_status_func(GPContext *context, const char *str, void *data)
{
	fprintf(stderr, "%s\n", str);
	fflush(stderr);
}

static void errordumper(GPLogLevel level, const char *domain, const char *str,
						void *data)
{
	/* Do not log ... but let it appear here so we discover debug paths */
	// fprintf(stderr, "%s:%s\n", domain, str);
}
void parseCMD()
{
}

int main(int argc, char **argv)
{
	std::string pipe_filename;
	std::string status_filename;

	std::ifstream pipe_file;

	GPhoto gphoto;

	CameraObj camera;
	int ret, storagecnt;

	CameraStorageInformation *storageinfo;

	// CameraWidget *rootwidget;
	char buf[200];
	// const char *name;
	CameraText summary;
	CameraFile *file;
	CameraFilePath path;
	strcpy(path.folder, "test");
	strcpy(path.name, "test");
	/*CameraFilePath		path;*/
	// CameraList *list;
	// CameraAbilitiesList *abilities = NULL;

	int error;
	gp_log_add_func(GP_LOG_DEBUG, errordumper, NULL);

	strcpy(buf, "usb:");
	if (argc > 1)
		strcat(buf, argv[1]);

	fprintf(stderr, "setting path %s.\n", buf);

	// bool running = true;
	// while(running)
	// {
	// 	pipe_file.open(pipe_filename);
	// 	int length = pipe_file.tellg();
	// 	char * buffer = new char [length];
	// }

	ret = gphoto.detectCameras(buf);
	if (ret < GP_OK)
		return ret;

 	gphoto.openCamera(camera, buf);

	gphoto.getSummary(camera, summary);

	gphoto.getStorageInfo(camera, storageinfo, storagecnt);

	// gphoto.capture(camera, &path);

	// // gphoto.triggerCapture(camera);
	// gphoto.capture_preview(camera);
	/* AFL PART ENDS HERE */

	gphoto.exitCamera(camera);

	return 0;
}
