#pragma once
// TODO move this to it's own folder
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct daemon_config
{
	void make_directory(const char *path)
	{
		struct stat st = {0};
		if (stat(path, &st) == -1)
		{
			mkdir(path, 0777);
		}
	}
	void make_directory(std::string_view path)
	{
		make_directory(path.data());
	}
	void make_directory(std::string path)
	{
		make_directory(path.data());
	}

	daemon_config()
	{
		make_directory(main_dir);
		camera_dir = main_dir + "/" + camera_dir;
		make_directory(camera_dir);
	}

	auto new_camera_widget_file(std::string camera_name) const
	{
		return std::ofstream(camera_dir + "/" + camera_name + ".widgets", std::ios_base::trunc | std::ios_base::out);
	}
	auto new_camera_value_file(std::string camera_name) const
	{
		return std::ofstream(camera_dir + "/" + camera_name + ".values", std::ios_base::trunc | std::ios_base::out);
	}
	auto get_camera_preview_file(std::string_view extension)
	{
		return std::ofstream(camera_dir + "/" + preview_file + "." + extension.data(), std::ios_base::trunc | std::ios_base::out);
	}
	auto get_image_dir(std::string_view filename, std::string_view extension)
	{
		return std::ofstream(camera_dir + "/" + filename.data() + "." + extension.data(), std::ios_base::trunc | std::ios_base::out);
	}

	auto get_status_file_path()
	{
		return main_dir + "/" + statusFile;
	}
	auto get_pipe_file_path(){
		return main_dir + "/" + pipeFile;
	}

	auto get_instruction_pipe_path(){}

	std::string config_dir;

	std::string main_dir = "/tmp/gphoto_daemon";
	std::string camera_dir = "cameras";

	std::string pipeFile = "gphoto2.pipe";
	std::string statusFile = "status_gphoto2.txt";
	std::string widgetFile = "gphoto2_widget.txt";

	std::string preview_file = "capture_preview";
};