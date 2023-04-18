#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_pipe.h"
#include "daemon_config.h"
#include "gphoto-widget.h"
#include "gphoto.h"
#include "named_pipe.h"
#include <fstream>
#include <gphoto2/gphoto2-camera.h>
#include <iostream>
#include <string>

#include "common.h"
#include "format.h"
#include "status.h"
#include <condition_variable>
#include <csignal>
#include <sstream>
#include <thread>
// https://gist.github.com/gcmurphy/c4c5222075d8e501d7d1

using millisecond_duration = std::chrono::duration<double, std::milli>;

// TODO Create template
struct cli_args
{
	void set_value(std::stringstream &error_stream, std::string_view flagName, auto &arg_itt)
	{
		++arg_itt;
		if (flagName == "config")
		{
			config_file = arg_itt->data();
		}
		else
		{
			error_stream << "unrecognized argument" << *arg_itt << "\n";
		}
	}

	std::string_view config_file;
};

cli_args process_args(int argc, const char **argv)
{
	cli_args args;

	std::vector<std::string_view> arg_strs;
	std::stringstream error_stream;

	for (std::size_t i = 0; i < argc; i++)
		arg_strs.emplace_back(argv[i]);

	for (auto itt = arg_strs.begin(); itt < arg_strs.end(); itt++)
	{
		std::string_view arg(*itt);
		try
		{
			if (arg.starts_with("--"))
				args.set_value(error_stream, arg.substr(2, arg.size() - 2), itt);
		}
		catch (const std::out_of_range &e)
		{
			// TODO exit
		}
	}
	auto error_view = error_stream.view();
	if (error_view.empty())
		return args;

	std::cout << error_view;
	exit(-2);
}

std::mutex signal_mutex;
std::condition_variable signal_cv;
bool signal_processed = false;
bool running = true;

static void signal_handler(int signum) noexcept
{
	running = false;
}

static void io_processor(int signum) noexcept
{
	{
		std::lock_guard<std::mutex> lock(signal_mutex);
		signal_processed = true;
	}
	signal_cv.notify_one();

	// TODO Actually do parsing of io
}

int main(int argc, const char **argv)
{
	daemon_config config;

	{
		cli_args args = process_args(argc, argv);
		config.init(args.config_file);
	}

	// Linux signal handling, more information in signal.h
	signal(SIGINT, &signal_handler);
	signal(SIGIO, io_processor);

	command_pipe gphoto_pipe(
		object_instruction_set(
			GPhoto(config),
			instruction("detect_camera", &GPhoto::detectCameras),
			instruction("open_camera", &GPhoto::openCamera),
			instruction("close_camera", &GPhoto::closeCamera),
			instruction("camera", &GPhoto::process_camera_command)),
		config.get_pipe_file_path(),
		config.get_status_file_path(),
		"log.txt");

	while (running)
	{
		{
			std::unique_lock<std::mutex> lock(signal_mutex);
			signal_cv.wait(lock, []
						   { return signal_processed; });
			signal_processed = false;
		}

		gphoto_pipe.parse_commands();
	}

	return 0;
}
