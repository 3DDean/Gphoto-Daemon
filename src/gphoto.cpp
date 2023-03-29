#include "gphoto.h"
#include "gphoto-error.h"
#include <chrono>
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
	gp_error_check(detectCameras());

	// int cameraCount = gp_list_count(list);
	// detectedCameras.resize(cameraCount);

	// for (std::size_t i = 0; i < cameraCount; i++)
	// {
	// 	detectedCameras[i] = CameraListEntry(list, i);
	// 	// gp_list_get_value(list, i, )
	// 	// gp_error_check(gp_list_get_name(list, i, &detectedCameras[i]));
	// }
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

		// // int sample_open_camera(Camera **camera, const char *model, const char *port, GPContext *context)
		// // {

		// ret = gp_camera_new(&ptr);
		// if (ret < GP_OK)
		// 	return ret;

		// /* First lookup the model / driver */
		// m = gp_abilities_list_lookup_model(abilities, nameStr);
		// if (m < GP_OK)
		// 	return ret;
		// ret = gp_abilities_list_get_abilities(abilities, m, &a);
		// if (ret < GP_OK)
		// 	return ret;
		// ret = gp_camera_set_abilities(ptr, a);
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
		// ret = gp_camera_set_port_info(ptr, pi);
		// if (ret < GP_OK)
		// 	return ret;

		// // }

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
		// camera.context = context;
		// camera.ptr = ptr;
		// camera.name = nameStr;
		// camera = CameraObj(context, ptr, context);
		// camera.name += cameraEntry.value;
		//= CameraObj(context, ptr);
	}
	else
	{
		throw std::logic_error("No camera's detected.");
	}
}

std::string_view CameraStorage::basedir()
{
	return (GP_STORAGEINFO_BASE & info->fields) ? info->basedir : "";
}

std::string_view CameraStorage::label()
{
	return (GP_STORAGEINFO_LABEL & info->fields) ? info->label : "Label Undefined";
}

std::string_view CameraStorage::description()
{
	return (GP_STORAGEINFO_DESCRIPTION & info->fields) ? info->description : "Description Undefined";
}

CameraStorageType CameraStorage::type()
{
	return (GP_STORAGEINFO_STORAGETYPE & info->fields) ? info->type : GP_STORAGEINFO_ST_UNKNOWN;
}

CameraStorageFilesystemType CameraStorage::fstype()
{
	return (GP_STORAGEINFO_FILESYSTEMTYPE & info->fields) ? info->fstype : GP_STORAGEINFO_FST_UNDEFINED;
}

CameraStorageAccessType CameraStorage::access()
{
	return (GP_STORAGEINFO_ACCESS & info->fields) ? info->access : GP_STORAGEINFO_AC_READONLY;
}

uint64_t CameraStorage::capacitykbytes()
{
	return (GP_STORAGEINFO_MAXCAPACITY & info->fields) ? info->capacitykbytes : 0;
}

uint64_t CameraStorage::freekbytes()
{
	return (GP_STORAGEINFO_FREESPACEKBYTES & info->fields) ? info->freekbytes : 0;
}

uint64_t CameraStorage::freeimages()
{
	return (GP_STORAGEINFO_FREESPACEIMAGES & info->fields) ? info->freeimages : 0;
}

int CameraStorage::array_count()
{
	return count;
}

int CameraObj::getSummary(CameraText &summary)
{
	int ret = gp_camera_get_summary(ptr, &summary, context);
	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		printf("Could not get summary.\n");
	}
	return ret;
}

int CameraObj::getConfig(CameraWidget *&rootwidget)
{
	int ret = gp_camera_get_config(ptr, &rootwidget, context);
	if (ret < GP_OK)
	{
		fprintf(stderr, "Could not get config.\n");
	}
	return ret;
}

int CameraObj::getStorageInfo(CameraStorage &storage)
{
	int ret = gp_camera_get_storageinfo(ptr, &storage.info, &storage.count, context);

	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		printf("Could not get storage info.\n");
	}
	return ret;
	// free(storageinfo);
}

