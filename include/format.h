#pragma once
#include "Fixed_Array.h"
#include "static_string.h"
#include "tuple.h"
#include "utility.h"
#include <cassert>
#include <ctre.hpp>
#include <variant>
#include <vector>

inline constexpr ignore_t placeholder;

struct formatting_error
{
	constexpr formatting_error() {}
	constexpr formatting_error(std::string_view _error, std::string_view str) {}
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

static inline constexpr Fixed_String variable_regex = "\\$\\{(.+?)\\}";
static inline constexpr Fixed_String attribute_regex = "(.+):(.+)";

using variable_types = std::tuple<str_var, int_var, float_var, unspecified_var>;

using arg_variant = std::variant<constant_string, str_var, int_var, float_var, unspecified_var>;

template <typename Type>
static inline constexpr auto arg_type_index = index_of<Type, arg_variant>::value;

template <typename Type>
static inline constexpr bool is_variable_type = contains<Type, variable_types>::value;

struct format_arg : arg_variant
{
	using base = arg_variant;

	template <typename Type>
	static inline constexpr auto type_index = index_of_v<Type, base>;

	template <typename Type>
	static inline constexpr auto string_index_pair = index_string_view<type_index<Type>>{Type::match_str};

	template <typename... Types>
	static inline constexpr auto string_index_tuple = std::make_tuple(string_index_pair<Types>...);

	static inline constexpr auto indexed_tuple = string_index_tuple<str_var, int_var, float_var>;

	using base::emplace;

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
		auto [whole, firstCap, secondCap] = ctre::match<attribute_regex>(var);
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

	inline constexpr bool is_constant() const noexcept { return type_index<constant_string> == index(); }
};

template <std::size_t VarCount>
struct arg_array
{
	template <typename T>
	using array = std::array<T, count_args<VarCount>>;

	array<format_arg> args;

	constexpr std::size_t size() { return args.size(); }

	consteval arg_array(const std::string_view str)
	{
		auto assign_arg = [](auto &itt, auto... args)
		{
			*itt = format_arg(args...);
			++itt;
		};

		auto output_itt = args.begin();
		auto const_start = str.begin();

		for (auto [match, var] : ctre::range<variable_regex>(str))
		{
			assign_arg(output_itt, const_start, match.begin());
			assign_arg(output_itt, var.to_view());

			const_start = match.end();
		}

		if (const_start < str.end())
			assign_arg(output_itt, const_start, str.end());
	}

	constexpr auto get_type_indices() const
	{
		array<std::size_t> indices;

		for (auto arg = args.begin(), index = indices.begin(); arg < args.end(); arg++, index++)
			*index = arg->index();

		return indices;
	}

	constexpr auto get_var_indices() const
	{
		array<std::size_t> indices;

		std::size_t var_pos = 0;

		for (auto arg = args.begin(), index = indices.begin(); arg < args.end(); arg++, index++)
			if (arg->is_constant())
				*index = std::size_t(-1);
			else
				*index = var_pos++;

		return indices;
	};
};

static inline constexpr std::size_t not_var = std::size_t(-1);

template <typename... ArgsT>
static inline constexpr auto get_var_offset()
{
	std::array args{is_variable_type<ArgsT>...};
	std::array<std::size_t, sizeof...(ArgsT)> indices;
	std::size_t var_count = 0;

	for (auto is_var = args.begin(), index = indices.begin(); is_var < args.end(); is_var++, index++)
		*index = (*is_var) ? var_count++ : not_var;

	return indices;
};

template <std::size_t N>
static inline constexpr auto process_arg(str_var var, const Fixed_String<N> &param)
{
	return std::pair{var, param};
};

template <typename T>
static inline constexpr auto process_arg(T arg, ignore_t)
{
	return arg;
};

template <typename T>
requires(!std::same_as<T, ignore_t>) static inline constexpr auto process_arg(str_var, T param)
{
	static_assert(std::is_same_v<T, int>);
	// throw std::exception("Next target");
	return 1;
};

template <typename T, typename T2>
static inline constexpr auto process_arg(T arg, T2 param)
{
	// static_assert(std::is_same_v<T2, int>);
	// throw std::exception("Next target");
	return 1;
};

template <std::size_t I, typename TupleT>
static inline constexpr auto get_parameter(TupleT &parameters)
{
	if constexpr (I != not_var || std::tuple_size_v < TupleT >> I)
		return std::get<I>(parameters);
	else
		return ignore;
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
		// Will not work
		constexpr auto arg_array = parse_string<var_count>(Str);
		constexpr auto args_type_indexes = get_type_index(arg_array);

		return make_args<args_type_indexes>(arg_array);
	}
};

