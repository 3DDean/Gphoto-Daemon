#pragma once
#include <gphoto2/gphoto2-camera.h>
#include <string_view>

struct CameraObj;

struct CameraStorage
{
	CameraStorage();

	std::string_view basedir();
	std::string_view label();
	std::string_view description();
	CameraStorageType type();
	CameraStorageFilesystemType filesystem_type();
	CameraStorageAccessType access();
	uint64_t capacitykbytes();
	uint64_t freekbytes();
	uint64_t freeimages();
	int array_count();

  private:
	friend CameraObj;
	CameraStorageInformation *info;
	int count;
};