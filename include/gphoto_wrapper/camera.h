#pragma once
#include "command_pipe.h"
#include "daemon_config.h"
#include "format.h"
#include "gphoto-widget.h"
#include "gphoto_wrapper/file.h"
#include "gphoto_wrapper/list.h"
#include "gphoto_wrapper/port_info.h"
#include "instruction.h"
#include <filesystem>
#include <gphoto2/gphoto2-camera.h>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <vector>

struct camera_config
{
	camera_config(std::string_view camera_name)
	{
	}
	// std::filesystem::path
	std::string config_file;
	std::string value_file;
};

struct CameraObj;

struct timelapse_manager
{
	timelapse_manager()
		: thread_state(state::idle),
		  do_run(false)
	{}

	enum class state
	{
		idle,
		stopped,
		running,
		error
	};

	void start(CameraObj &activeCamera, int delayMs, int count, std::filesystem::path timelapse_directory);

	void stop();

	bool is_running();

  private:
	std::atomic<bool> do_run;
	std::atomic<state> thread_state;
	std::atomic<useconds_t> delay;

	std::thread thread;
	std::filesystem::path filepath;

	int capture_count;
	int failed_capture_count;
	int failed_capture_max;
};

struct CameraObj
{
	CameraObj();
	CameraObj(GPContext *contextPtr,
			  CameraAbilities abilities,
			  gphoto_port_info &info,
			  std::string_view nameStr,
			  std::string_view port,
			  std::filesystem::path image_dir);

	// CameraObj(const CameraObj &copyTarget);
	CameraObj(CameraObj &&copyTarget);

	~CameraObj();

	void init(GPContext *contextPtr, std::string_view nameStr, std::filesystem::path image_dir);
	int exit();

	bool create_config_file(const daemon_config &config);
	bool create_value_file(const daemon_config &config);
	void set_config_value(std::string_view name, std::string_view value);

	int triggerCapture();
	std::string capture_preview();
	int waitForEvent(int timeout);

	// The functional difference between capture and timelapse is that
	// saves the image before taking another image
	gphoto_file capture();
	std::string capture(int delay, int count);
	// Split this into 2 start and stop timelapse

	bool toggle_timelapse(int delay, int count);

	std::string start_timelapse(int delay, int count);
	bool stop_timelapse();

	gphoto_file get_file(std::string_view folderPath,
						 std::string_view fileName,
						 GPContext *context,
						 CameraFileType fileType)
	{
		gphoto_file file;
		gp_camera_file_get(ptr, folderPath.data(), fileName.data(), fileType, file, context);

		return file;
	};

	gphoto_file get_file(CameraFilePath *filePath, GPContext *context, CameraFileType fileType)
	{
		gphoto_file file;
		gp_camera_file_get(ptr, filePath->folder, filePath->name, fileType, file, context);

		return file;
	};

	gphoto_file save_file(CameraFilePath *filePath, GPContext *context, CameraFileType fileType)
	{
		gphoto_file file;
		gp_camera_file_get(ptr, filePath->folder, filePath->name, fileType, file, context);

		return file;
	};

	auto list_folders(std::string_view folder, GPContext *context)
	{
		gphoto_list fileList;
		gp_camera_folder_list_folders(ptr, folder.data(), fileList, context);

		return fileList;
	}

	gphoto_list list_files(std::string_view folder, GPContext *context)
	{
		gphoto_list fileList;
		gp_camera_folder_list_files(ptr, folder.data(), fileList, context);

		return fileList;
	}

	void set_abilities(CameraAbilities abilities);
	CameraAbilities *get_abilities();
	void set_port_info(gphoto_port_info &info);
	GPPortInfo *get_port_info();

	void set_port_speed(int speed);
	int get_port_speed();

	using camera_instruction_set =
		instruction_set<
			instruction<std::string (CameraObj::*)(int, int)>,
			instruction<std::string (CameraObj::*)()>,
			instruction<bool (CameraObj::*)(int, int)>>;

	void process_command(status_manager &config,
						 std::string_view &command,
						 std::vector<std::string_view> &cmdArgs);

  private:
	GPContext *context = nullptr;
	Camera *ptr = nullptr;
	std::string name;
	std::string port;

	timelapse_manager timelapse;
	std::filesystem::path image_path;
	int capture_count;
	int preview_count;

	std::filesystem::path get_image_path(std::string_view prefix, int& image_count)
	{
		std::filesystem::path captureName = image_path;
		captureName /= prefix;
		captureName += std::to_string(image_count);

		return captureName;
	}

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
