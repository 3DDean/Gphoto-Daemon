#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Commander.h"
#include "gphoto-widget.h"
#include "gphoto.h"
#include "named_pipe.h"
#include <gphoto2/gphoto2-camera.h>

#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>

// https://gist.github.com/gcmurphy/c4c5222075d8e501d7d1

bool running = true;
void term(int signum)
{
	running = false;
}

struct line_formater
{
	std::ostream &output_stream;
	bool isFirstWrite = true;
	line_formater(std::ostream &output)
		: output_stream(output)
	{
	}
	~line_formater()
	{
		output_stream.put('\n');
	}

	line_formater &operator<<(const auto &value)
	{
		if (!isFirstWrite)
		{
			output_stream.put(',');
		}
		else
		{
			isFirstWrite = false;
		}
		formater_write(output_stream, value);
		return *this;
	}
};

struct formater
{
	formater(std::ostream &ouput)
		: output_stream(ouput)
	{
	}

	~formater()
	{
		for (std::size_t i = 0; i < indent_amount; i++)
		{
			output_stream << ("\n");
		}
	}
	void push_scope()
	{
		output_stream << "\n{\n";
	}
	void pop_scope()
	{
		output_stream << "}";
	}
	line_formater get_line_formater()
	{
		return line_formater(output_stream);
	}

	std::size_t indent_amount = 0;
	std::ostream &output_stream;
};

int widget_writer(formater &_formater, CameraWidget *cameraWidget)
{
	auto line = _formater.get_line_formater();

	gp_get_widget_type_and_data(line, cameraWidget);

	int childCount = gp_widget_count_children(cameraWidget);
	if (childCount > 0)
	{
		_formater.push_scope();
		for (std::size_t i = 0; i < childCount; i++)
		{
			CameraWidget *child;
			gp_widget_get_child(cameraWidget, i, &child);
			widget_writer(_formater, child);
		}
		_formater.pop_scope();
	}
	return 0;
}

int main(int argc, char **argv)
{
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

	// human_readable_type_with_error<Member<"context", &CameraObj::context>>::type> temp;

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

	gphoto.openCamera(0, activeCamera);

	std::ofstream widget_file("/home/threeddean/Documents/" + widgetFile, std::ios_base::trunc | std::ios_base::out);
	formater formater(widget_file);
	CameraWidget *data;

	activeCamera.getConfig(data);

	widget_writer(formater, data);
	widget_file.close();

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
