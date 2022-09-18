#pragma once
#include <filesystem>
#include <string.h>
#include <string>
#include <unistd.h>

//TODO make specializations for RO, WO, WR pipes


// template <std::size_t Size>
struct named_pipe;

template <std::size_t Size>
struct Buffer
{
	template <typename T>
	bool getParameters(T &_parameter)
	{
		std::size_t bytesRequired = sizeof(T);
		if (bytesRequired < bytesHeld - pos)
		{
			_parameter = getVar<T>();
			lastReadResult = true;
		}
		else
		{
			lastReadResult = false;
		}
		return lastReadResult;
		// T *out = (T *)(bytes[pos]);
		// pos += sizeof(T);
		// return (pos < bytesHeld) ? out : nullptr;
	}

	template <typename... Args>
	bool getParameters(std::tuple<Args...> &_parameters)
	{
		std::size_t bytesRequired = (sizeof(Args) + ...);
		if (bytesRequired < bytesHeld - pos)
		{
			_parameters = std::make_tuple(getVar<Args>()...);
			lastReadResult = true;
		}
		else
		{
			lastReadResult = false;
		}
		return lastReadResult;

		return false;
		// T *out = (T *)(bytes[pos]);
		// pos += sizeof(T);
		// return (pos < bytesHeld) ? out : nullptr;
	}

	uint16_t getBytesHeld()
	{
		return bytesHeld;
	}
	bool isGood()
	{
		return lastReadResult;
	}

  private:
	template <typename T>
	T getVar()
	{
		T *out = (T *)(bytes+pos);
		pos += sizeof(T);
		return *(out);
		//(pos < bytesHeld) ? out : nullptr;
	}
	// friend struct named_pipe<Size>;
	friend struct named_pipe;
	bool lastReadResult;
	char bytes[Size];
	// char bytes;
	uint16_t pos;
	uint16_t bytesHeld;

	void reset()
	{
		lastReadResult=true;
		long remaining = bytesHeld - pos;
		if (remaining > 0)
		{
			memcpy(bytes +pos, bytes, remaining);
			bytesHeld = remaining;
			pos = 0;
		}
		else
		{
			bytesHeld = 0;
			pos = 0;
		}
	}
	std::size_t getUnwrittenSpace()
	{
		return Size - bytesHeld;
	}
	char *data() { return bytes + bytesHeld; }
	// static constexpr std::size_t size = Size;
};

// template <std::size_t Size>
struct named_pipe
{
	named_pipe(const char *filePath, const int open_mode);
	named_pipe(const std::string filePath, const int open_mode);
	named_pipe(const std::filesystem::path filePath, const int open_mode);

	~named_pipe();

	using PipeBuffer = Buffer<512>;

	PipeBuffer &read()
	{
		buffer.reset();
		buffer.bytesHeld = ::read(pipe_fd, buffer.data(), buffer.getUnwrittenSpace());

		return buffer;
		// return ::read(pipe_fd, buffer, nbytes);
	}
	// int write()
	// {

	// }

  protected:
	PipeBuffer buffer;
	int pipe_fd;
};
