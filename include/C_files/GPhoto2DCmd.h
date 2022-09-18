#include <stdlib.h>
#include "stdint.h"

#define OPEN_CAMERA_BY_NAME 0
#define OPEN_CAMERA_BY_INDEX 1

struct _OpenCameraOptions
{
	uint8_t openBy : 1;
};
typedef struct _OpenCameraOptions OpenCameraOptions;



enum GP2_Commands
{
	GP2_OPEN_CAMERA,
	GP2_EXIT_CAMERA,
	GP2_CAPTURE_IMAGE,
	GP2_CAPTURE_PREVIEW,
	GP2_GET_DETECTED_CAMERAS
};
int gphoto2_process_commands(uint8_t* buffer, size_t size);



GP2_SETTING_SET, // key value
GP2_SETTING_GET, // key
GP2_NOTIFICATION_SET, // number
GP2_DEBUG_SET, // number
GP2_CAMERA_LIST, //
GP2_CAMERA_SET, // name
GP2_CAMERA_ABILITIES, // [name]
GP2_FOLDER_LIST, // path
GP2_FOLDER_SET, // path
GP2_FILE_COUNT, //
GP2_FILE_GET, // number	[directory] [filename]
GP2_FILE_GET_PREVIEW, // number	[directory] [filename]
GP2_FILE_PUT, // filename
GP2_FILE_DELETE, // number
GP2_CONFIG_GET, //
GP2_CONFIG_SET, // "label"=[choice|value] "label"=[choice|value] "label"=[choice|value] ...
GP2_CAPTURE_TO_DISK, // path [filename]
GP2_ABOUT, //
GP2_MANUAL, //
GP2_SUMMARY, //
//gPhoto notifications
GP2_PROGRESS, // number
GP2_MESSAGE, // "string"
GP2_STATUS, // "string"
GP2_CONFIRM, // "string"





