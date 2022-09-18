#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gphoto.h"
#include <gphoto2/gphoto2-camera.h>

#include <Commander.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>


bool running = true;
void term(int signum)
{
	running = false;
}

struct CameraOps
{
	int getSummary(CameraText &);
	int getConfig(CameraWidget *);
	int getStorageInfo(CameraStorage &);
	int capture();
};

int getSummary(CameraText &);
int getConfig(CameraWidget *);
int getStorageInfo(CameraStorage &);

int main(int argc, char **argv)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	GPhoto gphoto;

	CameraObj activeCamera;
	CameraStorage storage;

	auto capture = [&activeCamera]()
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
		std::printf("WidgetChildCount %i", childCount);
		//currently doesn't output unsure why
	};
	
	auto getStorageInfolmb = [&activeCamera]()
	{
		CameraStorage data;
		activeCamera.getStorageInfo(data);
		std::printf("add %i", data.count);
		std::cout << data.count << "\n\n";
	};

	auto test = std::make_tuple(
		Instruction<"CI", void()>(capture),
		Instruction<"AA", void()>(getSummarylmb),
		Instruction<"AB", void()>(getConfiglmb),
		Instruction<"AC", void()>(getStorageInfolmb));

	Commander shepherd("/home/threedean/Documents/Temp/gphoto2In", test);

	gphoto.openCamera(0, activeCamera);

	while (running)
	{
		shepherd.read_instructions();
		usleep(10000);
	}

	return 0;
}
