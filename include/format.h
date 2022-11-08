#pragma once
#include "Fixed_Array.h"
#include <cassert>
#include <ctre.hpp>
#include <variant>

struct formatting_error
{
	constexpr formatting_error(std::string_view _error, std::string_view str){}
};

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
		if (*i == '$')
			count++;

	return count;
}

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

struct placeholder
{};

using format_arg_variant = std::variant<constant_string, unspecified_var, str_var, int_var, float_var>;

struct format_arg : format_arg_variant
{
	using base = format_arg_variant;
	using base::operator=;
	using base::emplace;
	static constexpr Fixed_String variable_regex = "\\$\\{(.+?)\\}";

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

		if (ctre::match<variable_regex>(str))
			throw formatting_error("constant_string contains variable ", str);

		emplace<constant_string>(str);
	};

	inline constexpr auto parse_arg(std::string_view var)
	{
		auto [whole, firstCap, secondCap] = ctre::match<"(.+):(.+)">(var);

		if (firstCap == "str")
			emplace<str_var>(secondCap);
		else if (firstCap == "int")
			emplace<int_var>(secondCap);
		else if (firstCap == "float")
			emplace<float_var>(secondCap);
		else
			emplace<unspecified_var>(var);
	};
};

template <std::size_t N>
struct format_args
{
	constexpr format_args() = default;
	static constexpr std::size_t var_count = N;
	static constexpr std::size_t arg_count = var_count * 2 + 1;

	using arg_array = std::array<format_arg, arg_count>;
	using var_array = std::array<std::size_t, var_count>;

	arg_array args; // = parse_string<arg_count>(Str);
	var_array vars; // = process_vars<var_count>(args);
	//Move parse_string and process_vars
};

template <size_t N>
static inline constexpr auto process_vars(auto &args)
{
	std::array<std::size_t, N> vars;

	auto var_itt = vars.begin();

	for (auto arg_itt = args.begin(); arg_itt < args.end(); arg_itt++)
	{
		if (arg_itt->index() != 0)
		{
			*var_itt = args.end() - arg_itt;
			var_itt++;
		}
	}
	return vars;
}

template <size_t N>
static inline constexpr auto parse_string(const auto &Str)
{
	format_args<N> fmt_args;

	auto &args = fmt_args.args;

	using iterator = std::string_view::iterator;
	iterator const_start = Str.begin();
	auto arg_itt = args.begin();

	for (auto [match, var] : ctre::range<format_arg::variable_regex.data>(Str))
	{
		arg_itt++->parse_arg(const_start, match.begin());
		const_start = match.end();
		arg_itt++->parse_arg(var);
	}

	if (const_start < Str.end())
		arg_itt->parse_arg(const_start, Str.end());

	fmt_args.vars = process_vars<N>(fmt_args.args);

	return fmt_args;
};

template <auto... Args>
struct format_string;

template <Fixed_String Str>
struct format_string<Str>
{
	static constexpr Fixed_String str = Str;
	static constexpr format_args args = parse_string<count_vars(str)>(Str);

	//unfinalized call 
	constexpr auto operator()(auto... Args) const requires(sizeof...(Args) == args.var_count)
	{}
};

template <format_string Format_String, auto... Args>
struct format_string<Format_String, Args...>
{
	static constexpr format_args args = Format_String.args;
};

void format_test()
{
	constexpr format_string<Fixed_String{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
										 "<label for=${str:id}>${label}</label>"}>
		test;
}
