
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
#include <map>
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

	CameraListEntry(CameraList *list, std::string_view name_view)
		: CameraListEntry()
	{
		int index;
		gp_list_find_by_name(list, &index, name_view.data());

		gp_list_get_value(list, index, &value);
		gp_list_get_name(list, index, &name);
	}
};

struct GPhoto
{
	GPhoto(daemon_config &config);
	GPhoto(GPhoto &&move);
	~GPhoto();

	// TODO add command to open camera by name
	// TODO add command to open first detected camera
	std::string openCamera(int index);
	bool closeCamera(std::string cameraID);
	int detectCameras();

	int cameraCount()
	{
		return cameraList.count();
	}
	void process_camera_command(status_manager &config,
								std::vector<std::string_view> &cmdArgs);

  private:
	const char *port = "usb:";

	daemon_config &config;
	GPContext *context;
	gphoto_list cameraList;

	gphoto_port_info_list port_list;
	camera_abilities_list abilities;
	std::map<std::string, CameraObj *> loadedCameras;
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
