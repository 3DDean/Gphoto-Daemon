#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commander.h"
#include "daemon_config.h"
#include "gphoto-widget.h"
#include "gphoto.h"
#include "named_pipe.h"
#include <gphoto2/gphoto2-camera.h>

#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>

#include "format.h"
#include <sstream>

#include "gphoto_commands.h"
// https://gist.github.com/gcmurphy/c4c5222075d8e501d7d1

bool running = true;
void term(int signum)
{
	running = false;
}

int main(int argc, char **argv)
{
	// Linux signal handling, more information in signal.h
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	sigset_t continueMask;
	sigemptyset(&continueMask);
	sigaddset(&continueMask, SIGCONT);
	sigaddset(&continueMask, SIGUSR1);
	sigaddset(&continueMask, SIGUSR2);

	daemon_config config;

	std::string pathDir = "/tmp";

	std::string pipeFile = "gphoto2.pipe";
	std::string statusFile = "status_gphoto2.txt";
	std::string widgetFile = "gphoto2_widget.txt";

	GPhoto gphoto;

	CameraObj activeCamera;
	CameraStorage storage;

	auto capturelmb = [&activeCamera]()
	{ activeCamera.capture(); };

	auto getSummarylmb = [&activeCamera]()
	{
		CameraText data;
		activeCamera.getSummary(data);
		std::printf("%s", data.text);
	};

	auto getStorageInfolmb = [&activeCamera]()
	{
		CameraStorage data;
		activeCamera.getStorageInfo(data);
		std::printf("add %i", data.count);
		std::cout << data.count << "\n\n";
	};

	if (gphoto.cameraCount() > 0)
	{
		if (gphoto.openCamera(0, activeCamera) < GP_OK)
		{
			std::printf("Could Not open camera\n");
			return -1;
		}

		activeCamera.create_config_file(config);
		activeCamera.create_value_file(config);
	}
	else
	{
		std::printf("No cameras detected\n");
		return -1;
	}
	using stack_allocated_buffer = stack_buffer<512>;

	StatusMessenger statusMsgr(config.get_status_file_path());

	read_pipe<stack_allocated_buffer> instruction_pipe(config.get_pipe_file_path(), O_NONBLOCK);

	auto instructions = std::make_tuple(
		Instruction<"capturing\n", void()>(capturelmb),
		Instruction<"gettingSummery\n", void()>(getSummarylmb),
		Instruction<"gettingStorage\n", void()>(getStorageInfolmb));

//TODO Set up response que
//TODO Update values file when camera config options are updated
//TODO Fix radio buttons
	statusMsgr.set();
	int temp = 0;
	pipe_buffer<512> piperBuffer;
	while (running)
	{
		// sigsuspend(&continueMask);
		if (instruction_pipe.read())
		{
			for (auto match : ctre::split<"\n">(instruction_pipe.get_string_view()))
			{
				std::string_view matchingStr = match;
				if (auto [match2, config, value] = ctre::match<"(\\w+)\\s+(.+)">(matchingStr); match2)
				{
					std::string configStr((std::string_view)config);
					std::string valueStr((std::string_view)value);
					activeCamera.set_config_value(configStr, valueStr);
				}
				// TODO Command parsing

				std::cout << matchingStr << "\n";
			}
			instruction_pipe.buffer.clear();
		}
		usleep(10000);
	}

	return 0;
}
