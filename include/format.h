#pragma once
#include "Fixed_Array.h"
#include <ctre.hpp>

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

template <size_t N>
struct formatted_string
{
	constexpr formatted_string() {}
	std::array<std::string_view, N + 1> constants;
	std::array<std::string_view, N> vars;
};

struct formatting_error
{};

template <size_t N>
struct variable_str_constructor_helper : formatted_string<N>
{
	using formatted_string<N>::constants;
	using formatted_string<N>::vars;

	std::size_t current = 0;

	constexpr variable_str_constructor_helper() {}

	constexpr void set_constant(auto begin, auto end)
	{
		constants[current] = std::string_view(begin, end);
	}
	constexpr auto set_variable(std::string_view _str)
	{
		auto start = _str.begin();
		if (start[0] == '$' && start[1] == '{' && start[2] != '}')
		{
			start = start + 2;
		}

		for (auto i = start; i < _str.end(); i++)
		{
			switch (*i)
			{
			case '$':
			case '{':
				throw formatting_error{};
			case '}':
				vars[current] = std::string_view(start, i);
				current++;
				return i;
			}
		}
		throw formatting_error{};
	}
	constexpr void set_last_constant(auto end)
	{
		if (current == N)
		{
			set_constant(vars[N - 1].end() + 1, end);
		}
		else
		{
			throw formatting_error{};
		}
	}
};

template <size_t N>
constexpr formatted_string<N> get_vars(const std::string_view _str)
{
	variable_str_constructor_helper<N> formatted_string;

	std::size_t current = 0;
	auto lastMatch = _str.begin();

	for (auto i = _str.begin(); i < _str.end(); i++)
	{
		switch (*i)
		{
		case '$':
			formatted_string.set_constant(lastMatch, i);
			i = formatted_string.set_variable(std::string_view(i, _str.end()));
			lastMatch = i + 1;
			break;
		case '{':
		case '}':
			throw formatting_error{};
		}
	}
	if (_str.end() > lastMatch)
	{
		formatted_string.set_last_constant(_str.end());
	}

	return formatted_string;
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

constexpr format_type get_type(std::string_view _str)
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
	constexpr format_var() {}
	format_var(std::string_view type, std::string_view label)
		: type(get_type(type)), label(hash(label))
	{}
	format_type type;
	hash_t label;
};

void format_test()
{
	static constexpr Fixed_String unprocessed_str{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
												  "<label for=${str:id}>${label}</label>"};

	constexpr size_t var_estimate = count_vars(unprocessed_str);
	formatted_string formater_str = get_vars<var_estimate>(unprocessed_str);
	std::array<format_var, var_estimate> vars;

	auto currentVar = vars.begin();
	auto regex = ctre::match<"(.+):(.+)">;
	auto hash = std::hash<std::string_view>{};
	for (auto var : formater_str.vars)
	{
		auto [whole, firstCap, secondCap] = regex(var);

		if (whole)
		{
			*currentVar = format_var{firstCap, secondCap};
		}
		else
		{
			*currentVar = format_var{std::string_view(), var};
		}
		++currentVar;
	}
	// TODO process variables to get their type and id
}
