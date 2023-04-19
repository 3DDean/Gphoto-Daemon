#pragma once
#include "buffer.h"
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

struct named_pipe
{
	named_pipe(const char *filePath, const int open_mode);
	named_pipe(const std::string filePath, const int open_mode);
	named_pipe(const std::filesystem::path filePath, const int open_mode);

	~named_pipe();

  protected:
	int pipe_fd;
};

struct read_pipe : public named_pipe
{
	read_pipe(const char *filePath, const int open_flags = 0);
	read_pipe(const std::string filePath, const int open_flags = 0);
	read_pipe(const std::filesystem::path filePath, const int open_flags = 0);

	int required_bytes();

	int read_to(char *ptr, std::size_t size);
};

struct write_pipe : public named_pipe
{
	write_pipe(const char *filePath)
		: named_pipe(filePath, O_RDWR | O_NONBLOCK){};
	write_pipe(const std::string filePath)
		: named_pipe(filePath, O_RDWR | O_NONBLOCK){};
	write_pipe(const std::filesystem::path filePath)
		: named_pipe(filePath, O_RDWR | O_NONBLOCK){};

	void write(std::string_view str)
	{
		::write(named_pipe::pipe_fd, str.data(), str.size());
	}
};
