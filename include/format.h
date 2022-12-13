#pragma once
#include "Fixed_Array.h"
#include "hash.h"
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

// Comparator helper
template <Fixed_String Type_Str>
struct type_str
{
	constexpr bool operator()(const std::string_view str) const
	{
		return Type_Str == str;
	}
};

template <std::size_t N = 0>
struct constant
{
	Fixed_String<N> str;

	constexpr constant(std::string_view view)
		: str(view)
	{}
};
struct string;

// template<auto* Ptr>
struct parsed_string
{
	// using fixed_view = fixed_string_view<Ptr>;
	using fixed_view = fixed_string_view;
	bool is_var;
	constexpr parsed_string()
		: is_var(true), type(), name(), attribute() {}

	constexpr parsed_string(const fixed_view type, const fixed_view name, const fixed_view attribute)
		: is_var(true), type(type), name(name), attribute(attribute)
	{}

	constexpr parsed_string(fixed_view const_str)
		: is_var(false), attribute(const_str)
	{}

	constexpr auto get_constant() const { return attribute; }

	fixed_view type;
	fixed_view name;
	fixed_view attribute;
};

template <std::size_t N>
struct unspecified
{
	constexpr unspecified(std::size_t id_hash, std::string_view type, std::string_view fmt_str){};
};

template <>
struct unspecified<0>
{
	// template<auto* Ptr>
	constexpr unspecified(parsed_string str){};
};

template <typename Output, Fixed_String Type_Str = "">
struct variable_base
{
	std::size_t id_hash;
	constexpr variable_base() = default;
	constexpr variable_base(std::size_t id_hash)
		: id_hash(id_hash){};
	using type = Output;

	static constexpr bool str_match(const std::string_view str)
	{
		return Type_Str == str;
	}
};

template <typename Output>
struct variable;

template <>
struct variable<string> :
	variable_base<variable<string>, "str">
{
	using base = variable_base;
	using base::base;

	constexpr variable(std::size_t id_hash, std::string_view fmt_str)
		: base(id_hash)
	{}
};

template <>
struct variable<int> :
	variable_base<variable<int>, "int">
{
	using base = variable_base<variable<int>, "int">;
	using base::base;

	constexpr variable(std::size_t id_hash, std::string_view fmt_str)
		: base(id_hash)
	{}
};

template <>
struct variable<float> :
	variable_base<variable<float>, "float">
{
	using base = variable_base;
	using base::base;

	constexpr variable(std::size_t id_hash, std::string_view fmt_str)
		: base(id_hash)
	{}
};

template <typename... Types>
using tuple_variable_wrapper = std::tuple<variable<Types>...>;

using basic_variables = tuple_variable_wrapper<string, int, float>;

static inline constexpr Fixed_String variable_regex = "\\$\\{(.+?)\\}";
static inline constexpr Fixed_String attribute_regex = "(.+):(.+(:.+)?)";

//TODO Make the assign lambdas skip over empty constants
template <std::size_t VarCount>
struct arg_array
{
	template <typename T>
	using array = Fixed_Array<T, count_args<VarCount>>;

	// using parsed_string_type = parsed_string<StrPtr>;
	using parsed_string_type = parsed_string;
	using args_array = array<parsed_string_type>;

	using arg_iterator_const = typename args_array::const_iterator;

	args_array args;

	using value_type = typename args_array::value_type;
	using const_reference = typename args_array::const_reference;

	constexpr std::size_t size() const { return args.size(); }

	consteval arg_array(const std::string_view str)
	{
		auto make_view = [&str](std::size_t ptr, std::size_t size)
		{
			return fixed_string_view(ptr, size);
		};

		auto assign_const = [&str, make_view](auto &itt, auto start, auto end)
		{
			*itt = parsed_string_type(make_view(start - str.data(), end - start));
			++itt;
		};
		auto assign_arg = [&str, make_view](auto &itt, auto var_str)
		{
			auto [match, type_match, name_match, attributes_match] = ctre::match<attribute_regex>(var_str);
			if (var_str.end() < str.data() + str.size())
			{
				if (!match)
					*itt = parsed_string_type(make_view(var_str.data() - str.data(), var_str.size()));
				else
				{
					fixed_string_view type_view(type_match.data() - str.data(), type_match.size());
					fixed_string_view name_view(name_match.data() - str.data(), name_match.size());

					fixed_string_view attributes_view;
					if (attributes_match)
						attributes_view = fixed_string_view(attributes_match.data() - str.data(), attributes_match.size());

					*itt = parsed_string_type(type_view, name_view, attributes_view);
				}
			}
			++itt;
		};

		auto output_itt = args.begin();
		auto const_start = str.begin();

		for (auto [match, var] : ctre::range<variable_regex>(str))
		{
			assign_const(output_itt, const_start, match.begin());
			assign_arg(output_itt, var.to_view());

			const_start = match.end();
		}

		if (const_start < str.end())
			assign_const(output_itt, const_start, str.end());
	}