int CameraObj::triggerCapture()
{
	int ret;
	do
	{
		// TODO figure out if this is a bug
		ret = gp_camera_trigger_capture(ptr, context);
	} while (ret == GP_ERROR_CAMERA_BUSY);

	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		printf("Could not trigger capture.\n");
	}

	return ret;
}

int CameraObj::exitCamera()
{
	gp_camera_exit(ptr, context);
	return GP_OK;
}
//This currently returns early as the noises my camera makes scares my dog, so testing file handling is a bit difficult atm
int CameraObj::capture()
{
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);

	// convert to string
	std::stringstream ss;
	ss << std::put_time(std::localtime(&t), "%Y-%m-%d %X");
	std::string time = ss.str();

	CameraFilePath filePath;
	gp_error_check(gp_camera_capture(ptr, GP_CAPTURE_IMAGE, &filePath, context));

	// TODO WAIT FOR EVENT SO THAT I CAN CONSUME IT ALL
	waitForEvent(10);

	gp_error_check(gp_camera_file_delete(ptr, filePath.folder, filePath.name, context));
	
	return GP_OK;

	CameraFileInfo fileInfo;

	gp_camera_file_get_info(ptr, filePath.folder, filePath.name, &fileInfo, context);
	CameraFile *file;
	gp_file_new(&file);
	// gp_filesystem_put_file
	gp_camera_file_get(ptr,
					   filePath.folder,
					   filePath.name,
					   GP_FILE_TYPE_NORMAL,
					   file,
					   context);

	std::string_view folder = storageInformation.basedir();

	const char *data;
	unsigned long size;

	gp_file_save(file, "Testing.jpg");

	gp_file_get_data_and_size(file, &data, &size);

	std::string_view filenameStr(time.c_str());

	std::ofstream captureFiles = config->get_image_dir(time, "jpg");
	captureFiles.write(data, size);

	gp_camera_file_delete(ptr, filePath.folder, filePath.name, context);
	// gp_file_free(file);

	return GP_OK;
}

#include <fcntl.h>
#include <fstream>
int CameraObj::capture_preview()
{
	int ret;
	CameraFile *file;
	const char *filename;
	// int fd = open(previewPath.data(), O_CREAT | O_TRUNC, O_RDWR);

	waitForEvent(10);
	gp_error_check(gp_file_new(&file), "Failed to create new file handle");
	gp_error_check(gp_file_adjust_name_for_mime_type(file), "Failed to adjust name");

	ret = gp_camera_capture_preview(ptr, file, context);

	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		gp_file_free(file);
		printf("Could not capture preview.\n");
		return ret;
	}

	const char *data;
	unsigned long size;
	gp_file_get_data_and_size(file, &data, &size);

	gp_error_check(gp_file_get_name(file, &filename), "Failed to get file name");

	std::string_view filenameStr(filename);

	std::ofstream captureFiles = config->get_camera_preview_file("jpg");
	captureFiles.write(data, size);

	gp_file_free(file);
	return GP_OK;
}

int CameraObj::waitForEvent(int timeout)
{
	int ret;
	while (1)
	{
		CameraEventType evttype;
		void *data = NULL;

		ret = gp_camera_wait_for_event(ptr, timeout, &evttype, &data, context);
		switch (evttype)
		{
		case GP_EVENT_UNKNOWN: /**< unknown and unhandled event. argument is a char* or NULL */
			break;
		case GP_EVENT_TIMEOUT: /**< timeout, no arguments */
			break;
		case GP_EVENT_FILE_ADDED: /**< CameraFilePath* = file path on camfs */
			break;
		case GP_EVENT_FOLDER_ADDED: /**< CameraFilePath* = folder on camfs */
			break;
		case GP_EVENT_CAPTURE_COMPLETE: /**< last capture is complete */
			break;
		case GP_EVENT_FILE_CHANGED: /**< CameraFilePath* = file path on camfs */
			break;
		default:
			if (data)
				free(data);
			break;
		}

		if (ret < GP_OK)
			break;
	}
	return ret;
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