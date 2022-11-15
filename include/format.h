#pragma once
#include "Fixed_Array.h"
#include "tuple.h"
#include <cassert>
#include <ctre.hpp>
#include <variant>
#include <vector>

struct formatting_error
{
	constexpr formatting_error() {}
	constexpr formatting_error(std::string_view _error, std::string_view str) {}
};

// Not yet implemented error
struct nyi_error
{
	constexpr nyi_error() {}
	constexpr nyi_error(std::string_view _error, std::string_view str) {}
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

constexpr size_t count_vars(const std::string_view _str) noexcept
{
	std::size_t count = 0;

	for (auto i : _str)
		if (i == '$')
			++count;

	return count;
}

template <std::size_t VarCount>
static inline constexpr std::size_t arg_count = VarCount * 2 + 1;

struct constant_string : public std::string_view
{
	static inline constexpr Fixed_String match_str = "";

	using base = std::string_view;
	using base::begin;
	std::size_t var_type = 0;

	constexpr constant_string() = default;
	constexpr constant_string(std::string_view str, std::size_t var_type = 0)
		: std::string_view(str), var_type(var_type)
	{}
	// constexpr constant_string(auto var) requires std::invocable<decltype(var.index())> : std::string_view(var), var_type(var.index()) {}
};

struct unspecified_var : public std::string_view
{
	static inline constexpr Fixed_String match_str = "";
	using base = std::string_view;
	using base::begin;
	constexpr unspecified_var() = default;
	constexpr unspecified_var(auto str)
		: std::string_view(str) {}
};

struct str_var : public std::string_view
{
	static inline constexpr Fixed_String match_str = "str";
	using base = std::string_view;
	using base::begin;
	constexpr str_var() = default;
	constexpr str_var(auto str)
		: std::string_view(str) {}
};

struct int_var : public std::string_view
{
	static inline constexpr Fixed_String match_str = "int";

	using base = std::string_view;
	using base::begin;
	constexpr int_var() = default;
	constexpr int_var(auto str)
		: std::string_view(str) {}
};

struct float_var : public std::string_view
{
	static inline constexpr Fixed_String match_str = "float";

	using base = std::string_view;
	using base::begin;
	constexpr float_var() = default;
	constexpr float_var(auto str)
		: std::string_view(str) {}
};

struct placeholder
{};

static inline constexpr auto count_args(const placeholder &)
{
	return 1;
}

template <auto... Args>
struct format_string;

static inline constexpr std::size_t type_not_found = -1;

template <std::size_t I, typename T, typename Head, typename... ArgsT>
constexpr inline std::size_t get_index()
{
	if constexpr (std::same_as<T, Head>)
		return I;
	else if constexpr (sizeof...(ArgsT) > 0)
		return get_index<I + 1, T, ArgsT...>();
	else
		return type_not_found;
}

template <typename T, template <typename...> class container, typename... ArgsT>
constexpr inline std::size_t get_index(container<ArgsT...>)
{
	return get_index<0, T, ArgsT...>();
}

template <std::size_t Index>
struct index_string_view
{
	static constexpr std::size_t index() { return Index; }
	std::string_view str;

	constexpr bool operator==(std::string_view str2) const
	{
		return str == str2;
	}
};

using arg_variant = std::variant<constant_string, str_var, int_var, float_var, unspecified_var>;

struct arg : arg_variant
{
	using base = arg_variant;

	template <typename Type>
	static inline constexpr auto index_of = get_index<Type>(base());

	template <typename Type>
	static inline constexpr auto string_index_pair = index_string_view<index_of<Type>>{Type::match_str};

	template <typename... Types>
	static inline constexpr auto string_index_tuple = std::make_tuple(string_index_pair<Types>...);

	static inline constexpr auto indexed_tuple = string_index_tuple<str_var, int_var, float_var>;

	using base::emplace;
	static constexpr Fixed_String variable_regex = "\\$\\{(.+?)\\}";

	constexpr arg() = default;

