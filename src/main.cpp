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
#include "sstream"
// https://gist.github.com/gcmurphy/c4c5222075d8e501d7d1

bool running = true;
void term(int signum)
{
	running = false;
}

//Todo add widget config values output and selection
//Current approach is to create a file with ${camera_name}.values that contains both the current value and the corresponding widget id
//it also needs a way of updating config values using the daemon interface
// Format is ', ' delinatate and indentation indicants depth
int widget_writer(auto &output, CameraWidget *cameraWidget, uint32_t indent = 0)
{
	auto [type, type_str] = get_widget_type(cameraWidget);

	for (std::size_t i = 0; i < indent; i++)
		output << " ";

	widget_formatter formatter;
	formatter(output, std::string_view(gwidget_label{}(cameraWidget)),
			  gwidget_name{}(cameraWidget),
			  gwidget_info{}(cameraWidget),
			  gwidget_id{}(cameraWidget),
			  gwidget_readonly{}(cameraWidget),
			  gwidget_changed{}(cameraWidget),
			  type_str);
	output << "\n";

	get_widget_options(output, cameraWidget, indent + 1);

	uint32_t childCount = gp_widget_count_children(cameraWidget);
	if (childCount > 0)
	{
		for (std::size_t i = 0; i < childCount; i++)
		{
			CameraWidget *child;
			gp_widget_get_child(cameraWidget, i, &child);
			widget_writer(output, child, indent + 1);
		}
		// _formater.pop_scope();
	}
	return 0;
}
// TODO move this to it's own folder
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

auto make_directory(const char *path)
{
	struct stat st = {0};
	if (stat(path, &st) == -1)
	{
		mkdir(path, 0777);
	}
}
auto make_directory(std::string_view path)
{
	make_directory(path.data());
}
auto make_directory(std::string path)
{
	make_directory(path.data());
}

struct file_manager
{
	file_manager()
	{
		make_directory(main_dir);
		camera_dir = main_dir + "/" + camera_dir;
		make_directory(camera_dir);
	}

	auto init_camera_config(std::string camera_name)
	{
		return std::ofstream(camera_dir + "/" + camera_name + ".widgets", std::ios_base::trunc | std::ios_base::out);
	}
	auto init_camera_values(std::string camera_name)
	{
		return std::ofstream(camera_dir + "/" + camera_name + ".values", std::ios_base::trunc | std::ios_base::out);
	}
	std::string main_dir = "/tmp/gphoto_daemon";
	std::string camera_dir = "cameras";
};

int main(int argc, char **argv)
{
	// Linux signal handling, more information in signal.h
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	file_manager files;

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

		auto widget_file = files.init_camera_config(activeCamera.name);

		widget_writer(widget_file, data);
		widget_file.close();
		return 0;
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
