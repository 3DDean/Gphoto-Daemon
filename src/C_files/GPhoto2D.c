#include "C_files/GPhoto2DUtils.h"
#include <gphoto2/gphoto2-camera.h>
#include <stdlib.h>
#include <string.h>

static void ctx_error_func(GPContext *context, const char *str, void *data)
{
	printLog("\n*** Contexterror ***              \n%s\n", str);
}

static void ctx_status_func(GPContext *context, const char *str, void *data)
{
	printLog("%s\n", str);
}

static GPPortInfoList *portinfolist = NULL;
static CameraAbilitiesList *abilities = NULL;

GPContext *context;
CameraList *cameraList = NULL;
GPPortInfoList *gpinfolist;

struct _OpenCamera
{
	char *name;
	Camera *ptr;
};

typedef struct _OpenCamera OpenCamera;

struct _OpenCameraList
{
	size_t size;
	OpenCamera *cameras;
};

typedef struct _OpenCameraList OpenCameraList;

static OpenCameraList openCameras = {0, NULL};

void gp_open_camera_list_expand(int amount)
{
	size_t newSize = openCameras.size + amount;

	OpenCamera *newList = (OpenCamera *)malloc(sizeof(OpenCamera) * newSize);

	memcpy(newList, openCameras.cameras, (sizeof(OpenCamera) * openCameras.size));
	free(openCameras.cameras);
	openCameras.cameras = newList;
}

size_t gp_open_camera_list_add_camera(const char *name, Camera *ptr)
{
	if (openCameras.cameras == NULL)
	{
		openCameras.cameras = (OpenCamera *)malloc(sizeof(OpenCamera) * 1);		
	}

	size_t pos = openCameras.size;
	openCameras.cameras[pos].name = trim_str(name);
	openCameras.cameras[pos].ptr = ptr;
	openCameras.size++;
	return pos;
}

int gp_open_camera_list_get_camera(size_t index, Camera ** camera)
{
	if(index >= openCameras.size)
	{
		printLog("Camera not open on index %i", index);
		return -1;
	}
	*camera = openCameras.cameras[index].ptr;
}

const char *port = "usb:";

int gphoto2_camera_count()
{
	return gp_list_count(cameraList);
}

#define GP_ERROR_CHECK(ReturnVar, ErrorMsg, ...) \
	if (ReturnVar < GP_OK)                       \
	{                                            \
		printLog(ErrorMsg, __VA_ARGS__);         \
		return return_var;                       \
	}

int gphoto2_detect_cameras()
{
	int ret;

	gp_port_info_list_new(&gpinfolist);
	ret = gp_port_info_list_load(gpinfolist);
	if (ret < GP_OK)
		return ret;

	// Buffer read
	ret = gp_port_info_list_lookup_path(gpinfolist, port);
	if (ret < GP_OK)
		return ret;

	/* Detect all the cameras that can be autodetected... */
	// Check if list is actually new
	ret = gp_list_new(&cameraList);
	if (ret < GP_OK)
		return 1;

	/* Load all the camera drivers we have... */
	ret = gp_abilities_list_new(&abilities);
	if (ret < GP_OK)
		return ret;
	ret = gp_abilities_list_load(abilities, context);
	if (ret < GP_OK)
		return ret;
	ret = gp_abilities_list_detect(abilities, gpinfolist, cameraList, context);
	if (ret < GP_OK)
		return ret;

	return GP_OK;
}

int gphoto2_initialize()
{
	context = gp_context_new();
	gp_context_set_error_func(context, ctx_error_func, NULL);
	gp_context_set_status_func(context, ctx_status_func, NULL);
	int ret = gphoto2_detect_cameras();
	if (ret < GP_OK)
		return ret;

	return ret;
}

int gphoto2_shutdown()
{
	gp_context_unref(context);
	gp_list_free(cameraList);
	gp_port_info_list_free(gpinfolist);
	gp_abilities_list_free(abilities);
}

int sample_open_camera(Camera **camera, const char *model, GPContext *context)
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
		if (ret < GP_OK)
			return ret;
		ret = gp_port_info_list_count(portinfolist);
		if (ret < GP_OK)
			return ret;
	}

	/* Then associate the camera with the specified port */
	p = gp_port_info_list_lookup_path(portinfolist, port);
	switch (p)
	{
	case GP_ERROR_UNKNOWN_PORT:
		printLog("The port you specified "
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
typedef size_t CameraID; 

int gphoto2_open_camera_str(char* str, CameraID* output)
{

}

int gphoto2_open_camera_index(uint8_t index, CameraID* output)
{
	int ret, m, p;
	CameraAbilities a;
	GPPortInfo pi;

	const char *name = (char *)malloc(sizeof(char) * 200);
	Camera *ptr;

	ret = gp_list_get_name(cameraList, index, &name);
	if (ret < GP_OK)
		return ret;

	printLog("name is %s\n", name);

	ret = sample_open_camera(&ptr, name, context);
	if (ret < GP_OK)
	{
		printLog("camera %s at %s not found.\n", name, port);
		return ret;
	}

	ret = gp_camera_init(ptr, context);
	if (ret < GP_OK)
	{
		printLog("No camera auto detected.\n");
		return ret;
	}
	*output = gp_open_camera_list_add_camera(name, ptr);
	free((void *)name);

	return ret;
}

int gphoto2_exitCamera(Camera* camera)
{
	gp_camera_exit(camera, context);
	return GP_OK;
}

int gphoto2_wait_for_event(Camera* camera, int timeout)
{
	int ret;
	while (1)
	{
		CameraEventType evttype;
		void *data = NULL;

		ret = gp_camera_wait_for_event(camera, timeout, &evttype, &data, context);
		if (ret < GP_OK)
			break;
		if (data)
			free(data);
		if (evttype == GP_EVENT_TIMEOUT)
			break;
	}
	return ret;
}

int gphoto2_capture(Camera* camera, CameraFilePath *path)
{
	return gp_camera_capture(camera, GP_CAPTURE_IMAGE, path, context);
}

int gphoto2_capture_preview(Camera* camera)
{
	int ret;
	CameraFile *file;

	gphoto2_wait_for_event(camera, 10);
	gp_file_new(&file);
	ret = gp_camera_capture_preview(camera, file, context);
	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		gp_file_free(file);
		printLog("Could not capture preview.\n");
		return ret;
	}
	gp_file_free(file);
	return GP_OK;
}
