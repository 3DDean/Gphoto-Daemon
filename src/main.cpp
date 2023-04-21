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
#include "linux_error.h"
#include "status.h"
#include <condition_variable>
#include <csignal>
#include <sstream>
#include <thread>
// https://gist.github.com/gcmurphy/c4c5222075d8e501d7d1

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
read_pipe *input_pipe;
buffer *command_buffer;

static void signal_handler(int signum) noexcept
{
	running = false;
}

//TODO fix this
static void io_processor(int signum) noexcept
{
	{
		std::lock_guard<std::mutex> lock(signal_mutex);
		signal_processed = true;
	}
	try
	{
		command_buffer->write_to(input_pipe);
		signal_cv.notify_one();
	}
	catch (linux_exception &e)
	{
		if(e.error_num() != EAGAIN)
			std::cout << "exception caught " << e.what() << "\n";
	}

	// TODO Actually do parsing of io
}

int main(int argc, const char **argv)
{
	daemon_config config;

	{
		cli_args args = process_args(argc, argv);
		config.init(args.config_file);
	}

	command_pipe gphoto_pipe(
		object_instruction_set(
			GPhoto(config),
			instruction("detect_camera", &GPhoto::detectCameras),
			instruction("open_camera", &GPhoto::openCamera),
			instruction("close_camera", &GPhoto::closeCamera),
			instruction("camera", &GPhoto::process_camera_command)),
		"gphoto",
		config.main_dir);

	command_buffer = gphoto_pipe.get_buffer();
	input_pipe = gphoto_pipe.get_pipe();

	signal(SIGINT, &signal_handler);
	signal(SIGIO, io_processor);
	while (running)
	{
		{
			std::unique_lock<std::mutex> lock(signal_mutex);
			signal_cv.wait(lock, []
						   { return signal_processed; });
			signal_processed = false;
		}

		gphoto_pipe.process_commands();
	}

	return 0;
}
