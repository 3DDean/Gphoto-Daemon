#pragma once
#include "Fixed_Array.h"
#include "static_string.h"
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
static inline constexpr std::size_t count_args = VarCount * 2 + 1;

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

// static inline constexpr auto count_args(const placeholder &)
// {
// 	return 1;
// }

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

struct format_arg : arg_variant
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

	constexpr format_arg() = default;

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
	inline constexpr format_arg(Itt start, End end)
	{
		std::string_view str(start, end);

		if (ctre::match<variable_regex>(str))
			throw formatting_error("constant_string contains variable ", str);

		emplace<constant_string>(str);
	};

	inline constexpr format_arg(std::string_view var)
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

	inline constexpr bool is_constant() const noexcept { return index_of<constant_string> == index(); }
};

static inline constexpr Fixed_String variable_regex = "\\$\\{(.+?)\\}";
static inline constexpr Fixed_String attribute_regex = "(.+):(.+)";

template <std::size_t Size>
struct format_args_helper
{
	constexpr format_args_helper() = default;
	std::array<format_arg, Size> arg_array;
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
	constexpr std::size_t array_size = count_args<VarCount>;
	std::array<format_arg, array_size> arg_array;

	auto output_itt = arg_array.begin();
	auto assign_arg = [](auto &itt, auto... args)
	{
		*itt = format_arg(args...);
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

	return arg_array;
}

struct arg_metadata
{
	std::size_t type_index;
	std::int32_t var_index;
};

template <std::size_t Size>
static inline constexpr auto get_type_index(const std::array<format_arg, Size> &args)
{
	std::array<std::size_t, Size> type_indexes;
	std::int32_t var_pos = 0;
	auto index_itt = type_indexes.begin();
	for (auto arg : args)
	{
		*index_itt = arg.index();
		index_itt++;
	}
	return type_indexes;
};

static constexpr std::size_t constant_string_index = std::size_t(-1);

template <std::size_t Size>
static inline constexpr auto get_var_index(const std::array<format_arg, Size> &args)
{
	std::array<std::size_t, Size> var_indexes;
	std::size_t var_pos = 0;
	auto index_itt = var_indexes.begin();
	for (auto arg : args)
	{
		if (arg.is_constant())
		{
			*index_itt = std::size_t(-1);
		}
		else
		{
			*index_itt = var_pos;
			var_pos++;
		}
		index_itt++;
	}
	return var_indexes;
};

// template<auto Array, typename T, >
// using var_sequence = type<T>;

template <auto Index_Array, typename T, T... index>
constexpr auto arg_array_to_tuple_w_index(auto container, std::integer_sequence<T, index...>)
{
	return std::make_tuple(std::get<Index_Array[index]>(container[index])...);
}

template <auto Index_Array>
static inline constexpr auto make_args(auto ArgArray)
{
	return arg_array_to_tuple_w_index<Index_Array>(ArgArray, std::make_index_sequence<ArgArray.size()>());
};

template <static_string_type Str>
static inline constexpr auto parse_param()
{
	constexpr auto var_count = count_vars(Str);

	if constexpr (var_count == 0)
	{
		return std::string_view{Str};
	}
	else
	{
		constexpr auto arg_array = parse_string<var_count>(Str);
		constexpr auto args_type_indexes = get_type_index(arg_array);

		return make_args<args_type_indexes>(arg_array);
	}
};

template <Fixed_String Str>
static inline constexpr auto parse_param()
{
	return parse_param<static_string<Str>>();
};

template <placeholder>
static inline constexpr auto parse_param()
{
	return placeholder();
};

template <auto... Params>
requires(sizeof...(Params) > 1) static inline constexpr auto parse_params()
{
	return std::tuple{parse_param<Params>()...};
}

template <auto base_pos, std::size_t var_index>
constexpr auto merge_arg(auto &&base, auto &&parameters)
{
	auto return_base = [&base]()
	{
		auto arg = std::get<base_pos>(base);

		return std::make_tuple(arg);
	};

	if constexpr (var_index == constant_string_index)
	{
		return return_base();
	}
	else
	{
		auto arg = std::get<var_index>(parameters);
		using ArgT = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<ArgT, placeholder>)
		{
			return return_base();
		}
		else if constexpr (std::is_same_v<ArgT, std::string_view>)
		{
			using VarT = std::decay_t<decltype(std::get<base_pos>(base))>;
			return std::make_tuple(constant_string(arg, format_arg::index_of<VarT>));
		}
		else
		{
			return arg;
		}
	}
}

template <auto Index_Array, typename T, T... index>
constexpr auto merge_args(auto &&container, auto &&parameters, std::integer_sequence<T, index...>)
{
	return std::tuple_cat(merge_arg<index, Index_Array[index]>(container, parameters)...);
}

template <auto Index_tuple>
static inline constexpr auto merge_args(auto &&base_tuple, auto &&parameters)
{
	return merge_args<Index_tuple>(base_tuple, parameters, std::make_index_sequence<Index_tuple.size()>());
};

template <Fixed_String Str>
struct format_string<Str>
{
	static constexpr Fixed_String str = Str;
	static constexpr const std::size_t var_count() noexcept { return count_vars(str); }
	static constexpr const std::size_t arg_count() noexcept { return count_args<var_count()>; }

	static constexpr auto arg_array = parse_string<var_count()>(Str);
	static constexpr auto type_indexes = get_type_index(arg_array);
	static constexpr auto var_indexes = get_var_index(arg_array);

	static constexpr auto args = make_args<type_indexes>(arg_array);
};

template <format_string Format_String, auto... Params>
struct format_string<Format_String, Params...>
{
	static constexpr auto args_tuple = parse_params<Params...>();
	static constexpr auto args = merge_args<Format_String.var_indexes>(Format_String.args, args_tuple);

	// count_vars(Args)
};

void format_test()
{
	constexpr format_string<Fixed_String{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
										 "<label for=${str:id}>${label}</label>"}>
		test;
}
