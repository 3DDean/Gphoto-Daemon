#include "gphoto_wrapper/file.h"
#include <gphoto2/gphoto2-file.h>

gphoto_file::gphoto_file()
	: ptr(nullptr)
{
	gp_error_check(
		gp_file_new(&ptr),
		"Failed to create CameraFile");
}

gphoto_file::gphoto_file(const int fd)
	: ptr(nullptr)
{
	gp_error_check(
		gp_file_new_from_fd(&ptr, fd),
		"Failed to create CameraFile from fd");
}

gphoto_file::gphoto_file(CameraFileHandler *handler, void *priv)
	: ptr(nullptr)
{
	gp_error_check(
		gp_file_new_from_handler(&ptr, handler, priv),
		"Failed to create CameraFile from handler");
}

gphoto_file::~gphoto_file()
{
	gp_error_check(
		gp_file_unref(ptr),
		"Failed to free CameraFile");
}

// Copy constructor and assignment operator
gphoto_file::gphoto_file(const gphoto_file &other)
	: ptr(other.ptr)
{
	gp_error_check(
		gp_file_ref(other.ptr),
		"Failed to ref CameraFile");
}

gphoto_file &gphoto_file::operator=(const gphoto_file &other)
{
	if (this != &other)
	{
		gp_error_check(
			gp_file_unref(ptr),
			"Failed to unref current CameraFile");
		ptr = other.ptr;
		gp_error_check(
			gp_file_ref(other.ptr),
			"Failed to ref other CameraFile");
	}
	return *this;
}

// Setters and getters
void gphoto_file::set_name(std::string_view name)
{
	gp_error_check(
		gp_file_set_name(ptr, name.data()),
		"Failed to set CameraFile name");
}

std::string_view gphoto_file::get_name() const
{
	const char *name;
	gp_error_check(
		gp_file_get_name(ptr, &name),
		"Failed to get CameraFile name");

	return std::string_view(name);
}

void gphoto_file::set_mime_type(std::string_view mime_type)
{
	gp_error_check(
		gp_file_set_mime_type(ptr, mime_type.data()),
		"Failed to set CameraFile mime type");
}

std::string_view gphoto_file::get_mime_type() const
{
	const char *mime_type;
	gp_error_check(
		gp_file_get_mime_type(ptr, &mime_type),
		"Failed to get CameraFile mime type");
	return std::string_view(mime_type);
}

void gphoto_file::set_mtime(const time_t &mtime)
{
	gp_error_check(
		gp_file_set_mtime(ptr, mtime),
		"Failed to set CameraFile mtime");
}

time_t gphoto_file::get_mtime() const
{
	time_t mtime;
	gp_error_check(
		gp_file_get_mtime(ptr, &mtime),
		"Failed to get CameraFile mtime");
	return mtime;
}

// Other functions
void gphoto_file::detect_mime_type()
{
	gp_error_check(
		gp_file_detect_mime_type(ptr),
		"Failed to detect CameraFile mime type");
}

void gphoto_file::adjust_name_for_mime_type()
{
	gp_error_check(
		gp_file_adjust_name_for_mime_type(ptr),
		"Failed to adjust CameraFile name for mime type");
}

char *gphoto_file::get_name_by_type(std::string_view basename, CameraFileType type) const
{
	char *newname = nullptr;
	gp_error_check(
		gp_file_get_name_by_type(ptr, basename.data(), type, &newname),
		"Failed to get CameraFile name by type");

	return newname;
}

void gphoto_file::set_data_and_size(char *data, unsigned long int size)
{
	gp_error_check(
		gp_file_set_data_and_size(ptr, data, size),
		"Failed to set CameraFile data and size");
}

std::pair<const char *, unsigned long int> gphoto_file::get_data_and_size() const
{
	const char *data;
	unsigned long int size;
	gp_error_check(
		gp_file_get_data_and_size(ptr, &data, &size),
		"Failed to get CameraFile data and size");

	return std::make_pair(data, size);
}

gphoto_file::operator CameraFile *()
{
	return ptr;
}