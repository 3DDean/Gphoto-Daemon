#include "gphoto_wrapper/camera_storage.h"

CameraStorage::CameraStorage()
{}

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

CameraStorageFilesystemType CameraStorage::filesystem_type()
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