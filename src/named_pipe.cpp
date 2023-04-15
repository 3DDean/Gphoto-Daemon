#include "named_pipe.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef FIFO_PERMISSIONS
#define FIFO_PERMISSIONS 0777
#endif

named_pipe::named_pipe(const char *filepath, const int open_mode)
{
	if (access(filepath, F_OK) == -1)
		if (mkfifo(filepath, FIFO_PERMISSIONS) != 0)
			throw std::runtime_error(std::string("Could not create fifo: ") + strerror(errno));

	pipe_fd = open(filepath, open_mode);
	if (pipe_fd == -1)
		throw std::runtime_error(std::string("Could not open fifo: ") + strerror(errno));
}

named_pipe::named_pipe(const std::string filePath, const int open_mode)
	: named_pipe(filePath.c_str(), open_mode)
{}

named_pipe::named_pipe(const std::filesystem::path filePath, const int open_mode)
	: named_pipe(filePath.c_str(), open_mode)
{}

named_pipe::~named_pipe()
{
	close(pipe_fd);
}