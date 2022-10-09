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

	Member<"context", &CameraObj::context> cameraContext;

	Member<"openCamera", &CameraObj::openCamera> openCamera;
	Member<"exitCamera", &CameraObj::exitCamera> exitCamera;
	Member<"getSummary", &CameraObj::getSummary> getSummary;
	Member<"getConfig", &CameraObj::getConfig> getConfig;

	Member<"getStorageInfo", &CameraObj::getStorageInfo> getStorageInfo;
	Member<"triggerCapture", &CameraObj::triggerCapture> triggerCapture;
	Member<"capture_preview", &CameraObj::capture_preview> capture_preview;
	Member<"waitForEvent", &CameraObj::waitForEvent> waitForEvent;
	Member<"capture", &CameraObj::capture> capture;

	// human_readable_type_with_error<Member<"context", &CameraObj::context>>::type> temp;
	// openCamera::type

	auto getStatus = [](){};
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
	std::string socketPath= "/run/gphoto2.sock";
	// std::string socketPath= "/home/threeddean/Documents/gphoto2.sock";

	using ServerSocketT = ServerSocket<AF_UNIX>;
	using StatusMessengerT = StatusMessenger<ServerSocketT>;

	ServerSocketT socket(SOCK_DGRAM, 5, socketPath.c_str());
	StatusMessengerT statusMsgr(socket.socket_desc);

	auto instructions = std::make_tuple(&statusMsgr,
		Instruction<"status\n", void()>(getStatus),
		Instruction<"capturing\n", void()>(capturelmb),
		Instruction<"gettingSummery\n", void()>(getSummarylmb),
		Instruction<"gettingConfig\n", void()>(getConfiglmb),
		Instruction<"gettingStorage\n", void()>(getStorageInfolmb));


	std::printf("Ready\n");

	MsgBuffer<512> msgBuffer(socket.socket_desc);
	while (running)
	{
		auto buffer = msgBuffer.rec_msg();
		
		uint16_t opCode;
		if (buffer.read(opCode))
		{
			std::printf("opcode %i \n", opCode);

			tupleFunctorSwitch(instructions, opCode, buffer);
		}
		usleep(10000);
	}

	return 0;
}
