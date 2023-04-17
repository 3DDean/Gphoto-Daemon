#pragma once
#include "gphoto_wrapper/common.h"

#include "gphoto-error.h"
#include "gphoto2/gphoto2-file.h"

#include <ctime>

#include <stdexcept>
#include <string>
#include <string_view>

class gphoto_file
{
  public:
	gphoto_file();
	GPHOTO_WRAPPER_COPY_AND_MOVE_CONSTRUCTOR(gphoto_file, gp_file_ref);

	gphoto_file(const int fd);
	gphoto_file(const int dirfd, std::string_view filename);
	gphoto_file(CameraFileHandler *handler, void *priv);

	~gphoto_file();

	gphoto_file &operator=(const gphoto_file &other);

	void set_name(std::string_view name);
	std::string_view get_name() const;

	void set_mime_type(std::string_view mime_type);
	std::string_view get_mime_type() const;

	void set_mtime(const time_t &mtime);
	time_t get_mtime() const;

	void detect_mime_type();
	void adjust_name_for_mime_type();

	char *get_name_by_type(std::string_view basename, CameraFileType type) const;

	void set_data_and_size(char *data, unsigned long int size);

	void save(std::string_view filename);

	std::pair<const char *, unsigned long int> get_data_and_size() const;

	operator CameraFile *();
	operator bool();

  protected:
	CameraFile *ptr;
};
