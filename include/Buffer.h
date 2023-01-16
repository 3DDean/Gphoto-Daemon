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
		//The reason this is less than or equal to is because we are checking pointer positions
		if (readEnd <= dataEnd)
		{
			read_internal(&_value);
			return true;
		}
		return false;
	}

  protected:
	template <typename... ArgsT, std::size_t... I>
	inline bool read_tuple(std::tuple<ArgsT...> &_parameters, std::index_sequence<I...>)
	{
		char *readEnd = dataPos + (sizeof(ArgsT) + ...);
		if (readEnd <= dataEnd)
		{
			(read_internal(std::get<I>(_parameters)), ...);
			return true;
		}
		return false;
	}

	template <typename T>
	inline void read_internal(T *_value)
	{
		_value = (T *)(dataPos);
		dataPos += sizeof(T);
	}

	char *dataPos;
	char *dataEnd;
};

template <typename T>
struct BufferReader;
