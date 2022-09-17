#include "gphoto.h"
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

	ret = gp_camera_new(camera);
	if (ret < GP_OK)
		return ret;

	if (!abilities)
	{
		/* Load all the camera drivers we have... */
		ret = gp_abilities_list_new(&abilities);
		if (ret < GP_OK)
			return ret;
		ret = gp_abilities_list_load(abilities, context);
		if (ret < GP_OK)
			return ret;
	}

	/* First lookup the model / driver */
	m = gp_abilities_list_lookup_model(abilities, model);
	if (m < GP_OK)
		return ret;
	ret = gp_abilities_list_get_abilities(abilities, m, &a);
	if (ret < GP_OK)
		return ret;
	ret = gp_camera_set_abilities(*camera, a);
	if (ret < GP_OK)
		return ret;

	if (!portinfolist)
	{
		/* Load all the port drivers we have... */
		ret = gp_port_info_list_new(&portinfolist);
		if (ret < GP_OK)
			return ret;
		ret = gp_port_info_list_load(portinfolist);
		if (ret < 0)
			return ret;
		ret = gp_port_info_list_count(portinfolist);
		if (ret < 0)
			return ret;
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

	ret = gp_port_info_list_get_info(portinfolist, p, &pi);
	if (ret < GP_OK)
		return ret;
	ret = gp_camera_set_port_info(*camera, pi);
	if (ret < GP_OK)
		return ret;
	return GP_OK;
}

/*
 * This function opens a camera depending on the specified model and port.
 */

static void ctx_error_func(GPContext *context, const char *str, void *data)
{
	fprintf(stderr, "\n*** Contexterror ***              \n%s\n", str);
	fflush(stderr);
}

static void ctx_status_func(GPContext *context, const char *str, void *data)
{
	fprintf(stderr, "%s\n", str);
	fflush(stderr);
}

GPhoto::GPhoto()
{
	context = gp_context_new();
	gp_context_set_error_func(context, ctx_error_func, NULL);
	gp_context_set_status_func(context, ctx_status_func, NULL);
}

GPhoto::~GPhoto()
{
	gp_context_unref(context);
	gp_port_info_list_free(gpinfolist);
	gp_abilities_list_free(abilities);
}

int GPhoto::detectCameras(char *buf)
{
	int ret;

	gp_port_info_list_new(&gpinfolist);
	ret = gp_port_info_list_load(gpinfolist);
	if (ret < GP_OK)
		return ret;

	int count1=gp_port_info_list_count(gpinfolist);

	// Buffer read
	ret = gp_port_info_list_lookup_path(gpinfolist, "usb:");
	if (ret < GP_OK)
		return ret;
	int count2=gp_port_info_list_count(gpinfolist);



	/* Detect all the cameras that can be autodetected... */

	// Check if list is actually new

	ret = gp_list_new(&list);
	if (ret < GP_OK)
		return 1;

	/* Load all the camera drivers we have... */
	ret = gp_abilities_list_new(&abilities);
	if (ret < GP_OK)
		return ret;
	ret = gp_abilities_list_load(abilities, context);
	if (ret < GP_OK)
		return ret;
	ret = gp_abilities_list_detect(abilities, gpinfolist, list, context);
	if (ret < GP_OK)
		return ret;

	return GP_OK;
}

