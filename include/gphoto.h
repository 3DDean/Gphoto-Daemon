
#pragma once
#include "daemon_config.h"
#include "format.h"
#include "gphoto-widget.h"
#include <gphoto2/gphoto2-camera.h>
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
	const char *port = "usb:";

	GPContext *context;
	CameraList *list = NULL;
	GPPortInfoList *gpinfolist;
	CameraAbilitiesList *abilities = NULL;
	// TODO Implement
	//  std::vector<CameraListEntry> detectedCameras;

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
	CameraStorage() {}
	CameraStorageInformation *info;
	int count;
};

struct CameraPath
{
	CameraPath();
	CameraPath(const char *folder, const char *name);
	CameraFilePath file;
};

struct CameraObj
{
	CameraObj(GPContext *context, Camera *ptr, std::string_view name)
		: context(context), ptr(ptr), name(name)
	{
	}

	CameraObj(CameraObj &cam)
		: context(cam.context), ptr(cam.ptr)
	{}

	CameraObj()
	{}

	~CameraObj()
	{
		if (ptr != nullptr)
		{
			gp_camera_exit(ptr, context);
			gp_camera_free(ptr);
		}
	}

	std::string name;
	GPContext *context = nullptr;
	Camera *ptr = nullptr;
	CameraPath path;

	bool create_config_file(const daemon_config &config)
	{
		camera_widget root_widget(ptr, context);

		auto config_file = config.new_camera_widget_file(name);
		config_writer widget_writer(config_file);
		widget_writer.write(name);
		std::stack<indent> indent_stack;

		uint32_t widget_count = 1;

		auto writer_func = [&widget_writer, &widget_count](camera_widget &widget)
		{
			widget_writer.write(widget);
		};
		auto push_func = [&]()
		{
			indent_stack.emplace(widget_writer);
		};
		auto pop_func = [&indent_stack]()
		{
			indent_stack.pop();
		};

		process_widget(root_widget, writer_func, push_func, pop_func);

		return true;
	}
	bool create_value_file(const daemon_config &config)
	{
		camera_widget root_widget(ptr, context);
		auto config_file = config.new_camera_value_file(name);
		value_writer widget_writer(config_file);

		auto writer_func = [&widget_writer](camera_widget &widget)
		{
			widget_writer.write(widget);
		};
		auto push_func = []() {};
		auto pop_func = []() {};

		process_widget(root_widget, writer_func, push_func, pop_func);
		return true;
	}

	std::vector<camera_widget> widgets;

	int exitCamera();

	int getSummary(CameraText &);
	int getConfig(CameraWidget *&);
	int getStorageInfo(CameraStorage &);

	int triggerCapture();
	int capture_preview();
	int waitForEvent(int timeout);
	int capture();

  private:
	// Linear implementation
	void process_widget(camera_widget root_widget, auto &&element_func, auto &&push_func, auto &&pop_func)
	{
		using widget_iterator = std::tuple<camera_widget, std::size_t, std::size_t>;

		std::stack<widget_iterator> widget_stack;

		uint32_t widget_count = 1;

		auto &&push_to_stack = [&](camera_widget widget)
		{
			widget_stack.emplace(widget, 0, widget.get_child_count());
			push_func();
		};
		push_to_stack(root_widget);

		while (!widget_stack.empty())
		{
			bool changed_top = false;
			auto &&[parent_widget, itt, end] = widget_stack.top();

			while (itt < end)
			{
				camera_widget widget(parent_widget.get_child(itt), camera_widget_non_owning());
				++itt;
				element_func(widget);

				if (widget.get_child_count() > 0)
				{
					push_to_stack(widget);
					changed_top = true;
					break;
				}
			}

			if (itt == end && !changed_top)
			{
				widget_stack.pop();
				pop_func();
			}
		}
	}
};
