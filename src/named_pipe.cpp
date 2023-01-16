#include "named_pipe.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

named_pipe::named_pipe(const char *filepath, const int open_mode)
{
	int res = 0;
	if (access(filepath, F_OK) == -1)
	{
		res = mkfifo(filepath, 0777);
		if (res != 0)
		{
			perror("Could not create fifo\n");
			exit(EXIT_FAILURE);
		}
	}
	pipe_fd = open(filepath, open_mode);
	if (pipe_fd == -1)
	{
		perror("Could not open fifo\n");
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