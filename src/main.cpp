#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Commander.h"
#include "formater.h"
#include "gphoto-widget.h"
#include "gphoto.h"
#include "named_pipe.h"
#include <gphoto2/gphoto2-camera.h>

#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>

#include "format.h"

// https://gist.github.com/gcmurphy/c4c5222075d8e501d7d1

bool running = true;
void term(int signum)
{
	running = false;
}

int main(int argc, char **argv)
{
	auto tab_base1 = format_string<"<div class=${str:class} id=${str:id} name=${str:name}>${array:members}</div>">();
	auto args = tab_base1.args;

	auto tab_book4 = args.assign_vars("tab_book"_fStr, placeholder, placeholder, placeholder);

	return 0;
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

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

	auto getConfiglmb = [&activeCamera]()
	{
		CameraWidget *data;
		activeCamera.getConfig(data);
		int childCount = gp_widget_count_children(data);
		// std::printf("WidgetChildCount %i", childCount);
		// currently doesn't output unsure why
	};

	auto getStorageInfolmb = [&activeCamera]()
	{
		CameraStorage data;
		activeCamera.getStorageInfo(data);
		std::printf("add %i", data.count);
		std::cout << data.count << "\n\n";
	};

	// Commander shepherd("/run/gphoto2.sock", "/var/www/gphoto2out", test);
	// Commander shepherd("/home/threeddean/Documents/gphoto2.sock", "/var/www/gphoto2out", test);
	if (gphoto.cameraCount() > 0)
	{
		if (gphoto.openCamera(0, activeCamera) < GP_OK)
		{
			std::printf("Could Not open camera\n");
			return -1;
		}

		CameraWidget *data;
		activeCamera.getConfig(data);

		camera_widget widget(data);

		auto value = camera_widget_base_accessors().get_members(data);

		//	std::ofstream widget_file("/home/threeddean/Documents/" + widgetFile, std::ios_base::trunc | std::ios_base::out);
		//	formater formater(widget_file);
		//	widget_writer(formater, data);
		//	widget_file.close();
	}
	else
	{
		std::printf("No cameras detected\n");
		return -1;
	}

	StatusMessenger statusMsgr(pathDir + "/" + statusFile);
	read_pipe instruction_pipe(pathDir + "/" + pipeFile, O_NONBLOCK);

	auto instructions = std::make_tuple(
		Instruction<"capturing\n", void()>(capturelmb),
		Instruction<"gettingSummery\n", void()>(getSummarylmb),
		Instruction<"gettingConfig\n", void()>(getConfiglmb),
		Instruction<"gettingStorage\n", void()>(getStorageInfolmb));

	statusMsgr.set();
	int temp = 0;
	pipe_buffer<512> piperBuffer;
	while (running)
	{
		auto buffer = instruction_pipe.read(piperBuffer);

		uint16_t opCode;
		while (buffer.read(opCode))
		{
			std::printf("%i opcode %i \n", temp, opCode);

			tupleFunctorSwitch(instructions, opCode, statusMsgr, buffer);
			temp++;
		}
		usleep(10000);
	}

	return 0;
}