int GPhoto::openCamera(CameraObj &camera, char *port)
{
	int ret, m, p;
	CameraAbilities a;
	GPPortInfo pi;

	ret = gp_list_get_name(list, 0, &camera.name);
	if (ret < GP_OK)
		return ret;

	fprintf(stderr, "name is %s\n", camera.name);

	// // int sample_open_camera(Camera **camera, const char *model, const char *port, GPContext *context)
	// // {

	// ret = gp_camera_new(&camera.ptr);
	// if (ret < GP_OK)
	// 	return ret;

	// /* First lookup the model / driver */
	// m = gp_abilities_list_lookup_model(abilities, camera.name);
	// if (m < GP_OK)
	// 	return ret;
	// ret = gp_abilities_list_get_abilities(abilities, m, &a);
	// if (ret < GP_OK)
	// 	return ret;
	// ret = gp_camera_set_abilities(camera.ptr, a);
	// if (ret < GP_OK)
	// 	return ret;

	// if (!portinfolist)
	// {
	// 	/* Load all the port drivers we have... */
	// 	ret = gp_port_info_list_count(portinfolist);
	// 	if (ret < 0)
	// 		return ret;
	// }

	// /* Then associate the camera with the specified port */
	// p = gp_port_info_list_lookup_path(portinfolist, port);
	// switch (p)
	// {
	// case GP_ERROR_UNKNOWN_PORT:
	// 	fprintf(stderr, "The port you specified "
	// 					"('%s') can not be found. Please "
	// 					"specify one of the ports found by "
	// 					"'gphoto2 --list-ports' and make "
	// 					"sure the spelling is correct "
	// 					"(i.e. with prefix 'serial:' or 'usb:').",
	// 			port);
	// 	break;
	// default:
	// 	break;
	// }
	// if (p < GP_OK)
	// 	return p;

	// ret = gp_port_info_list_get_info(portinfolist, p, &pi);
	// if (ret < GP_OK)
	// 	return ret;
	// ret = gp_camera_set_port_info(camera.ptr, pi);
	// if (ret < GP_OK)
	// 	return ret;

	// // }

	ret = sample_open_camera(&camera.ptr, camera.name, port, context);
	if (ret < GP_OK)
	{
		fprintf(stderr, "camera %s at %s not found.\n", camera.name, port);
		gp_list_free(list);
		return ret;
	}
	gp_list_free(list);

	ret = gp_camera_init(camera.ptr, context);
	if (ret < GP_OK)
	{
		fprintf(stderr, "No camera auto detected.\n");
		return ret;
	}
	return ret;
}

int GPhoto::getSummary(CameraObj &camera, CameraText &summary)
{
	int ret = gp_camera_get_summary(camera.ptr, &summary, context);
	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		printf("Could not get summary.\n");
	}
	return ret;
}

int GPhoto::getConfig(CameraObj &camera, CameraWidget *rootwidget)
{
	int ret = gp_camera_get_config(camera.ptr, &rootwidget, context);
	if (ret < GP_OK)
	{
		fprintf(stderr, "Could not get config.\n");
	}
	return ret;
	// gp_widget_free(rootwidget);
}

int GPhoto::getStorageInfo(CameraObj &camera, CameraStorageInformation *&storageinfo, int &storagecnt)
{
	int ret = gp_camera_get_storageinfo(camera.ptr, &storageinfo, &storagecnt, context);
	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		printf("Could not get storage info.\n");
	}
	return ret;
	// free(storageinfo);
}

int GPhoto::triggerCapture(CameraObj &camera)
{
	int ret;
	do
	{
		ret = gp_camera_trigger_capture(camera.ptr, context);
	} while (ret == GP_ERROR_CAMERA_BUSY);

	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		printf("Could not trigger capture.\n");
	}

	return ret;
}

int GPhoto::exitCamera(CameraObj &camera)
{
	gp_camera_exit(camera.ptr, context);
	return GP_OK;
}
int GPhoto::capture(CameraObj &camera, CameraFilePath *path)
{
	return gp_camera_capture(camera.ptr, GP_CAPTURE_IMAGE, path, context);
}
int GPhoto::capture_preview(CameraObj &camera)
{
	int ret;
	CameraFile *file;

	waitForEvent(camera, 10);
	gp_file_new(&file);
	ret = gp_camera_capture_preview(camera.ptr, file, context);
	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		gp_file_free(file);
		printf("Could not capture preview.\n");
		return ret;
	}
	gp_file_free(file);
	return GP_OK;
}

int GPhoto::waitForEvent(CameraObj &camera, int timeout)
{
	int ret;
	while (1)
	{
		CameraEventType evttype;
		void *data = NULL;

		ret = gp_camera_wait_for_event(camera.ptr, timeout, &evttype, &data, context);
		if (ret < GP_OK)
			break;
		if (data)
			free(data);
		if (evttype == GP_EVENT_TIMEOUT)
			break;
	}
	return ret;
}

int GPhoto::getConfig(CameraObj &camera, CameraWidget *&widget)
{
}
