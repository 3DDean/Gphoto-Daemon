
#pragma once
#include "daemon_config.h"
#include "format.h"
#include "gphoto-widget.h"
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

struct GPhotoListWrapper
{
	GPhotoListWrapper()
		: fileList(nullptr)
	{
		gp_list_new(&fileList);
	}
	~GPhotoListWrapper()
	{
		if (fileList != nullptr)
		{
			gp_list_free(fileList);
		}
	}
	using name_value_pair = std::pair<std::string_view, std::string_view>;

	struct iterator
	{
		iterator(int index, GPhotoListWrapper &obj)
			: index(index),
			  obj(obj)
		{}

		iterator &operator++()
		{
			index += 1;
			return *this;
		}
		inline name_value_pair operator*()
		{
			return obj.get_pair(index);
		}

		bool operator!=(const iterator &other) const
		{
			return index != other.index;
		}

	  private:
		int index;
		GPhotoListWrapper &obj;
	};

	name_value_pair get_pair(int index)
	{
		const char *name = nullptr;
		const char *value = nullptr;

		gp_list_get_name(fileList, index, &name);
		gp_list_get_value(fileList, index, &value);

		return name_value_pair((name != nullptr) ? name : "", (value != nullptr) ? value : "");
	}

	std::string_view get_name(int index)
	{
		const char *name = nullptr;
		gp_list_get_name(fileList, index, &name);
		return (name != nullptr) ? name : "";
	}

	std::string_view get_value(int index)
	{
		const char *value = nullptr;

		gp_list_get_value(fileList, index, &value);
		return (value != nullptr) ? value : "";
	}

	int count()
	{
		return gp_list_count(fileList);
	}

	iterator begin()
	{
		return iterator(0, *this);
	}
	iterator end()
	{
		return iterator(count(), *this);
	}

	operator CameraList *()
	{
		return fileList;
	}

	CameraList *fileList;
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

	void openCamera(int index, CameraObj &camera);

	int detectCameras();

	int cameraCount()
	{
		return gp_list_count(list);
	}
};

struct CameraObj;

struct CameraStorage
{
	CameraStorage() {}

	std::string_view basedir();
	std::string_view label();
	std::string_view description();
	CameraStorageType type();
	CameraStorageFilesystemType fstype();
	CameraStorageAccessType access();
	uint64_t capacitykbytes();
	uint64_t freekbytes();
	uint64_t freeimages();
	int array_count();

  private:
	friend CameraObj;
	CameraStorageInformation *info;
	int count;
};

struct CameraPath
{
	CameraPath();
	CameraPath(const char *folder, const char *name);

	void set_folder(const char *folder);
	void set_name(const char *name);
	CameraFilePath file;
};

struct CameraObj
{

	CameraObj(CameraObj &cam)
		: context(cam.context), ptr(cam.ptr)
	{}

	CameraObj()
		: cameraPath("/"),
		  context(nullptr),
		  ptr(nullptr)
	{}

	~CameraObj()
	{
		if (ptr != nullptr)
		{
			gp_camera_exit(ptr, context);
			gp_camera_free(ptr);
		}
	}

	void init(GPContext *contextPtr, Camera *cameraPtr, std::string_view nameStr)
	{
		context = contextPtr;
		ptr = cameraPtr;
		name = nameStr;

		gp_error_check(
			gp_camera_get_storageinfo(ptr, &storageInformation.info, &storageInformation.count, context),
			"Could not get storage info.");
	}
	
	std::string name;
	std::string cameraPath;
	GPContext *context = nullptr;
	Camera *ptr = nullptr;
	CameraPath path;
	CameraStorage storageInformation;
	daemon_config *config;

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
		widget_writer << "\n";

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

	void set_config_value(std::string_view name, std::string_view value)
	{
		camera_widget widget(ptr, context, name);
		// TODO Rewrite this to send output message to log
		if (widget.set_value(value) == GP_OK)
		{
			auto result = gp_camera_set_single_config(ptr, name.data(), widget, context);

			// TODO PUSH OUT TO LOG FILE
		}
		else
		{
			std::cout << "Error " << name << " " << value << "\n";
		}
	}

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
		element_func(root_widget);

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
