
#pragma once
#include <gphoto2/gphoto2-camera.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include "type_descriptor.h"
struct CameraObj;

struct GPhoto
{
	const char *port = "usb:";

	GPContext *context;
	CameraList *list = NULL;
	GPPortInfoList *gpinfolist;
	CameraAbilitiesList *abilities = NULL;
	std::vector<const char *> detectedCameras;

	GPhoto();
	~GPhoto();

	int openCamera(int index, CameraObj &camera);

	int detectCameras();
	int cameraCount()
	{
		return gp_list_count(list);
	}
};
struct CameraStorage
{
	CameraStorage(){}
	CameraStorageInformation *info;
	int count;
};

struct CameraPath
{
	CameraPath();
	CameraPath(const char* folder, const char* name);
	CameraFilePath file;
};

struct CameraObj
{
	CameraObj(GPContext *context, Camera *ptr)
		: context(context), ptr(ptr)
	{}

	CameraObj(CameraObj &cam)
		: context(cam.context), ptr(cam.ptr)
	{}

	CameraObj()
	{}

	~CameraObj()
	{
		if (ptr != nullptr){
			gp_camera_free(ptr);
			gp_camera_exit(ptr, context);
		}
	}

	GPContext *context = nullptr;
	Camera *ptr = nullptr;
	CameraPath path;

	int openCamera(char *buf);
	int exitCamera();

	int getSummary(CameraText&);
	int getConfig(CameraWidget*&);

	int getStorageInfo(CameraStorage&);

	int triggerCapture();
	int capture_preview();
	int waitForEvent(int timeout);
	int capture();
};


struct gphoto_error : public std::exception
{
	template<typename... ArgsT>
	gphoto_error(int ret, std::string msg, ArgsT... Args) : ret(ret)
	{
		// std::stringstream ss;
		// fprintf(ss., msg, Args...);
		// msg = ss.str();
	}
	int ret;
	std::string msg;
};
template<typename... ArgsT>
void gphoto2_check_result(int result, std::string msg = "Undefined Error", ArgsT... Args)
{
		if (result < GP_OK)
	{
		throw gphoto_error(result, msg, Args...);
	}
}