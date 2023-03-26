#pragma once
#include "buffer.h"
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>

template <std::size_t Size>
struct pipe_buffer
{
	constexpr static std::size_t maxSize = Size;
	char data[Size];
	std::size_t size = 0;

	void read_pipe(int fd)
	{
		ssize_t amountRead = ::read(fd, data + size, maxSize - size);
		if (amountRead > 0)
		{
			size += amountRead;
		}
		else if (amountRead == 0)
		{
			size = 0;
		}
		else
		{
			std::cout << "Failed to read pipe" << amountRead;
		}
	}
};

// template <std::size_t Size>
// struct buffer_reader<pipe_buffer<Size>> : buffer_reader_base
// {
// 	using Base = buffer_reader_base;
// 	using Buffer_Type = pipe_buffer<Size>;

// 	buffer_reader(pipe_buffer<Size> &_buffer)
// 		: buffer(_buffer),
// 		  buffer_reader_base(_buffer.data, _buffer.size)
// 	{}

// 	~buffer_reader()
// 	{
// 		uint32_t remainingData = Base::dataEnd - Base::dataPos;
// 		memcpy(buffer.data, Base::dataPos, remainingData);
// 		buffer.size = remainingData;
// 	}

// 	pipe_buffer<Size> &buffer;
// };

// TODO create a system for user controlled pipe flags
//  struct PipeFlags
//  {
//  	const int nonblock = O_NONBLOCK;
//  };

struct named_pipe
{
	named_pipe(const char *filePath, const int open_mode);
	named_pipe(const std::string filePath, const int open_mode);
	named_pipe(const std::filesystem::path filePath, const int open_mode);

	~named_pipe();

  protected:
	int pipe_fd;
};

template <typename BufferT>
struct read_pipe : public named_pipe
{
	// using named_pipe::fd;
	read_pipe(const char *filePath, const int open_flags = 0)
		: named_pipe(filePath, O_RDONLY | O_NONBLOCK){};
	read_pipe(const std::string filePath, const int open_flags = 0)
		: named_pipe(filePath, O_RDONLY | O_NONBLOCK){};
	read_pipe(const std::filesystem::path filePath, const int open_flags = 0)
		: named_pipe(filePath, O_RDONLY | O_NONBLOCK){};

	bool read()
	{
		int fd = named_pipe::pipe_fd;
		auto pipe_reader = [fd](void *ptr, std::size_t size)
		{
			return ::read(fd, ptr, size);
		};

		//Read from the OS owned pipe buffer to a buffer owned by the application
		return buffer.write(pipe_reader);
	}
	
	template<typename ReaderT>
	ReaderT get_reader()
	{
		return ReaderT(buffer);
	}

	std::string_view get_string_view()
	{
		return buffer.to_string_view();
	}

	void clear(){}

	BufferT buffer;
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
