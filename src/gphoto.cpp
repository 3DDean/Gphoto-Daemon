#include "gphoto.h"
#include "gphoto-error.h"
#include <chrono>
#include <fcntl.h>
#include <filesystem>
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

GPhoto::GPhoto(daemon_config &config)
	: config(config)
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

	abilities.load(context);
}
GPhoto::GPhoto(GPhoto &&move)
	: config(move.config),
	  context(std::move(move.context)),
	  cameraList(std::move(move.cameraList)),
	  port_list(std::move(move.port_list)),
	  abilities(std::move(move.abilities)),
	  loadedCameras(std::move(move.loadedCameras))
{
	move.context = nullptr;
}

GPhoto::~GPhoto()
{
	for (auto [hash, camera] : loadedCameras)
	{
		delete camera;
	}
	if (context != nullptr)
		gp_context_unref(context);
}

// This detects cameras
int GPhoto::detectCameras()
{
	port_list.load();

	abilities.detect(port_list, cameraList, context);

	std::cout << cameraList.count() << "\n";
	return cameraList.count();
}

std::string GPhoto::openCamera(int index)
{
	int ret, modelDescriptor, portDescriptor;
	CameraAbilities camAbilities;
	int cameraIndex = loadedCameras.size();

	// TODO add a life check
	if (index < cameraCount())
	{
		CameraListEntry cameraEntry(cameraList, index);
		std::string_view nameStr = cameraEntry.name;
		std::string_view camera_port = cameraEntry.value;
		std::size_t cameraHash = std::hash<std::string_view>{}(nameStr);

		// TODO Collision Resolution
		if (loadedCameras.count(nameStr.data()) == 0)
		{
			modelDescriptor = abilities.lookup_model(nameStr);
			/* First lookup the model / driver */

			abilities.get_abilities(modelDescriptor, &camAbilities);

			/* Then associate the camera with the specified port */
			portDescriptor = port_list.lookup_path(port);

			auto port_info = port_list.getPortInfoListInfo(portDescriptor);

			std::filesystem::path camera_path = config.camera_dir;
			camera_path /= "camera" + std::to_string(cameraIndex);

			std::filesystem::create_directories(camera_path);
			loadedCameras.emplace(nameStr, new CameraObj(context, camAbilities, port_info, nameStr, camera_port, camera_path));
			// TODO wrtie loaded cameras
		}
		else
		{
			// TODO Collision Resolution
			std::cout << "Implement Collision data\n";
		}

		return nameStr.data();
	}
	else
	{
		throw std::logic_error("No cameras detected to load.");
	}
}

bool GPhoto::closeCamera(std::string cameraID)
{
	throw "Not yet implemented";
}

void GPhoto::process_camera_command(status_manager &config,
									std::vector<std::string_view> &cmdArgs)
{
	std::string_view cmd = cmdArgs[0];
	cmdArgs.erase(cmdArgs.begin());

	if (loadedCameras.size())
	{
		if (ctre::match<"camera_all">(cmd))
		{
			for (auto [name, camera] : loadedCameras)
				camera->process_command(config, cmd, cmdArgs);
		}
		else
		{
			auto it = loadedCameras.find(cmd.data());

			// Check if the key was found
			if (it != loadedCameras.end())
				it->second->process_command(config, cmd, cmdArgs);
			else
				config.error(std::string("Could not find ") + cmd.data());
		}
	}
	else
		config.error(std::string("No camera's loaded"));
}

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