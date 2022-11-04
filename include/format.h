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

// Not using the hash functions because I don't think my approach is really working
template <typename T>
struct hash
{
	using hash_t = uint32_t;

	constexpr hash(T &obj)
		// I think this is just hashing the
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

	constexpr format_var(const format_var &) = default;

	constexpr format_var() {}

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
		else
		{
			throw formatting_error{};
		}
	}
	bool operator==(const format_var &rhs) const
	{
		return type == rhs.type && label == rhs.label;
	}
	constexpr format_var(std::string_view _type, std::string_view _label)
		: type(get_type_enum(_type)), label(_label) {}

	format_type type;
	std::string_view label;
};

template <class Compare = std::less<>>
struct format_var_label_compare
{
	constexpr inline bool operator()(const format_var &lhs, const format_var &rhs)
	{
		return Compare{}(lhs.label, rhs.label);
	}
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
				vars[current] = std::string_view(start, i + 1);
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
		using const_array = std::array<std::string_view, N + 1>;

		std::pair<var_array, const_array> output;

		auto output_itt = output.first.begin();
		auto constant_view_itt = output.second.begin();

		auto constantStart = str.begin();

		for (auto var : vars)
		{
			*output_itt = format_var(var);
			*constant_view_itt = std::string_view(constantStart, var.begin());
			constantStart = var.end();
			output_itt++;
			constant_view_itt++;
		}
		if (constantStart < str.end())
		{
			*constant_view_itt = std::string_view(constantStart, str.end());
		}
		return output;
	}
};

template <typename Key, typename T, std::size_t N, class Compare>
struct ref_counter
{
	using value_type = std::pair<Key, T>;

	using Array = std::array<value_type, N>;
	using iterator = typename Array::iterator;
	Array data;
	size_t count = 0;

	iterator first = data.begin();
	iterator last = data.begin();

	struct internal_compare
	{
		constexpr inline bool operator()(const value_type &lhs, const Key &rhs)
		{
			return Compare{}(lhs.first, rhs);
		}
	};
	constexpr size_t size()
	{
		return last - data.begin();
	}

	constexpr auto begin()
	{
		return data.begin();
	}

	constexpr auto end()
	{
		return last;
	}

	constexpr void insert(Key &_str)
	{
		auto result = std::lower_bound(begin(), last, _str, internal_compare{});

		if (result == last)
		{
			*last = value_type{_str, 1};
			last++;
		}
		else
		{
			// TODO move found behavior into template parameters
			if ((*result).first == _str)
			{
				(*result).second++;
				return;
			}
			else
			{
				std::move_backward(result, last, last + 1);
				*result = value_type(_str, 1);
				last++;
			}
		}
	}
};

template <size_t N>
constexpr auto get_vars(const std::string_view _str)
{
	variable_str_constructor_helper<N> formatted_string(_str);
	auto [var_data, str_data] = formatted_string.get_var_array();

	using var_usage = std::pair<std::string_view, size_t>;

	ref_counter<format_var, size_t, N, format_var_label_compare<>> refCounter;
	for (auto var : var_data)
	{
		refCounter.insert(var);
	}

	// TODO count number of bytes required for new constant array
	// TODO remove duplicate variables and replace them with a position arg

	return str_data;
}

void format_test()
{
	static constexpr Fixed_String unprocessed_str{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
												  "<label for=${str:id}>${label}</label>"};

	constexpr size_t var_estimate = count_vars(unprocessed_str);
	auto temp = get_vars<var_estimate>(unprocessed_str);
}