template <std::size_t N>
static inline constexpr auto parse_param(Fixed_String<N> Str)
{
	return parse_param<static_string<Str>>();
};

template <ignore_t>
static inline constexpr auto parse_param()
{
	return ignore;
};

template <auto... Params>
requires(sizeof...(Params) > 1) static inline constexpr auto parse_params()
{
	return std::tuple{parse_param<Params>()...};
}

template <typename... ArgsT>
static constexpr inline auto make_format_args(ArgsT... Args);

template <typename... ArgsT>
struct format_args
{
	std::tuple<ArgsT...> args;
	// Keeps track of which types are variables
	using var_offsets = to_integer_sequence_t<get_var_offset<ArgsT...>()>;

	constexpr format_args(ArgsT &...Args)
		: args{Args...}
	{}

	template <typename V, std::size_t N, typename T, T... TypeIndexes, T... index>
	constexpr format_args(std::array<V, N> ArgArray, std::integer_sequence<T, TypeIndexes...>, std::integer_sequence<T, index...>)
		: args{std::get<TypeIndexes>(ArgArray[index])...}
	{}

	template <typename T, T... TypeIndexes>
	constexpr format_args(auto ArgArray, std::integer_sequence<T, TypeIndexes...> integer_seq)
		: format_args(ArgArray, integer_seq, std::make_index_sequence<ArgArray.size()>())
	{}

	template <typename T, T... VarOffsets, T... index>
	constexpr auto assign_vars_impl(const auto parameters, std::integer_sequence<T, VarOffsets...>, std::integer_sequence<T, index...>) const
	{
		return make_format_args(process_arg(std::get<index>(args), get_parameter<VarOffsets>(parameters))...);
	}

	constexpr auto assign_vars(const auto... Params) const
	{
		return assign_vars_impl(std::tuple{Params...}, var_offsets(), std::make_index_sequence<sizeof...(ArgsT)>{});
	}
};

// TODO convert this into a format context
template <typename... ArgsT>
static constexpr inline auto make_format_args(ArgsT... Args)
{
	return format_args<ArgsT...>(Args...);
}

template <typename Array, typename TypeIndexes>
struct format_args_converter;

template <typename Variant, typename IndexT, IndexT... TypeIndexes>
struct format_args_converter<Variant, std::integer_sequence<IndexT, TypeIndexes...>>
{
	using type = format_args<std::variant_alternative_t<TypeIndexes, Variant>...>;
};

template <typename TypeIndexes>
using default_format_args = typename format_args_converter<format_arg::base, TypeIndexes>::type;

template <Fixed_String... SubStrs>
struct format_string
{
	static constexpr Fixed_String str{SubStrs...};

	// This cannot be moved to a template parameter because of the pointer in std::string_view
	static constexpr auto arg_array_obj = arg_array<count_vars(str)>(str);

	using type_indices = to_integer_sequence_t<arg_array_obj.get_type_indices()>;
	using var_indices = to_integer_sequence_t<arg_array_obj.get_var_indices()>;

	using args_type = default_format_args<type_indices>;

	static constexpr args_type args = args_type(arg_array_obj.args, type_indices());
};

// Temporary
template <Fixed_String Str>
constexpr auto operator"" _fStr()
{
	return Str;
}

template <typename T>
struct formater
{};