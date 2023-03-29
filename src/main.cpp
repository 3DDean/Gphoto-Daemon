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
#include <thread>

// https://gist.github.com/gcmurphy/c4c5222075d8e501d7d1

bool running = true;
void term(int signum)
{
	running = false;
}

struct timelapse_manager
{
	timelapse_manager()
		: doTimelapse(false) {}

	int start(CameraObj &activeCamera, useconds_t delayMs)
	{
		if (delayMs > 0)
		{
			doTimelapse = true;
			thread = std::thread(
				[&]()
				{
					while (doTimelapse)
					{
						gp_error_check(activeCamera.capture());
						usleep(delay);
					}
				});
		}
	}

	void stop()
	{
		doTimelapse = false;
	}

  private:
	std::atomic<bool> doTimelapse;
	std::atomic<useconds_t> delay;
	std::thread thread;
};

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
		// std::printf("add %i", data.array_count());
		// std::cout << data.count << "\n\n";
	};

	if (gphoto.cameraCount() > 0)
	{
		// try
		// {
			gphoto.openCamera(0, activeCamera);
			activeCamera.create_config_file(config);
			activeCamera.create_value_file(config);
			activeCamera.config = &config;
		// }
		// catch (GPhotoException &e)
		// {}		
	}
	else
	{
		std::printf("No cameras detected\n");
		return -1;
	}

	using stack_allocated_buffer = stack_buffer<512>;

	StatusMessenger statusMsgr(config.get_status_file_path());

	read_pipe<stack_allocated_buffer> instruction_pipe(config.get_pipe_file_path(), O_NONBLOCK);

	// TODO Set up response que
	// TODO Update values file when camera config options are updated
	// TODO Fix radio buttons
	statusMsgr.set();

	timelapse_manager timelapseManager;
	int temp = 0;
	bool doTimelapse = false;
	pipe_buffer<512> piperBuffer;
	while (running)
	{
		// sigsuspend(&continueMask);
		for (auto pipeData : instruction_pipe)
		{
			for (auto match : ctre::split<"\n">(instruction_pipe.get_string_view()))
			{
				std::string_view matchingStr = match;
				if (auto [match2, commandResult, valueResult] = ctre::match<"(\\w+)\\s*(.+)?">(matchingStr); match2)
				{
					std::string command((std::string_view)commandResult);
					std::string value((std::string_view)valueResult);

					if (command == "capture_preview")
					{
						std::cout << activeCamera.capture_preview() << "\n";
					}
					else if (command == "capture")
					{
						std::cout << activeCamera.capture() << "\n";
					}
					else if (command == "capture_timelapse")
					{
						if (doTimelapse)
						{
							std::cout << "Timelapse End\n";
							timelapseManager.stop();
							doTimelapse = false;
						}
						else
						{
							try
							{
								int delayAmount = std::stoi(value.data());
								timelapseManager.start(activeCamera, delayAmount);

								std::cout << "Timelapse Start\n";
								doTimelapse = true;
							}
							catch (std::exception &e)
							{
								std::cout << "Error: " << e.what() << std::endl;
							}
						}
					}
					else
					{

						activeCamera.set_config_value(command, value);
						// TODO inform the instruction pipe that this data has been consumed and can be written over if need be
					}
					pipeData.consume(match.end());
				}
				// TODO Command parsing

				std::cout << matchingStr << "\n";
			}
		}
		instruction_pipe.clear();

		usleep(10000);
	}

	return 0;
}
