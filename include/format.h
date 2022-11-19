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
  private:
	template <typename T>
	using array = std::array<T, count_args<VarCount>>;
	using args_array = array<format_arg>;
	// using index_array = array<std::size_t>;
	using arg_iterator_const = typename args_array::const_iterator;

	args_array args;

  public:
	using value_type = typename args_array::value_type;
	using const_reference = typename args_array::const_reference;

	constexpr std::size_t size() const { return args.size(); }
	// constexpr const args_array& get_args() const { return args; }

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

	template <typename ArrayT>
	constexpr auto iterate_args(auto action_func) const requires(std::invocable<decltype(action_func), typename ArrayT::reference, typename args_array::const_reference>)
	{
		ArrayT indices;

		for (auto arg = args.begin(), index = indices.begin(); arg < args.end(); arg++, index++)
			action_func(*index, *arg);

		return indices;
	}

	constexpr auto get_type_indices() const
	{
		using index_array = array<std::size_t>;
		using index_ref = typename index_array::reference;
		using args_ref = typename args_array::const_reference;

		auto action = [](index_ref index, args_ref arg)
		{ index = arg.index(); };

		return iterate_args<index_array>(action);
	}

	constexpr auto get_var_indices() const
	{
		using index_array = array<std::size_t>;
		using index_ref = typename index_array::reference;
		using args_ref = typename args_array::const_reference;
		std::size_t var_pos = 0;

		auto action = [&var_pos](index_ref index, args_ref arg)
		{
			index = (arg.is_constant()) ? std::size_t(-1) : var_pos++;
		};

		return iterate_args<index_array>(action);
	};

	constexpr const_reference operator[](std::size_t Index) const { return args[Index]; }
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

template <typename... ArgsT>
constexpr inline auto make_format_args(ArgsT... Args);

template <typename T, typename... ArgsT>
concept indexable = requires(T obj, ArgsT... args)
{
	obj.operator[](args...);
};

template <typename T, T... Indicies>
using integer_sequence = std::integer_sequence<T, Indicies...>;

template <typename Tuple1, typename Arg2>
struct concat;

template <typename... ResultT, typename Arg2>
struct concat<std::tuple<ResultT...>, Arg2>
{
	using type = std::tuple<ResultT..., Arg2>;
};

template <typename... ResultT, typename... ArgsT>
struct concat<std::tuple<ResultT...>, std::tuple<ArgsT...>>
{
	using type = std::tuple<ResultT..., ArgsT...>;
};

template <typename Tuple1, typename Arg>
using concat_t = typename concat<Tuple1, Arg>::type;

template <typename T>
struct get_tail;

template <template <typename...> class Container, typename Head, typename... Tail>
struct get_tail<Container<Head, Tail...>>
{
	using type = Container<Tail...>;
};

template <typename T>
using get_tail_t = typename get_tail<T>::type;

template <typename Var, typename Param>
struct apply_var
{
	using type = Param;
};

template <typename Var>
struct apply_var<Var, ignore_t>
{
	using type = Var;
};
template <typename Var, typename Param>
using apply_var_t = typename apply_var<Var, Param>::type;

template <typename ArgTuple, typename ParamTuple, typename result_tuple = std::tuple<>> // requires ArgTuple::var_count < std::tuple_size_v<ParamTuple>
struct arg_assigner;

template <typename HeadArg, typename... TailArgs, typename HeadParam, typename... TailParams, typename Results>
struct arg_assigner<std::tuple<HeadArg, TailArgs...>, std::tuple<HeadParam, TailParams...>, Results>
{
	// Not a permanent solution, ignore_t should not appear in format_args
	template <typename Head, typename... Tail>
	using pop_ignore = std::conditional_t<std::is_same_v<ignore_t, Head>, std::tuple<Tail...>, std::tuple<Head, Tail...>>;

	template <typename Target, typename Head, typename... Tail>
	using pop_if_same = std::conditional_t<std::is_same_v<Target, Head>, std::tuple<Tail...>, pop_ignore<Head, Tail...>>;

	using new_result = std::conditional_t<is_variable_type<HeadArg>, apply_var_t<HeadArg, HeadParam>, HeadArg>;

	using results = concat_t<Results, new_result>;
	using args = pop_if_same<new_result, HeadArg, TailArgs...>;
	using params = pop_if_same<new_result, HeadParam, TailParams...>;

	using type = typename arg_assigner<args, params, results>::type;
};

template <typename ArgTuple, typename Results>
struct arg_assigner<ArgTuple, std::tuple<>, Results>
{
	using type = concat_t<Results, ArgTuple>;
};

// implementation is unnecessary as this is and error case, parameters should not exceed variable count
template <typename ParamTuple, typename result_tuple>
struct arg_assigner<std::tuple<>, ParamTuple, result_tuple>
{
};

template <typename ArgTuple, typename... Params>
using assign_vars_t = typename arg_assigner<ArgTuple, std::tuple<Params...>>::type;

template <typename... ArgsT>
struct format_args
{
	using arg_type = std::tuple<ArgsT...>;
	arg_type args;
	// Keeps track of which types are variables
	using var_offsets = to_integer_sequence_t<get_var_offset<ArgsT...>()>;

	// constructor 1
	constexpr format_args(const ArgsT &...Args)
		: args{Args...}
	{}

	// constructor 2
	template <typename T, T... VariantIndex, T... Index>
	constexpr format_args(const indexable<T> auto ArgArray, integer_sequence<T, VariantIndex...>, integer_sequence<T, Index...>)
		: format_args(std::get<VariantIndex>(ArgArray[Index])...)
	{}

	// constructor 3
	template <typename T, T... VariantIndicies>
	constexpr format_args(const indexable<T> auto ArgArray, integer_sequence<T, VariantIndicies...> integer_seq)
		: format_args(ArgArray, integer_seq, std::make_index_sequence<ArgArray.size()>{})
	{}

	template <typename T, T... VarOffsets, T... Index>
	constexpr auto assign_vars_impl(const auto parameters, integer_sequence<T, VarOffsets...>, integer_sequence<T, Index...>) const
	{
		return make_format_args(process_arg(std::get<Index>(args), get_parameter<VarOffsets>(parameters))...);
	}

	template <typename... ParamT>
	constexpr auto assign_vars(const ParamT... Params) const
	{
		using test = assign_vars_t<arg_type, ParamT...>;
		// test tr;
		// tr.blarg();

		return assign_vars_impl(std::tuple{Params...}, var_offsets{}, std::make_index_sequence<sizeof...(ArgsT)>{});
	}
};

// TODO convert this into a format context
template <typename... ArgsT>
constexpr inline auto make_format_args(ArgsT... Args)
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

	static constexpr args_type args = args_type(arg_array_obj, type_indices());
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