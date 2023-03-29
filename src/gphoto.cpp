#include "gphoto.h"
#include "gphoto-error.h"
#include "gphoto_wrapper/event.h"
#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>

static GPPortInfoList *portinfolist = NULL;
static CameraAbilitiesList *abilities = NULL;

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

int sample_open_camera(Camera **camera, const char *model, const char *port, GPContext *context)
{
	int ret, m, p;
	CameraAbilities a;
	GPPortInfo pi;

	gp_error_check(gp_camera_new(camera));

	if (!abilities)
	{
		/* Load all the camera drivers we have... */
		gp_error_check(gp_abilities_list_new(&abilities));
		gp_error_check(gp_abilities_list_load(abilities, context));
	}

	/* First lookup the model / driver */
	m = gp_abilities_list_lookup_model(abilities, model);
	if (m < GP_OK)
		return ret;
	gp_error_check(gp_abilities_list_get_abilities(abilities, m, &a));
	gp_error_check(gp_camera_set_abilities(*camera, a));

	if (!portinfolist)
	{
		/* Load all the port drivers we have... */
		gp_error_check(gp_port_info_list_new(&portinfolist));
		gp_error_check(gp_port_info_list_load(portinfolist));
		gp_error_check(gp_port_info_list_count(portinfolist));
	}

	/* Then associate the camera with the specified port */
	p = gp_port_info_list_lookup_path(portinfolist, port);
	switch (p)
	{
	case GP_ERROR_UNKNOWN_PORT:
		fprintf(stderr, "The port you specified "
						"('%s') can not be found. Please "
						"specify one of the ports found by "
						"'gphoto2 --list-ports' and make "
						"sure the spelling is correct "
						"(i.e. with prefix 'serial:' or 'usb:').",
				port);
		break;
	default:
		break;
	}
	if (p < GP_OK)
		return p;

	gp_error_check(gp_port_info_list_get_info(portinfolist, p, &pi));
	gp_error_check(gp_camera_set_port_info(*camera, pi));
	return GP_OK;
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
	
	gp_error_check(detectCameras());
}

GPhoto::~GPhoto()
{
	gp_context_unref(context);
	gp_port_info_list_free(gpinfolist);
	gp_abilities_list_free(abilities);
	gp_list_free(list);
}

// This detects cameras
int GPhoto::detectCameras()
{
	int ret;

	gp_port_info_list_new(&gpinfolist);
	gp_error_check(gp_port_info_list_load(gpinfolist));

	int count1 = gp_port_info_list_count(gpinfolist);

	// Buffer read
	gp_error_check(gp_port_info_list_lookup_path(gpinfolist, "usb:"));
	int count2 = gp_port_info_list_count(gpinfolist);

	/* Detect all the cameras that can be autodetected... */
	// Check if list is actually new
	gp_error_check(gp_list_new(&list), "ReturnValue %i", 1);

	/* Load all the camera drivers we have... */
	gp_error_check(gp_abilities_list_new(&abilities));
	gp_error_check(gp_abilities_list_load(abilities, context));
	gp_error_check(gp_abilities_list_detect(abilities, gpinfolist, list, context));

	return GP_OK;
}

void GPhoto::openCamera(int index, CameraObj &camera)
{
	int ret, m, p;
	CameraAbilities a;
	GPPortInfo pi;

	if (index < cameraCount())
	{
		CameraListEntry cameraEntry(list, index);
		const char *nameStr = cameraEntry.name;

		// const char *nameStr = detectedCameras[index].name;
		Camera *ptr = NULL;
		fprintf(stderr, "name is %s\n", nameStr);

		ret = sample_open_camera(&ptr, nameStr, port, context);
		if (ret < GP_OK)
		{
			fprintf(stderr, "camera %s at %s not found.\n", nameStr, port);
			// gp_list_free(list);
			throw GPhotoException(ret, "Camera not found at specified port");
		}
		// Double check this
		// gp_list_free(list);

		ret = gp_camera_init(ptr, context);
		if (ret < GP_OK)
		{
			throw std::logic_error("No camera auto detected.\n");
		}
		camera.init(context, ptr, nameStr);
	}
	else
	{
		throw std::logic_error("No camera's detected.");
	}
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