	template <typename T, typename... ArgsT>
	inline constexpr auto emplace(ArgsT &&...Args)
	{
		if constexpr (__cpp_lib_variant >= 202106L)
			base::template emplace<T>(Args...);
		else
			base::operator=(base(std::in_place_type<T>, std::forward<ArgsT>(Args)...));
	};

	template <std::size_t I, typename... ArgsT>
	inline constexpr auto emplace(ArgsT &&...Args)
	{
		if constexpr (__cpp_lib_variant >= 202106L)
			base::template emplace<I>(Args...);
		else
			base::operator=(base(std::in_place_index<I>, std::forward<ArgsT>(Args)...));
	};

	template <typename Itt, typename End>
	inline constexpr arg(Itt start, End end)
	{
		std::string_view str(start, end);

		if (ctre::match<variable_regex>(str))
			throw formatting_error("constant_string contains variable ", str);

		emplace<constant_string>(str);
	};

	inline constexpr arg(std::string_view var)
	{
		auto [whole, firstCap, secondCap] = ctre::match<"(.+):(.+)">(var);
		std::string_view second_cap = secondCap;
		std::string_view first_cap = firstCap;

		bool matched = false;
		auto matcher = [&](const auto test) -> bool
		{
			if (test == first_cap)
			{
				emplace<test.index()>(second_cap);
				matched = true;
				return false;
			}
			return true;
		};
		tupleForEach(matcher, indexed_tuple);
		if (matched)
			return;

		emplace<unspecified_var>(var);
	};

	inline constexpr bool is_constant() const noexcept { return index_of<constant_string> == 0; }
};

static inline constexpr Fixed_String variable_regex = "\\$\\{(.+?)\\}";
static inline constexpr Fixed_String attribute_regex = "(.+):(.+)";

template <std::size_t Size>
struct format_args_helper
{
	constexpr format_args_helper() = default;
	std::array<arg, Size> arg_array;
	std::array<std::size_t, Size> index_array;

	constexpr void get_indexes()
	{
		auto index_itt = index_array.begin();
		for (auto var : arg_array)
		{
			*index_itt = var.index();
			index_itt++;
		}
	}
};

template <std::size_t VarCount>
static inline consteval auto parse_string(const std::string_view str)
{
	constexpr std::size_t array_size = arg_count<VarCount>;
	format_args_helper<array_size> output;

	auto output_itt = output.arg_array.begin();
	auto assign_arg = [](auto &itt, auto... args)
	{
		*itt = arg(args...);
		++itt;
	};

	auto const_start = str.begin();

	for (auto [match, var] : regex_range<variable_regex>(str))
	{
		assign_arg(output_itt, const_start, match.begin());
		assign_arg(output_itt, var.to_view());

		const_start = match.end();
	}

	if (const_start < str.end())
		assign_arg(output_itt, const_start, str.end());

	output.get_indexes();
	return output;
}

template <auto Index_Array, typename T, T... index>
constexpr auto arg_array_to_tuple_w_index(auto container, std::integer_sequence<T, index...>)
{
	return std::make_tuple(std::get<Index_Array[index]>(container[index])...);
}

template <auto Index_Array>
static inline constexpr auto arg_array_to_tuple(auto ArgArray)
{
	return arg_array_to_tuple_w_index<Index_Array>(ArgArray, std::make_index_sequence<ArgArray.size()>());
};

template <Fixed_String Str>
struct format_string<Str>
{
	static constexpr Fixed_String str = Str;

	static constexpr const std::size_t var_count() noexcept { return count_vars(str); }
	static constexpr const std::size_t arg_count() noexcept { return count_vars(str); }

	static constexpr auto args_data = parse_string<count_vars(Str)>(Str);
	static constexpr auto args = arg_array_to_tuple<args_data.index_array>(args_data.arg_array);
};

template <format_string Format_String, auto... Params>
struct format_string<Format_String, Params...>
{
	// static constexpr auto args_tuple = make_args<Params...>();
	// static constexpr auto args = make_args(Format_String.args, args_tuple);

	// count_vars(Args)
};

void format_test()
{
	constexpr format_string<Fixed_String{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
										 "<label for=${str:id}>${label}</label>"}>
		test;
}
