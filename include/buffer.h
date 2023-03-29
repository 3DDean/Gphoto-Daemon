#pragma once
#include <iostream>
#include <string.h>
#include <string>
#include <tuple>
#include <unistd.h>

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

	int write(auto write_func)
	{
		auto availableBytes = MaxSize - size;
		int32_t amountRead = write_func(data + size, availableBytes);

		if (amountRead < 0)
		{
			throw "UGH";
		}
		if (size + amountRead > MaxSize)
		{
			std::cout << "A buffer overflow occurred\n";
			throw "";
		}
		size += amountRead;
		return amountRead;
	}

	void clear()
	{
		size = 0;
	}

	void consume(int amount)
	{
		if (amount == size)
		{
			size = 0;
		}
		else if (amount < size)
		{
			auto dataStart = data + amount;
			auto remainingData = size - amount;

			memcpy(data, dataStart, remainingData);
		}
		else
		{
			throw "Overflow error";
		}
	}

	std::string_view to_string_view()
	{
		return std::string_view(data, size);
	}

	explicit operator std::string_view()
	{
		return std::string_view(data, size);
	}
};

template <typename BufferT>
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
