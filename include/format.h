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
struct fmt_data
{
	std::size_t total_length;
};
struct constant_string : public std::string_view
{
	using base = std::string_view;
	using base::begin;
	std::size_t var_type = 0;

	constexpr constant_string() = default;
	constexpr constant_string(std::string_view str, std::size_t var_type = 0)
		: std::string_view(str), var_type(var_type)
	{}
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

template <typename Output, typename InputT>
struct variable
{
	constexpr variable() = default;
	constexpr variable(InputT val)
		: val(val) {}
	InputT val;
};

static inline constexpr Fixed_String variable_regex = "\\$\\{(.+?)\\}";
static inline constexpr Fixed_String attribute_regex = "(.+):(.+)";

using variable_types = std::tuple<str_var, int_var, float_var, unspecified_var>;


// constant_string->fixed_string<>
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

// Purely a class to clean up error messages
template <typename Container, auto getter_param>
struct getter_data_constant;

template <typename Container>
struct getter_data
{
	constexpr getter_data() = default;
	constexpr getter_data(std::size_t index)
		: index(index) {}
	std::size_t index = 0;

	using container_type = Container;
	using getter_param_type = std::size_t;

	constexpr getter_data operator+(std::size_t val) const { return getter_data(this->index + val); }

	constexpr operator getter_param_type() const { return index; }
};

template <const getter_data Value>
using make_getter_data_constant = getter_data_constant<typename decltype(Value)::container_type, (typename decltype(Value)::getter_param_type)Value>;

template <typename... Args>
struct getter;

template <typename Arg, typename Param, typename ContainerType, typename getter_param_type, getter_param_type getter_param>
struct getter<Arg, Param, getter_data_constant<ContainerType, getter_param>>
{
	using container_type = ContainerType;
	using output_type = variable<Arg, Param>;

	constexpr output_type operator()(const auto &args, const std::same_as<ContainerType> auto &params) const
	{
		return output_type(std::get<getter_param>(params));
	}
};

template <typename Arg, typename ContainerType, typename getter_param_type, getter_param_type getter_param>
struct getter<Arg, getter_data_constant<ContainerType, getter_param>>
{
	using container_type = ContainerType;
	using output_type = Arg;

	constexpr output_type operator()(const std::same_as<ContainerType> auto &args, const auto &params) const
	{
		return output_type(std::get<getter_param>(args));
	}
};

template <getter_data NextArgGetter, getter_data NextParamGetter>
struct next_getter_data
{
	static constexpr inline getter_data next_arg_index = NextArgGetter;
	static constexpr inline getter_data next_param_index = NextParamGetter;
};

template <typename... Args>
struct make_getter_base
{
	using type = getter<Args...>;
};

template <typename Arg, typename Param, getter_data ArgGetter, getter_data ParamGetter>
struct make_getter :
	next_getter_data<ArgGetter + 1, ParamGetter + 1>,
	make_getter_base<Arg, Param, make_getter_data_constant<ParamGetter>>
{};

template <typename Param, getter_data ArgGetter, getter_data ParamGetter>
struct make_getter<constant_string, Param, ArgGetter, ParamGetter> :
	next_getter_data<ArgGetter + 1, ParamGetter>,
	make_getter_base<constant_string, make_getter_data_constant<ArgGetter>>
{};

template <typename Arg, getter_data ArgGetter, getter_data ParamGetter>
requires(!std::same_as<Arg, constant_string>) struct make_getter<Arg, ignore_t, ArgGetter, ParamGetter> :
	next_getter_data<ArgGetter + 1, ParamGetter>,
	make_getter_base<Arg, make_getter_data_constant<ArgGetter>>
{};

template <typename Arguments, getter_data ArgGetter, typename Param = std::make_index_sequence<std::tuple_size_v<Arguments>>>
struct make_getters;

template <typename... Arguments, getter_data ArgGetter, typename IndexT, IndexT... Indicies>
struct make_getters<std::tuple<Arguments...>, ArgGetter, std::integer_sequence<IndexT, Indicies...>>
{
	using type = type_sequence<typename make_getter<Arguments, ignore_t, ArgGetter + Indicies, ArgGetter>::type...>;
};

template <typename Arguments, getter_data ArgGetter>
using make_getters_t = typename make_getters<Arguments, ArgGetter>::type;

template <typename Arguments, typename Parameters, typename Results, getter_data ArgGetter, getter_data ParamGetter>
struct apply_parameters
{
	using ArgsContainer = typename decltype(ArgGetter)::container_type;
	using ParamContainer = typename decltype(ParamGetter)::container_type;

	using head_arg = get_head_t<Arguments>;
	using tail_args = get_tail_t<Arguments>;
	using head_param = get_head_t<Parameters>;
	using tail_params = get_tail_t<Parameters>;

	using new_result = make_getter<head_arg, head_param, ArgGetter, ParamGetter>;
	using result_type = typename new_result::type;

	using results = concat_t<Results, result_type>;
	using args = tail_args;
	using params = std::conditional_t<std::is_same_v<ParamContainer, typename result_type::container_type>, tail_params, Parameters>;

  public:
	using type = typename apply_parameters<args, params, results, new_result::next_arg_index, new_result::next_param_index>::type;
};

template <typename Results, getter_data ArgGetter, getter_data ParamGetter>
struct apply_parameters<std::tuple<>, std::tuple<>, Results, ArgGetter, ParamGetter>
{
	using type = Results;
};

template <typename... Arguments, typename Results, getter_data ArgGetter, getter_data ParamGetter>
struct apply_parameters<std::tuple<Arguments...>, std::tuple<>, Results, ArgGetter, ParamGetter>
{
	using type = concat_t<Results, make_getters_t<std::tuple<Arguments...>, ArgGetter>>;
};

template <typename Arguments, std::same_as<ignore_t>... Parameters, typename Results, getter_data ArgGetter, getter_data ParamGetter>
struct apply_parameters<Arguments, std::tuple<Parameters...>, Results, ArgGetter, ParamGetter>
{
	using type = typename apply_parameters<Arguments, std::tuple<>, Results, ArgGetter, ParamGetter>::type;
};

template <typename Arguments, typename... Params>
using apply_parameters_t = typename apply_parameters<Arguments, std::tuple<Params...>, type_sequence<>, getter_data<Arguments>{}, getter_data<std::tuple<Params...>>{}>::type;

template <typename... ArgsT>
struct format_args
{
	using arg_type = std::tuple<ArgsT...>;
	arg_type args;

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

	template <typename ParamContainer, typename... Getters>
	constexpr auto assign_vars_impl(const ParamContainer parameters, type_sequence<Getters...>) const
	{
		return make_format_args(Getters{}(args, parameters)...);
	}

	template <typename... ParamT>
	constexpr auto assign_vars(const ParamT... Params) const
	{
		using test = apply_parameters_t<arg_type, ParamT...>;

		return assign_vars_impl(std::tuple{Params...}, test{});
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