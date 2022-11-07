#pragma once
#include "Fixed_Array.h"
#include <cassert>
#include <ctre.hpp>
#include <variant>

struct formatting_error
{};

template <Fixed_String ErrorStr>
class version_error : std::exception
{
  public:
	constexpr version_error() = default;

	virtual constexpr const char *what() const noexcept
	{
		return ErrorStr;
	}
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

struct constant_string : public std::string_view
{
	using base = std::string_view;
	using base::begin;

	constexpr constant_string() = default;
	constexpr constant_string(auto str)
		: std::string_view(str) {}
};

struct str_var : public std::string_view
{
	using base = std::string_view;
	using base::begin;
	constexpr str_var() = default;
	constexpr str_var(auto str)
		: std::string_view(str) {}
};
struct unspecified_var : public std::string_view
{
	using base = std::string_view;
	using base::begin;
	constexpr unspecified_var() = default;
	constexpr unspecified_var(auto str)
		: std::string_view(str) {}
};

struct int_var : public std::string_view
{
	using base = std::string_view;
	using base::begin;
	constexpr int_var() = default;
	constexpr int_var(auto str)
		: std::string_view(str) {}
};

struct float_var : public std::string_view
{
	using base = std::string_view;
	using base::begin;
	constexpr float_var() = default;
	constexpr float_var(auto str)
		: std::string_view(str) {}
};

constexpr format_type get_type_enum(std::string_view _str)
{
	const std::array enum_array = {std::pair("str", format_type::String), std::pair("int", format_type::Integer), std::pair("float", format_type::Float)};

	for (auto var : enum_array)
		if (var.first == _str)
			return var.second;

	return format_type::not_set;
};

struct format_var
{
	constexpr format_var() {}
	constexpr format_var(const format_var &) = default;

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

template <typename... Types>
using variant_with_ref = std::variant<Types..., std::reference_wrapper<Types>...>;

using format_arg_variant = std::variant<constant_string, unspecified_var, str_var, int_var, float_var>;

struct format_arg : format_arg_variant
{
	using base = format_arg_variant;
	using base::operator=;

	using base::emplace;
	using base::swap;

	using base::index;
	using base::valueless_by_exception;

	constexpr format_arg() = default;

	template <typename T, typename... ArgsT>
	inline constexpr auto emplace(ArgsT &&...Args)
	{
		if constexpr (__cpp_lib_variant >= 202106L)
			base::template emplace<T>(Args...);
		else
			base::operator=(base(std::in_place_type<T>, std::forward<ArgsT>(Args)...));
	};

	template <typename Itt, typename End>
	inline constexpr auto parse_arg(Itt start, End end)
	{
		std::string_view str(start, end);
		auto regex = ctre::match<"(.+):(.+)">;
		auto [wholeMatch, var] = ctre::match<"\\$\\{(.+)\\}">(str);

		if (var)
		{
			auto [whole, firstCap, secondCap] = regex(var);

			if (whole)
			{
				if (firstCap == "str")
				{
					emplace<str_var>(secondCap);
				}
				else if (firstCap == "int")
				{
					emplace<int_var>(secondCap);
				}
				else if (firstCap == "float")
				{
					emplace<float_var>(secondCap);
				}

				return;
			}

			emplace<unspecified_var>(var);
		}
		else
		{
			emplace<constant_string>(str);
		}
	};
};

template <std::size_t N>
struct formatted_string
{
	std::array<format_arg, N> args;
	formatted_string(const std::string_view _str)
	{}
};

template <size_t N>
struct format_string_data
{
	std::array<format_arg, N> args;

	template <size_t StrN>
	constexpr format_string_data(const Fixed_String<StrN> &Str)
	{
		using itterator = std::string_view::iterator;
		itterator const_start = Str.begin();
		auto arg_itt = args.begin();

		auto set_variable = [&arg_itt, &const_start](auto start, const auto end)
		{
			for (auto i = start; i < end; i++)
			{
				switch (*i)
				{
				case '$':
				case '{':
					break;
				case '}':
					arg_itt++->parse_arg(const_start, start);

					const_start = i + 1;

					arg_itt++->parse_arg(start, const_start);

					return i;
				}
			}
			throw formatting_error{};
		};

		for (auto i = Str.begin(); i < Str.end(); i++)
		{
			switch (*i)
			{
			case '$':
				i = set_variable(i, Str.end());
				break;
			case '{':
			case '}':
				throw formatting_error{};
			}
		}

		if (const_start < Str.end())
			arg_itt->parse_arg(const_start, Str.end());
	}
};

// TODO add this to html elements and figure out the deduction crap
// Todo convert this to a class
template <Fixed_String Str>
constexpr auto format_string()
{
	constexpr size_t arg_count = count_vars(Str) * 2 + 1;
	constexpr format_string_data<arg_count> output(Str);

	return output;
}

void format_test()
{
	constexpr auto formatted_string = format_string<Fixed_String{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
																 "<label for=${str:id}>${label}</label>"}>();
}
