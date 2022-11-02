#pragma once
#include "Fixed_Array.h"

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
	if (_str.end() < lastMatch)
	{
		formatted_string.set_last_constant(_str.end());
	}
	return formatted_string;
}

void format_test()
{
	constexpr Fixed_String unprocessed_str{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
										   "<label for=${str:id}>${label}</label>"};
	constexpr size_t var_estimate = count_vars(unprocessed_str);
	formatted_string formater_str = get_vars<var_estimate>(unprocessed_str);
}
