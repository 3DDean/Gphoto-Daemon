#include "daemon_config.h"
#include <ctre.hpp>
#include <iostream>
#include <sstream>
#include <filesystem>

daemon_config::daemon_config()
	: main_dir("/var/gphoto_daemon"),
	  log_file("gphoto_daemon.log"),
	  camera_dir("cameras"),
	  image_dir("images"),
	  preview_dir("preview"),

	  pipeFile("gphoto2.pipe"),
	  statusFile("status_gphoto2.txt"),
	  widgetFile("gphoto2_widget.txt"),

	  preview_file("capture_preview")
{}

void daemon_config::init(std::string_view config_path)
{
	if (!config_path.empty())
	{
		std::ifstream config_file(config_path.data());

		if (!config_file.fail())
		{
			std::stringstream buffer;
			buffer << config_file.rdbuf(); // read file contents into buffer
			int lineCount = 0;

			for (auto match : ctre::split<"\n">(buffer.view()))
			{
				if (auto [whole, variable, value] = ctre::match<"^\\s*(.*)\\s*=\\s*(.*[^=]*)\\s*$">(match); whole)
				{
					if (variable == "main_dir")
					{
						main_dir = value;
					}
					else if (variable == "log_file")
					{
						log_file = value;
					}
					else if (variable == "camera_dir")
					{
						camera_dir = value;
					}
					else if (variable == "image_dir")
					{
						image_dir = value;
					}
					else if (variable == "preview_dir")
					{
						preview_dir = value;
					}
					else if (variable == "pipe_file")
					{
						pipeFile = value;
					}
					else if (variable == "status_file")
					{
						statusFile = value;
					}
					else if (variable == "widget_file")
					{
						widgetFile = value;
					}
					else if (variable == "preview_file")
					{
						preview_file = value;
					}
				}
			}
		}
		else
		{
			std::cout << "Failed to open config file " << config_path.data() << "\n";
			exit(-1);
		}
	}
	// TODO Setup full paths

	if (!main_dir.ends_with("/"))
		main_dir += "/";
	std::filesystem::create_directory(main_dir);


	auto directory_placer = [&](std::string &path)
	{
		if (!path.starts_with("/"))
			path = main_dir + path;
	};
	
	auto directory_maker = [&](std::string &path)
	{
		if (!path.ends_with("/"))
			path += "/";

		directory_placer(path);
		std::filesystem::create_directory(path);
	};

	directory_maker(camera_dir);

	directory_placer(log_file);

	directory_placer(pipeFile);
	directory_placer(statusFile);
	directory_placer(widgetFile);

	directory_placer(preview_file);
}
