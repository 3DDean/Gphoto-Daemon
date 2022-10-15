#pragma once
#include <fcntl.h>
#include <filesystem>
#include <string.h>
#include <string>
#include <unistd.h>
#include "Buffer.h"
	
template <std::size_t Size>
struct pipe_buffer
{
	constexpr static std::size_t maxSize = Size;
	char data[Size];
	std::size_t size = 0;

	void read_pipe(int fd)
	{
		ssize_t amountRead = ::read(fd, data + size, maxSize - size);
		if(amountRead > 0)
		{
			size += amountRead;
		}
		else if (amountRead == 0)
		{
			size = 0;
		}
		else
		{
			std::printf("Failed to read pipe, %i", amountRead);
		}
	}
};

template <std::size_t Size>
struct BufferReader<pipe_buffer<Size>> : BufferReader_Base
{
	using Base = BufferReader_Base;
	using Buffer_Type = pipe_buffer<Size>;

	BufferReader(pipe_buffer<Size> &_buffer)
		: buffer(_buffer),
		BufferReader_Base(_buffer.data, _buffer.size)
	{}

	~BufferReader()
	{
		uint32_t remainingData = Base::dataEnd - Base::dataPos;
		memcpy(buffer.data, Base::dataPos, remainingData);
		buffer.size = remainingData;
	}

	pipe_buffer<Size> &buffer;
};

//TODO create a system for user controlled pipe flags
// struct PipeFlags
// {
// 	const int nonblock = O_NONBLOCK;
// };

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
	read_pipe(const char *filePath, const int open_flages = 0)
		: named_pipe(filePath, O_RDONLY | O_NONBLOCK){};
	read_pipe(const std::string filePath, const int open_flages = 0)
		: named_pipe(filePath, O_RDONLY | O_NONBLOCK){};
	read_pipe(const std::filesystem::path filePath, const int open_flages = 0)
		: named_pipe(filePath, O_RDONLY | O_NONBLOCK){};

	template<std::size_t Size>
	using PipeReader = BufferReader<pipe_buffer<Size>>;

	template<std::size_t Size>
	auto read(pipe_buffer<Size>& buffer)
	{
		buffer.read_pipe(named_pipe::pipe_fd);
		
		return PipeReader<Size>(buffer);
	}
};

struct write_pipe : public named_pipe
{
	// write_pipe(const char *filePath)
	// 	: named_pipe(filePath, O_WRONLY){};
	// write_pipe(const std::string filePath)
	// 	: named_pipe(filePath, O_WRONLY){};
	// write_pipe(const std::filesystem::path filePath)
	// 	: named_pipe(filePath, O_WRONLY){};
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

	// void write(std::string str)
	// {
	// 	::write(named_pipe::pipe_fd, str.data(), str.size());
	// }
	// O_WRONLY
};