	template <typename ArrayT>
	constexpr auto iterate_args(auto action_func) const requires(std::invocable<decltype(action_func), typename ArrayT::reference, typename args_array::const_reference>)
	{
		ArrayT indices;

		for (auto arg = args.begin(), index = indices.begin(); arg < args.end(); arg++, index++)
			action_func(*index, *arg);

		return indices;
	}

	constexpr const_reference operator[](std::size_t Index) const { return args[Index]; }
};

// TODO remove this
template <typename Output, typename InputT>
struct variable_pair
{
	constexpr variable_pair() = default;
	constexpr variable_pair(InputT val)
		: val(val) {}
	InputT val;
};

namespace Argument
{
// Purely a class to clean up error messages
template <typename Container, auto getter_param>
struct container_data_constant;

template <typename Container>
struct container_data
{
	constexpr container_data() = default;
	constexpr container_data(std::size_t index)
		: index(index) {}
	std::size_t index = 0;

	using container_type = Container;
	using getter_param_type = std::size_t;

	constexpr container_data operator+(std::size_t val) const { return container_data(this->index + val); }

	constexpr operator getter_param_type() const { return index; }
};

template <const container_data Value>
using make_container_data_constant = container_data_constant<typename decltype(Value)::container_type, (typename decltype(Value)::getter_param_type)Value>;

template <typename... Args>
struct processor_functor;

// A processor that uses the argument and the parameter
template <typename Arg, typename Param, typename ContainerType, typename getter_param_type, getter_param_type getter_param>
struct processor_functor<Arg, Param, container_data_constant<ContainerType, getter_param>>
{
	using container_type = ContainerType;
	using output_type = variable_pair<Arg, Param>;

	constexpr output_type operator()(const auto &args, const std::same_as<ContainerType> auto &params) const
	{
		return output_type(std::get<getter_param>(params));
	}
};

template <typename Arg, typename ContainerType, typename getter_param_type, getter_param_type getter_param>
struct processor_functor<Arg, container_data_constant<ContainerType, getter_param>>
{
	using container_type = ContainerType;
	using output_type = Arg;

	constexpr output_type operator()(const std::same_as<ContainerType> auto &args, const auto &params) const
	{
		return output_type(std::get<getter_param>(args));
	}
};

// get_processor uses type deduction to determine which argument is being processed and whether the parameter should be consumed
// The default case is a processor that uses both argument and the parameter
template <typename Arg, typename Param, container_data ArgIterator, container_data ParamIterator>
struct get_processor
{
	using processor = processor_functor<Arg, Param, make_container_data_constant<ParamIterator>>;
	static constexpr auto next_argument() { return ArgIterator + 1; }
	static constexpr auto next_parameter() { return ParamIterator + 1; }
};

// The argument is a constant so we just increment the arg iterator
template <std::size_t N, typename Param, container_data ArgIterator, container_data ParamIterator>
requires(!std::is_same_v<ignore_t, Param>) struct get_processor<constant<N>, Param, ArgIterator, ParamIterator>
{
	using processor = processor_functor<constant<N>, make_container_data_constant<ArgIterator>>;
	static constexpr auto next_argument() { return ArgIterator + 1; }
	static constexpr auto next_parameter() { return ParamIterator; }
};

// The parameter is ignored so just use the arg
template <typename Arg, container_data ArgIterator, container_data ParamIterator>
struct get_processor<Arg, ignore_t, ArgIterator, ParamIterator>
{
	using processor = processor_functor<Arg, make_container_data_constant<ArgIterator>>;
	static constexpr auto next_argument() { return ArgIterator + 1; }
	static constexpr auto next_parameter() { return ParamIterator; }
};

// Skips
template <typename Arguments, container_data ArgIterator, typename Param = std::make_index_sequence<std::tuple_size_v<Arguments>>>
struct get_processors_no_params;

template <typename... Arguments, container_data ArgIterator, typename IndexT, IndexT... Indicies>
struct get_processors_no_params<std::tuple<Arguments...>, ArgIterator, std::integer_sequence<IndexT, Indicies...>>
{
	using type = type_sequence<typename get_processor<Arguments, ignore_t, ArgIterator + Indicies, ArgIterator>::processor...>;
};

template <typename Arguments, container_data ArgIterator>
using get_processors_no_params_t = typename get_processors_no_params<Arguments, ArgIterator>::type;

template <typename Arguments, typename Parameters, typename Results, container_data ArgIterator, container_data ParamIterator>
struct apply_parameters
{
	using ArgsContainerType = typename decltype(ArgIterator)::container_type;
	using ParamIteratorType = typename decltype(ParamIterator)::container_type;

	template <typename T>
	static constexpr bool is_parameter = std::is_same_v<ParamIteratorType, typename T::container_type>;

