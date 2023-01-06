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
	std::string main_dir = "/tmp/gphoto_daemon";
	std::string camera_dir = "cameras";
};