
#pragma once
#include <gphoto2/gphoto2-camera.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct CameraObj
{
	~CameraObj()
	{
		gp_camera_free(ptr);
	}
	Camera *ptr = NULL;
	const char *name;
};

struct GPhoto
{
	GPContext *context;
	CameraList *list = NULL;
	GPPortInfoList *gpinfolist;
	CameraAbilitiesList *abilities = NULL;
	CameraObj activeCamera;
	const char* port = "usb:";

	GPhoto();

	~GPhoto();

	int detectCameras();

	int openCamera(CameraObj &camera, char *buf);

	int getSummary(CameraObj &camera, CameraText &summary);
	int getConfig(CameraObj &camera, CameraWidget *rootwidget);
	int getStorageInfo(CameraObj &camera, CameraStorageInformation *&storageinfo, int &storagecnt);
	int triggerCapture(CameraObj &camera);
	int exitCamera(CameraObj &camera);
	int capture_preview(CameraObj &camera);

	int waitForEvent(CameraObj &camera, int timeout);
	int capture(CameraObj &camera, CameraFilePath *path);
	int getConfig(CameraObj &camera, CameraWidget *&widget);
};