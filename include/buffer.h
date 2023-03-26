#pragma once
#include <string.h>
#include <string>
#include <tuple>
#include <unistd.h>
#include <iostream>

// A stack allocated buffer
template <std::size_t MaxSize>
struct stack_buffer
{
	using size_type = std::uint32_t;
	char data[MaxSize];
	std::size_t size = 0;

	size_type max_size() const noexcept
	{
		return MaxSize;
	}
	size_type bytes_available()
	{
		return MaxSize - size;
	}

	char *back()
	{
		return data + size;
	}

	bool write(auto write_func)
	{
		int32_t amountRead = write_func(data + size, MaxSize - size);
		if (amountRead <= 0)
		{
			// std::cout << "A buffer read returned an error " << amountRead << "\n";
			return false;
		}
		else
		{
			if (size + amountRead > MaxSize)
			{
				std::cout << "A buffer overflow occurred\n";
				throw "";
			}
			size += amountRead;
			return true;
		}
	}
	void clear()
	{
		size = 0;
	}
	// void read()
	
	std::string_view to_string_view()
	{
		return std::string_view(data, size);
	}
};

struct buffer_reader
{
	buffer_reader(char *buffer, size_t nBytes)
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
		// The reason this is less than or equal to is because we are checking pointer positions
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
	const char *dataEnd;
};

// TODO endianness management
//  #include <bit>
//  template<std::endian Endianness>
struct buffer_reader_base
{
	buffer_reader_base(char *buffer, size_t nBytes)
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
		// The reason this is less than or equal to is because we are checking pointer positions
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
	const char *dataEnd;
};
