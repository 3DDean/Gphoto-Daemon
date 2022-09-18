#include "C_files/GPhoto2DCmd.h"
#include "C_files/GPhoto2D.h"
#include "C_files/GPhoto2DUtils.h"
#include <string.h>

#define GP_CHECK(FUNC, MSG, ...)   \
	ret = FUNC;                    \
	if (ret < GP_OK)               \
	{                              \
		printLog(MSG, __VA_ARGS__) \
	}

#define READ_BUFFER_DATA(BUFFER, TYPE) \
	*((TYPE *)BUFFER);                 \
	BUFFER += sizeof(TYPE);

uint8_t *gphoto2_open_camera_func(uint8_t *buffer, const uint8_t *bufferEnd)
{
	int ret;
	CameraID openCameraID = -1;
	OpenCameraOptions options = READ_BUFFER_DATA(buffer, OpenCameraOptions)

	if (options.openBy == OPEN_CAMERA_BY_NAME)
	{
		size_t stringLength = strlen((char *)buffer);
		char *str = (char *)buffer;
		buffer += stringLength;
		if (buffer < bufferEnd)
		{
			ret = gphoto2_open_camera_str(str, &openCameraID);
			if (ret == GP_OK)
			{
			}
			else
			{
				printLog("Failed To open camera by string");
			}
		}
	}
	else
	{
		uint8_t index = READ_BUFFER_DATA(buffer, uint8_t)

		if (buffer < bufferEnd)
		{
			ret = gphoto2_open_camera_index(index, &openCameraID);
			if (ret == GP_OK)
			{
			}
			else
			{
				printLog("Failed To open camera by string");
			}
		}
	}
	return buffer;
	// );
}
uint8_t *gphoto2_exit_camera_func(uint8_t *buffer, const uint8_t *bufferEnd)
{
}
uint8_t *gphoto2_capture_image_func(uint8_t *buffer, const uint8_t *bufferEnd)
{
}
uint8_t *gphoto2_capture_preview_func(uint8_t *buffer, const uint8_t *bufferEnd)
{
}
uint8_t *gphoto2_get_detected_cameras_func(uint8_t *buffer, const uint8_t *bufferEnd)
{
}

int gphoto2_process_commands(uint8_t *buffer, size_t size)
{
	const uint8_t *bufferEnd = buffer + size;

	uint8_t cmd = buffer[0];
	buffer++;

	switch (cmd)
	{
	case GP2_OPEN_CAMERA:
		buffer = gphoto2_open_camera_func(buffer, bufferEnd);
		break;
	case GP2_EXIT_CAMERA:
		buffer = gphoto2_exit_camera_func(buffer, bufferEnd);
		break;
	case GP2_CAPTURE_IMAGE:
		buffer = gphoto2_capture_image_func(buffer, bufferEnd);
		break;
	case GP2_CAPTURE_PREVIEW:
		buffer = gphoto2_capture_preview_func(buffer, bufferEnd);
		break;
	case GP2_GET_DETECTED_CAMERAS:
		buffer = gphoto2_get_detected_cameras_func(buffer, bufferEnd);
		break;
	}
}
