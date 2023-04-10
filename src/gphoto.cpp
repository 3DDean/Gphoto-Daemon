#include "gphoto.h"
#include "gphoto-error.h"
#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>

/*
 * This detects all currently attached cameras and returns
 * them in a list. It avoids the generic usb: entry.
 *
 * This function does not open nor initialize the cameras yet.
 */
int sample_autodetect(CameraList *list, GPContext *context)
{
	gp_list_reset(list);
	return gp_camera_autodetect(list, context);
}


void error_func(GPContext *context, const char *text, void *data)
{
	std::cout << "Error function called: " << text << "\n";
}

void status_func(GPContext *context, const char *text, void *data)
{
	std::cout << "Status function called: " << text << "\n";
}

// void idle_func(GPContext *context, void *data)
// {
// 	std::cout << "Idle function called\n";
// }

// void message_func(GPContext *context, const char *text, void *data)
// {
// 	std::cout << "Message function called: " << text << "\n";
// }

// GPContextFeedback question_func(GPContext *context, const char *text, void *data)
// {
// 	std::cout << "Question function called: " << text << "\n";
// 	return GP_CONTEXT_FEEDBACK_OK;
// }

// GPContextFeedback cancel_func(GPContext *context, void *data)
// {
// 	std::cout << "Cancel function called\n";
// 	return GP_CONTEXT_FEEDBACK_OK;
// }

// unsigned int progress_start_func(GPContext *context, float target, const char *text, void *data)
// {
// 	std::cout << "Progress start function called with target " << target << " and text " << text << "\n";
// 	return 1;
// }

// void progress_update_func(GPContext *context, unsigned int id, float current, void *data)
// {
// 	std::cout << "Progress update function called for id " << id << " with current value " << current << "\n";
// }

// void progress_stop_func(GPContext *context, unsigned int id, void *data)
// {
// 	std::cout << "Progress stop function called for id " << id << "\n";
// }

GPhoto::GPhoto()
{
	context = gp_context_new();
	// Set the functions on the context
	// gp_context_set_idle_func(context, idle_func, nullptr);
	gp_context_set_error_func(context, error_func, nullptr);
	gp_context_set_status_func(context, status_func, nullptr);
	// gp_context_set_message_func(context, message_func, nullptr);
	// gp_context_set_question_func(context, question_func, nullptr);
	// gp_context_set_cancel_func(context, cancel_func, nullptr);
	// gp_context_set_progress_funcs(context, progress_start_func, progress_update_func, progress_stop_func, nullptr);
	
	port_list.load();
	abilities.load(context);
	detectCameras();
}

GPhoto::~GPhoto()
{
	gp_context_unref(context);
}

// This detects cameras
void GPhoto::detectCameras()
{
	abilities.detect(port_list, cameraList, context);;
}

int GPhoto::openCamera(int index)
{
	int ret, modelDescriptor, portDescriptor;
	CameraAbilities camAbilities;
	int cameraIndex = loadedCameras.size();
	auto camera = loadedCameras.emplace_back();

	if (index < cameraCount())
	{
		CameraListEntry cameraEntry(cameraList, index);
		const char *nameStr = cameraEntry.name;

		fprintf(stderr, "name is %s\n", nameStr);

		modelDescriptor = abilities.lookup_model(nameStr);
		/* First lookup the model / driver */

		abilities.get_abilities(modelDescriptor, &camAbilities);

		/* Then associate the camera with the specified port */
		portDescriptor = port_list.lookup_path(port);

		auto port_info = port_list.getPortInfoListInfo(portDescriptor);

		camera.init(context, nameStr);
		camera.set_abilities(camAbilities);

		camera.set_port_info(port_info);
	}
	else
	{
		throw std::logic_error("No camera's detected.");
	}
	return cameraIndex;
}
bool GPhoto::closeCamera(int index){}

CameraPath::CameraPath(){};
CameraPath::CameraPath(const char *folder, const char *name)
{
	strcpy(file.folder, folder);
	strcpy(file.name, name);
};

void CameraPath::set_folder(const char *folder)
{
	strcpy(file.folder, folder);
}

void CameraPath::set_name(const char *name)
{
	strcpy(file.name, name);
}