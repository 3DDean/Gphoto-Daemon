#pragma once
#include <string.h>
#include <string>
#include <tuple>
#include <unistd.h>

// TODO endianness management
//  #include <bit>
//  template<std::endian Endianness>
struct BufferReader_Base
{
	BufferReader_Base(char *buffer, size_t nBytes)
		: dataPos(buffer),
		  dataEnd(buffer + nBytes){};

	template <typename... ArgsT>
	bool read(std::tuple<ArgsT...> &_parameters)
	{
		return read_tuple(_parameters, std::make_index_sequence<sizeof...(ArgsT)>());
	}

	template <typename T>
	bool read(T &_value)
	{
		char *readEnd = dataPos + sizeof(T);
		if (readEnd <= dataEnd)
		{
			read_internal(&_value);
			return true;
		}
		return false;
	}

  protected:
	template <typename... ArgsT, std::size_t... I>
	bool read_tuple(std::tuple<ArgsT...> &_parameters, std::index_sequence<I...>)
	{
		char *readEnd = dataPos + (sizeof(ArgsT) + ...);
		if (readEnd < dataEnd)
		{
			(read_internal(std::get<I>(_parameters)), ...);
			return true;
		}
		return false;
	}

	template <typename T>
	void read_internal(T *_value)
	{
		_value = (T *)(dataPos);
		dataPos += sizeof(T);
	}

	// char* dataBuffer;
	// char *bufferEnd;
	char *dataPos;
	char *dataEnd;
};

template <typename T>
struct BufferReader;



// template <std::size_t Size, typename FriendT>
// struct ReadBuffer
// {
// 	template <typename T>
// 	bool getParameters(T &_parameter)
// 	{
// 		std::size_t bytesRequired = sizeof(T);
// 		if (bytesRequired < bytesHeld - pos)
// 		{
// 			_parameter = getVar<T>();
// 			lastReadResult = true;
// 		}
// 		else
// 		{
// 			lastReadResult = false;
// 		}
// 		return lastReadResult;
// 		// T *out = (T *)(bytes[pos]);
// 		// pos += sizeof(T);
// 		// return (pos < bytesHeld) ? out : nullptr;
// 	}

// 	template <typename... Args>
// 	bool getParameters(std::tuple<Args...> &_parameters)
// 	{
// 		std::size_t bytesRequired = (sizeof(Args) + ...);
// 		if (bytesRequired < bytesHeld - pos)
// 		{
// 			_parameters = std::make_tuple(getVar<Args>()...);
// 			lastReadResult = true;
// 		}
// 		else
// 		{
// 			lastReadResult = false;
// 		}
// 		return lastReadResult;

// 		return false;
// 		// T *out = (T *)(bytes[pos]);
// 		// pos += sizeof(T);
// 		// return (pos < bytesHeld) ? out : nullptr;
// 	}

// 	uint16_t getBytesHeld()
// 	{
// 		return bytesHeld;
// 	}
// 	bool isGood()
// 	{
// 		return lastReadResult;
// 	}

// 	template <typename T>
// 	T getVar()
// 	{
// 		T *out = (T *)(bytes + pos);
// 		pos += sizeof(T);
// 		return *(out);
// 		//(pos < bytesHeld) ? out : nullptr;
// 	}
// 	private:
// 	// friend struct named_pipe<Size>;
// 	friend FriendT;
// 	bool lastReadResult;
// 	char bytes[Size];
// 	// char bytes;
// 	uint16_t pos;
// 	int32_t bytesHeld;

// 	void reset()
// 	{
// 		lastReadResult = true;
// 		long remaining = bytesHeld - pos;
// 		if (remaining > 0)
// 		{
// 			memcpy(bytes + pos, bytes, remaining);
// 			bytesHeld = remaining;
// 			pos = 0;
// 		}
// 		else
// 		{
// 			bytesHeld = 0;
// 			pos = 0;
// 		}
// 	}
// 	std::size_t getUnwrittenSpace()
// 	{
// 		return Size - bytesHeld;
// 	}
// 	char *data() { return bytes + bytesHeld; }
// 	// static constexpr std::size_t size = Size;
// };