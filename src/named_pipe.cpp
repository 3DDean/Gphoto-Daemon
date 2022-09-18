#include "named_pipe.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "Logger.h"

named_pipe::named_pipe(const char *filepath, const int open_mode)
{
	int res = 0;
	if (access(filepath, F_OK) == -1)
	{
		res = mkfifo(filepath, 0777);
		if (res != 0)
		{
			logError("Could not create fifo %s\n", filepath);
			exit(EXIT_FAILURE);
		}
	}
	pipe_fd = open(filepath, open_mode);
	if (pipe_fd == -1)
	{
		logError("Could not open fifo %s in %i\n", filepath, open_mode);
		exit(EXIT_FAILURE);
	}
}
named_pipe::named_pipe(const std::string filePath, const int open_mode) : named_pipe(filePath.c_str(), open_mode)
{}

named_pipe::named_pipe(const std::filesystem::path filePath, const int open_mode) : named_pipe(filePath.c_str(), open_mode)
{}

named_pipe::~named_pipe()
{
	close(pipe_fd);
}