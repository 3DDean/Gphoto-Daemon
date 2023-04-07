#pragma once
// TODO move this to it's own folder
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//TODO Reconfigure this so it is all relative to each camera
struct daemon_config
{
	daemon_config();

	void init(std::string_view config_path);

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
	auto get_capture_file_path()
	{
		return main_dir + "/last_capture";
	}
	auto get_result_file_path()
	{
		return main_dir + "/result";
	}
	auto get_pipe_file_path(){
		return main_dir + "/" + pipeFile;
	}

	auto get_image_path(std::string_view filename)
	{
		return main_dir + "/" + image_dir + "/" + filename.data();
	}

	auto get_instruction_pipe_path(){}

	auto get_thumbnail_file(std::string_view filename)
	{
		return std::string(camera_dir + "/" + image_dir + "/thumb_" + filename.data());

	}
	auto get_image_file(std::string_view filename)
	{
		return std::string(camera_dir + "/" + image_dir + "/" + filename.data());
	}

	std::string log_file;

	std::string main_dir;
	std::string camera_dir;
	std::string image_dir;
	std::string preview_dir;

	std::string pipeFile;
	std::string statusFile;
	std::string widgetFile;

	std::string preview_file;
};