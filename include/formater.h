#pragma once
#include "Fixed_Array.h"
#include <ostream>
#include <string>
#include <type_traits>

void formater_write(auto &formater, const char *str)
{
	formater.put('\"');
	formater << str;
	formater.put('\"');
}

template <typename T>
requires std::is_arithmetic_v<T>
void formater_write(auto &formater, const T str)
{
	formater << std::to_string(str);
}
template <std::size_t Size>
void formater_write(auto &output_stream, const Fixed_String<Size> &value)
{
	output_stream << value.data;
}

struct line_formater
{
	std::ostream &output_stream;
	bool isFirstWrite = true;

	line_formater(std::ostream &output)
		: output_stream(output)
	{
	}
	~line_formater()
	{
		output_stream.put('\n');
	}

	line_formater &operator<<(const auto &value)
	{
		if (!isFirstWrite)
		{
			output_stream.put(',');
		}
		else
		{
			isFirstWrite = false;
		}
		formater_write(output_stream, value);
		return *this;
	}
};

struct formater
{
	formater(std::ostream &ouput)
		: output_stream(ouput)
	{
	}

	~formater()
	{
		if (indent_amount > 0)
			perror("Not all scopes were popped");

		for (std::size_t i = 0; i < indent_amount; i++)
		{
			output_stream << ("\n");
		}
	}
	void push_scope()
	{
		indent_amount++;
		output_stream << "\n{\n";
	}
	void pop_scope()
	{
		if (indent_amount == 0)
			throw std::runtime_error("No scope to pop");

		output_stream << "}";
		indent_amount--;
	}
	line_formater get_line_formater()
	{
		return line_formater(output_stream);
	}

	std::size_t indent_amount = 0;
	std::ostream &output_stream;
};