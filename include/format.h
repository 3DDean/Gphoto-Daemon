#pragma once
#include "Fixed_Array.h"
#include <ctre.hpp>

struct formatting_error
{};

// https://www.partow.net/programming/hashfunctions/

constexpr uint32_t ELFHash(const char *str, unsigned int length)
{
	unsigned int hash = 0;
	unsigned int x = 0;
	unsigned int i = 0;

	for (i = 0; i < length; ++str, ++i)
	{
		hash = (hash << 4) + (*str);

		if ((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
		}

		hash &= ~x;
	}

	return hash;
}

template <typename T>
struct hash
{
	using hash_t = uint32_t;

	constexpr hash(T &obj)
		: hash_code(ELFHash((char *)&obj, sizeof(T)))
	{}

	constexpr operator hash_t()
	{
		return hash_code;
	}
	hash_t hash_code;
};

constexpr size_t count_vars(std::string_view _str)
{
	std::size_t count = 0;

	for (auto i = _str.begin(); i < _str.end(); i++)
	{
		if (*i == '$')
		{
			count++;
		}
	}
	return count;
}

enum format_type
{
	not_set,
	String,
	Integer,
	Float
};

struct var_type_pair
{
	constexpr var_type_pair(format_type value, std::string_view _view)
		: enum_value(value), view(_view)
	{}

	format_type enum_value;
	std::string_view view;

	constexpr bool compare(std::string_view _str, format_type &_output)
	{
		if (_str == view)
		{
			_output = enum_value;
			return true;
		}
		return false;
	}
};

constexpr format_type get_type_enum(std::string_view _str)
{
	std::array enum_array = {var_type_pair(format_type::String, "str"), var_type_pair(format_type::Integer, "int"), var_type_pair(format_type::Float, "float")};
	format_type result = not_set;

	for (auto var : enum_array)
		if (var.compare(_str, result))
			break;

	return result;
};

struct format_var
{
	using hash_t = size_t;
	constexpr format_var(){}

	constexpr format_var(std::string_view str)
	{
		auto regex = ctre::match<"(.+):(.+)">;

		auto [wholeMatch, var] = ctre::match<"\\$\\{(.+)\\}">(str);

		if (var)
		{
			auto [whole, firstCap, secondCap] = regex(var);

			if (whole)
			{
				type = get_type_enum(firstCap);
				label = secondCap;
			}
			else
			{
				type = get_type_enum(std::string_view());
				// format_type::not_set;
				label = var;
			}
		}
		else{
			throw formatting_error{};
		}
	}

	constexpr format_var(std::string_view _type, std::string_view _label) : type(get_type_enum(_type)), label(_label){}

	format_type type;
	std::string_view label;
};

template <size_t N>
struct formatted_string
{
	constexpr formatted_string() {}
	std::array<std::string_view, N + 1> constants;
	std::array<std::string_view, N> vars;
};


template <size_t N>
struct variable_str_constructor_helper
{
	std::string_view str;
	std::array<std::string_view, N> vars;
	std::size_t current = 0;

	constexpr variable_str_constructor_helper(std::string_view _str) : str(_str)
	{
		for (auto i = str.begin(); i < str.end(); i++)
		{
			switch (*i)
			{
			case '$':
				i = set_variable(std::string_view(i, str.end()));
				break;
			case '{':
			case '}':
				throw formatting_error{};
			}
		}
	}

	constexpr auto set_variable(std::string_view _str)
	{
		auto start = _str.begin();

		for (auto i = start; i < _str.end(); i++)
		{
			switch (*i)
			{
			case '$':
			case '{':
				break;
			case '}':
				vars[current] = std::string_view(start, i+1);
				current++;
				return i;
			}
		}
		throw formatting_error{};
	}

	constexpr auto to_formatted_var(std::string_view str)
	{
		auto getVar = ctre::match<"\\$\\{(.+)\\}">;
		auto regex = ctre::match<"(.+):(.+)">;

		auto [wholeMatch, var] = getVar(str);

		if (var)
		{
			auto [whole, firstCap, secondCap] = regex(str);

			if (whole)
			{
				return format_var(firstCap, secondCap);
			}
			else
			{
				return format_var(std::string_view(), var);
			}
		}
	}

	constexpr auto get_var_array()
	{
		using var_array = std::array<format_var, N>;
		using const_array = std::array<std::string_view, N+1>;

		std::pair<var_array, const_array> output;

		auto output_itt = output.first.begin();
		auto constant_view_itt = output.second.begin();

		auto constantStart = str.begin();

		for(auto var : vars)
		{
			*output_itt = format_var(var);
			*constant_view_itt = std::string_view(constantStart, var.begin());
			constantStart = var.end();
			output_itt++;
			constant_view_itt++;
		}
		if(constantStart < str.end())
		{
			*constant_view_itt = std::string_view(constantStart, str.end());
		}
		return output;
	}
};

template <size_t N>
constexpr auto get_vars(const std::string_view _str)
{
	variable_str_constructor_helper<N> formatted_string(_str);
	auto [var_data, str_data]= formatted_string.get_var_array();

	//TODO count number of bytes required for new constant array
	//TODO remove duplicate variables and replace them with a position arg

	return str_data;
}

void format_test()
{
	static constexpr Fixed_String unprocessed_str{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
												  "<label for=${str:id}>${label}</label>"};

	constexpr size_t var_estimate = count_vars(unprocessed_str);
	auto temp = get_vars<var_estimate>(unprocessed_str);
}
