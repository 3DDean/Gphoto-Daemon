
#pragma once
#include "daemon_config.h"
#include "format.h"
#include "gphoto-widget.h"
#include "gphoto_wrapper/camera.h"
#include "gphoto_wrapper/camera_abilities.h"
#include "gphoto_wrapper/port_info.h"
#include "instruction.h"

#include <chrono>
#include <gphoto2/gphoto2-camera.h>
#include <iomanip>
#include <sstream>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

struct CameraObj;

struct CameraListEntry
{
	const char *name;
	const char *value;

	CameraListEntry()
		: name(nullptr), value(nullptr) {}

	CameraListEntry(CameraList *list, std::size_t index)
		: CameraListEntry()
	{
		gp_list_get_value(list, index, &value);
		gp_list_get_name(list, index, &name);
	}
};

struct GPhoto
{
	GPhoto();
	~GPhoto();

	int openCamera(int index);
	bool closeCamera(int index);
	void detectCameras();

	int cameraCount()
	{
		return cameraList.count();
	}

  private:
	const char *port = "usb:";

	GPContext *context;
	gphoto_list cameraList;
	gphoto_port_info_list port_list;
	camera_abilities_list abilities;
	std::vector<CameraObj> loadedCameras;
};

struct FileData
{
	FileData()
		: data(nullptr), size(0) {}

	const char *data;
	unsigned long size;
};

struct CameraPath
{
	CameraPath();
	CameraPath(const char *folder, const char *name);

	void set_folder(const char *folder);
	void set_name(const char *name);
	CameraFilePath file;
};