	using head_arg = get_head_t<Arguments>;
	using tail_args = get_tail_t<Arguments>;
	using head_param = get_head_t<Parameters>;
	using tail_params = get_tail_t<Parameters>;

	// ArgIterator= ArgIterator
	using result_data = get_processor<head_arg, head_param, ArgIterator, ParamIterator>;
	using result = typename result_data::processor;

	using results = concat_t<Results, result>;
	using args = tail_args;
	using params = std::conditional_t<is_parameter<result>, tail_params, Parameters>;

  public:
	using type = typename apply_parameters<args, params, results, result_data::next_argument(), result_data::next_parameter()>::type;
};

template <typename Results, container_data ArgIterator, container_data ParamIterator>
struct apply_parameters<std::tuple<>, std::tuple<>, Results, ArgIterator, ParamIterator>
{
	using type = Results;
};

template <typename... Arguments, typename Results, container_data ArgIterator, container_data ParamIterator>
struct apply_parameters<std::tuple<Arguments...>, std::tuple<>, Results, ArgIterator, ParamIterator>
{
	using type = concat_t<Results, typename get_processors_no_params<std::tuple<Arguments...>, ArgIterator>::type>;
};

template <typename Arguments, std::same_as<ignore_t>... Parameters, typename Results, container_data ArgIterator, container_data ParamIterator>
struct apply_parameters<Arguments, std::tuple<Parameters...>, Results, ArgIterator, ParamIterator>
{
	using type = typename apply_parameters<Arguments, std::tuple<>, Results, ArgIterator, ParamIterator>::type;
};

template <typename Arguments, typename... Params>
using apply_parameters_t = typename apply_parameters<Arguments, std::tuple<Params...>, type_sequence<>, container_data<Arguments>{}, container_data<std::tuple<Params...>>{}>::type;
} // namespace Argument

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

	template <typename ParamIterator, typename... Getters>
	constexpr auto assign_vars_impl(const ParamIterator parameters, type_sequence<Getters...>) const
	{
		return ::format_args<typename Getters::output_type...>(Getters{}(args, parameters)...);
	}

	template <typename... ParamT>
	constexpr auto assign_vars(const ParamT... Params) const
	{
		using test = Argument::apply_parameters_t<arg_type, ParamT...>;

		return assign_vars_impl(std::tuple{Params...}, test{});
	}
};

template <typename Array, typename TypeIndexes>
struct format_args_converter;

template <typename Variant, typename IndexT, IndexT... TypeIndexes>
struct format_args_converter<Variant, std::integer_sequence<IndexT, TypeIndexes...>>
{
	using type = format_args<std::variant_alternative_t<TypeIndexes, Variant>...>;
};

// TODO Fix this as it sorta works
template <auto *Str, auto Arg, typename Head, typename... Tail>
static inline constexpr auto parse_variable()
{
	constexpr const char *start = Str->data;
	constexpr auto make_str_view = [start](fixed_string_view view)
	{
		return std::string_view(start + view.offset, view.size);
	};

	if constexpr (!Arg.is_var)
	{
		constexpr std::size_t constantSize = Arg.get_constant().size;

		return constant<constantSize>(make_str_view(Arg.get_constant()));
	}
	else if constexpr (Head::str_match(make_str_view(Arg.type)))
	{
		auto str_view = make_str_view(Arg.name);
		return Head(ELFHash(str_view.data(), str_view.size()), make_str_view(Arg.attribute));
	}
	// else if constexpr (Arg.type.empty())
	// {
	// 	auto str_view = make_str_view(Arg.name);
	// 	return unspecified<0>(ELFHash(str_view.data(), str_view.size()));
	// }
	else if constexpr (sizeof...(Tail) > 0)
	{
		return parse_variable<Str, Arg, Tail...>();
	}
	else
	{
		// constexpr std::size_t attributeSize = Arg.Attribute.size;

		return unspecified<0>(Arg);
	}
}

template <auto *Str, auto ArgViewArray, typename variables_list = basic_variables, typename Indicies = std::make_index_sequence<ArgViewArray.size()>>
struct arg_processor;

template <auto *Str, auto ArgViewArray, typename... variables_list, typename IndexT, IndexT... Indicies>
struct arg_processor<Str, ArgViewArray, std::tuple<variables_list...>, std::integer_sequence<IndexT, Indicies...>>
{
	constexpr auto operator()()
	{
		return format_args((parse_variable<Str, ArgViewArray[Indicies], variables_list...>())...);
	};
};

template <Fixed_String... SubStrs>
struct format_string
{
	static constexpr Fixed_String str{SubStrs...};

	static constexpr auto arg_array_obj = arg_array<count_vars(str)>(str);

	using arg_array_processor = arg_processor<&str, arg_array_obj>;

	static constexpr auto args = arg_array_processor()();
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