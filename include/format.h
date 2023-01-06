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

template<std::size_t Value>
concept must_not_be_zero = Value != 0;

inline constexpr ignore_t placeholder;

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

template <std::size_t N = 0>
struct constant
{
	Fixed_String<N> str;

	constexpr constant(std::string_view view)
		: str(view)
	{}
	constexpr constant(auto... views)
		: str(views...)
	{}

	constexpr auto operator()(auto &stream) const
	{
		stream << str;
	}
};

template <typename T>
struct is_format_constant : std::false_type
{};

template <std::size_t N>
struct is_format_constant<constant<N>> : std::true_type
{};

template <typename T>
concept format_constant = is_format_constant<T>::value;

struct string;

template <typename FixedStringView>
struct parsed_string
{
	using fixed_view = FixedStringView;

	bool is_var;
	constexpr parsed_string()
		: is_var(true), type(), name(), attribute() {}

	constexpr parsed_string(bool is_var, const fixed_view type, const fixed_view name, const fixed_view attribute)
		: is_var(is_var), type(type), name(name), attribute(attribute)
	{}

	constexpr parsed_string(bool is_var, const fixed_view const_str)
		: is_var(is_var), attribute(const_str)
	{}

	constexpr auto get_constant() const { return attribute; }

	static constexpr parsed_string make_variable(const fixed_view type, const fixed_view name, const fixed_view attribute)
	{
		return parsed_string(true, type, name, attribute);
	}

	static constexpr parsed_string make_variable(const fixed_view name)
	{
		return parsed_string(true, fixed_view(), name, fixed_view());
	}

	static constexpr parsed_string make_constant(const fixed_view const_str)
	{
		return parsed_string(false, const_str);
	}

	fixed_view type;
	fixed_view name;
	fixed_view attribute;
};

template <std::size_t N>
struct unspecified
{
	constexpr unspecified(std::size_t id_hash, std::string_view type, std::string_view fmt_str){};

	constexpr auto operator()(auto &stream, auto string) const
	{
		stream << static_cast<std::string>(string);
	}
};

template <>
struct unspecified<0>
{
	template <typename FixedStringView>
	constexpr unspecified(parsed_string<FixedStringView> str){};

	constexpr auto operator()(auto &stream, auto value) const
	{
		stream << value;
	}
	constexpr auto operator()(auto &stream, std::string_view string) const
	{
		stream << string;
	}
	constexpr auto operator()(auto &stream, float value) const
	{
		stream << value;
	}

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

	template <std::size_t N>
	constexpr auto operator()(Fixed_String<N> str) const
	{
		return constant<N - 1 + 2>('\"', str, '\"');
	}
	
	constexpr auto operator()(auto &stream, const char* string) const
	{
		stream << '\"' << string << '\"';
	}

	constexpr auto operator()(auto &stream, auto string) const
	{
		stream << '\"' << static_cast<std::string>(string) << '\"';
	}
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
	constexpr auto operator()(auto &stream, const int value) const
	{
		stream << std::to_string(value);
	}
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
	
	constexpr auto operator()(auto &stream, const float value) const
	{
		stream << std::to_string(value);
	}
};

template <>
struct variable<bool> :
	variable_base<variable<bool>, "bool">
{
	using base = variable_base<variable<bool>, "bool">;
	using base::base;
	constexpr variable(std::size_t id_hash, std::string_view fmt_str)
		: base(id_hash)
	{}
	constexpr auto operator()(auto &stream, const int value) const
	{
		stream << std::to_string(value);
	}
	constexpr auto operator()(auto &stream, const bool value) const
	{
		stream << std::to_string(value);
	}
};

template <typename... Types>
using tuple_variable_wrapper = std::tuple<variable<Types>...>;

using basic_variables = tuple_variable_wrapper<string, int, float, bool>;

static inline constexpr Fixed_String variable_regex = "\\$\\{(.+?)\\}";
static inline constexpr Fixed_String attribute_regex = "(.+):(.+(:.+)?)";

// TODO Make the assign lambdas skip over empty constants
// template <typename FixedStringView, std::size_t VarCount = count_vars(FixedStringView::full_str())> requires(must_not_be_zero<VarCount>)
// struct parsed_string_array 

template <std::size_t N, const Fixed_String<N>& FixedStringView>
struct parsed_string_array 
{
	static constexpr std::size_t VarCount = count_vars(FixedStringView);
	template <typename T>
	using array = Fixed_Array<T, count_args<VarCount>>;

	// using parsed_string_type = parsed_string<StrPtr>;
	using fixed_view = fixed_string_view<FixedStringView>;
	using parsed_string_type = parsed_string<fixed_view>;
	using args_array = array<parsed_string_type>;

	args_array args;
	std::size_t m_size;

	using value_type = typename args_array::value_type;
	using const_reference = typename args_array::const_reference;

	constexpr std::size_t size() const { return m_size; }

	consteval parsed_string_array()
	{
		const std::string_view str = FixedStringView;

		auto assign_const = [](auto &itt, auto start, auto end)
		{
			fixed_view const_str(start, end - start);
			if (const_str.size() > 0)
			{
				*itt = parsed_string_type::make_constant(const_str);
				++itt;
			}
		};

		auto assign_arg = [&str](auto &itt, auto var_str)
		{
			auto [match, type_match, name_match, attributes_match] = ctre::match<attribute_regex>(var_str);

			if (var_str.end() < str.data() + str.size())
			{
				if (!match)
				{
					*itt = parsed_string_type::make_variable(var_str);
				}
				else
				{
					fixed_view type_view(type_match.to_view());
					fixed_view name_view(name_match.to_view());

					fixed_view attributes_view;
					if (attributes_match)
						attributes_view = fixed_view(attributes_match.to_view());

					*itt = parsed_string_type::make_variable(type_view, name_view, attributes_view);
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

		m_size = output_itt - args.begin();
	}

	constexpr const_reference operator[](std::size_t Index) const { return args[Index]; }
};

template <typename... ArgsT>
struct format_args;

namespace Argument
{

template <std::size_t ArgIndex = 0, std::size_t ParamIndex = 0>
struct container_iterator;

struct to_many_parameters;

template <typename T, std::size_t N = 0, std::size_t... Ns>
struct getter
{
	template <typename ContainerT>
	constexpr inline auto operator()(const ContainerT &container) const
	{
		return std::get<N>(container);
	}

	template <typename ContainerT>
	constexpr inline auto operator()(auto &outputStream, const ContainerT &container) const
	{
		return std::get<N>(container);
	}
};
template <typename... ArgsT, std::size_t N, std::size_t N2>
struct getter<format_args<ArgsT...>, N, N2>
{
	template <typename ContainerT>
	constexpr inline auto operator()(const ContainerT &container) const
	{
		const auto &base_container = std::get<N>(container);

		return std::get<N2>(base_container.args);
	}
	template <typename ContainerT>
	constexpr inline auto operator()(auto &outputStream, const ContainerT &container) const
	{
		const auto &base_container = std::get<N>(container);
		return get<N2>(base_container);
	}
};

template <typename ArgGetter, typename ParamGetter>
struct processor
{
	constexpr void operator()(auto &outputStream, const auto &args, const auto &params) const
	{
		const auto &arg = ArgGetter{}(args);
		const auto &param = ParamGetter{}(params);

		arg(outputStream, param);
	}


	constexpr auto operator()(const auto &args, const auto &params) const
	{
		const auto &arg = ArgGetter{}(args);
		const auto &param = ParamGetter{}(params);

		return arg(param);
	}
};

template <typename ArgGetter>
struct processor<ArgGetter, ignore_t>
{
	constexpr void operator()(auto &outputStream, const auto &args, const auto &) const
	{
		const auto &arg = ArgGetter{}(args);
		arg(outputStream);
	}

	constexpr auto operator()(const auto &args, const auto &) const { return ArgGetter()(args); }
};

template <typename ParamGetter>
struct processor<ignore_t, ParamGetter>
{
	constexpr void operator()(auto &outputStream, const auto &, const auto &params) const
	{
		const auto &param = ParamGetter{}(params);
		param(outputStream);
	}

	constexpr auto operator()(const auto &, const auto &params) const { return ParamGetter()(params); }
};

// get_processor uses type deduction to determine which argument is being processed and whether the parameter should be consumed
// The default case is a processor that uses both argument and the parameter
template <typename Arg, typename Param, std::size_t ArgIndex, std::size_t ParamIndex>
struct get_processor
{
	using use_param = std::bool_constant<!std::is_same_v<ignore_t, Param>>;

	using arg_functor = getter<Arg, ArgIndex>;
	using param_functor = std::conditional_t<use_param::value, getter<Param, ParamIndex>, ignore_t>;

	using processor_type = processor<arg_functor, param_functor>;

	using next_iterator = container_iterator<ArgIndex + 1, ParamIndex + 1>;
};

// The argument is a constant so we just increment the arg Index
template <format_constant Arg, typename Param, std::size_t ArgIndex, std::size_t ParamIndex>
struct get_processor<Arg, Param, ArgIndex, ParamIndex>
{
	using processor_type = processor<getter<Arg, ArgIndex>, ignore_t>;

	using next_iterator = container_iterator<ArgIndex + 1, ParamIndex>;
};

template <typename TupleLikeT, typename IndexSeq, std::size_t N>
struct create_processor_type_sequence;

template <typename... ParamsT, typename IndexT, IndexT... Indicies, std::size_t N>
struct create_processor_type_sequence<format_args<ParamsT...>, std::integer_sequence<IndexT, Indicies...>, N>
{
	using type = type_sequence<processor<ignore_t, getter<format_args<ParamsT...>, N, Indicies>>...>;
};

template <typename Arg, typename... ParamsT, std::size_t ArgIndex, std::size_t ParamIndex>
	requires(!format_constant<Arg>)
struct get_processor<Arg, format_args<ParamsT...>, ArgIndex, ParamIndex>
{
	using indicies = std::make_index_sequence<sizeof...(ParamsT)>;

	using processor_type = typename create_processor_type_sequence<format_args<ParamsT...>, indicies, ParamIndex>::type;
	static_assert(std::tuple_size_v<processor_type> == sizeof...(ParamsT), "Parameters Lost");

	using next_iterator = container_iterator<ArgIndex + 1, ParamIndex + 1>;
};

//
// Start of apply parameters
//
template <typename Arguments, typename Parameters, typename Results, typename ContainerIterator>
struct apply_parameters
{
	using type = Results;
};

template <typename Arguments, typename Parameters, typename Results, std::size_t ArgIndex, std::size_t ParamIndex>
	requires(std::tuple_size_v<Arguments> == ArgIndex) && (std::tuple_size_v<Parameters> > ParamIndex)
struct apply_parameters<Arguments, Parameters, Results, container_iterator<ArgIndex, ParamIndex>>
{
	static_assert((std::tuple_size_v<Arguments> == ArgIndex) && (std::tuple_size_v<Parameters> > ParamIndex), "Too Many Parameters");
};

template <typename Arguments, typename Parameters, typename Results, std::size_t ArgIndex, std::size_t ParamIndex>
	requires(std::tuple_size_v<Arguments> > ArgIndex) && (std::tuple_size_v<Parameters> >= ParamIndex)
struct apply_parameters<Arguments, Parameters, Results, container_iterator<ArgIndex, ParamIndex>>
{
	using argument = std::tuple_element_t<ArgIndex, Arguments>;
	using parameter = std::conditional_t<(std::tuple_size_v<Parameters> == ParamIndex), ignore_t, std::tuple_element<ParamIndex, Parameters>>;

	using result_data = get_processor<argument, typename parameter::type, ArgIndex, ParamIndex>;
	using result = typename result_data::processor_type;

	using results = concat_t<Results, result>;

  public:
	using type = typename apply_parameters<Arguments, Parameters, results, typename result_data::next_iterator>::type;
};

} // namespace Argument

template <typename Arguments, typename... Params>
using apply_argument_parameters =
	typename Argument::apply_parameters<Arguments, std::tuple<Params...>, type_sequence<>, Argument::container_iterator<0, 0>>::type;

template<std::size_t Max, std::size_t Count>
concept less_or_equal_to = Max >= Count;

template <typename... ArgsT>
struct format_args
{
	using arg_type = std::tuple<ArgsT...>;
	static constexpr std::size_t param_count = (((is_format_constant<ArgsT>::value) ? 0 : 1) + ...);

	arg_type args;

	// constructor 1
	constexpr format_args(const ArgsT &...Args)
		: args{Args...}
	{}

	template <typename ParamIterator, typename... Getters>
	constexpr auto assign_vars_impl(const ParamIterator parameters, type_sequence<Getters...>) const
	{
		// TODO simplify the return types here
		//  using return_types = std::tuple<decltype(Getters{}(args, parameters))...>;
		//  using mergedResults = typename Argument::merge_constants<return_types, type_sequence<Getters...>>::type;
		//  return_types().t;

		return ::format_args(Getters{}(args, parameters)...);
	}

	template <typename... ParamT>
	constexpr auto assign_vars(const ParamT... Params) const
	{
		static_assert(sizeof...(ParamT) != 0, "Parameters Must Not Be Empty");
		using result_sequence = apply_argument_parameters<arg_type, ParamT...>;

		return assign_vars_impl(std::tuple{Params...}, result_sequence{});
	}

	template <typename ParamTuple, typename... Getters>
	constexpr auto output_impl(auto &stream, const ParamTuple parameters, type_sequence<Getters...>) const
	{
		(Getters{}(stream, args, parameters), ...);
	}
//TODO add in a concept for stream
	template <typename... ParamT>
	constexpr auto output(auto &stream, const ParamT... Params) const requires (param_count == sizeof...(ParamT))
	// (param_count >= sizeof...(ParamT))
	{
		using result_sequence = apply_argument_parameters<arg_type, ParamT...>;
		return output_impl(stream, std::tuple{Params...}, result_sequence{});
	}
};

template <parsed_string Arg, typename Head, typename... Tail>
static inline constexpr auto parse_variable()
{
	if constexpr (!Arg.is_var)
	{
		constexpr std::size_t constantSize = Arg.get_constant().size();

		return constant<constantSize>(Arg.get_constant());
	}
	else if constexpr (Head::str_match(Arg.type))
	{
		auto str_view = Arg.name;
		return Head(ELFHash(str_view.data(), str_view.size()), Arg.attribute);
	}
	// else if constexpr (Arg.type.empty())
	// {
	// 	auto str_view = make_str_view(Arg.name);
	// 	return unspecified<0>(ELFHash(str_view.data(), str_view.size()));
	// }
	else if constexpr (sizeof...(Tail) > 0)
	{
		return parse_variable<Arg, Tail...>();
	}
	else
	{
		static_assert(Arg.type.size() == 0, "Type was specified, but not found");
		// constexpr std::size_t attributeSize = Arg.Attribute.size;

		return unspecified<0>(Arg);
	}
}

template<parsed_string_array FormatStrings, typename... variables_list, typename IndexT, IndexT... Indicies>
inline constexpr auto make_args(std::tuple<variables_list...>, std::integer_sequence<IndexT, Indicies...>)
{
	return format_args((parse_variable<FormatStrings[Indicies], variables_list...>())...);
}

template<const auto& FormatStr, typename variables_types>
inline constexpr auto make_args()
{
	constexpr parsed_string_array<FormatStr.size(), FormatStr> parsed_format_string;

	return make_args<parsed_format_string>(variables_types{}, std::make_index_sequence<parsed_format_string.size()>{});
}

template <Fixed_String String, typename variable_types = basic_variables>
struct format_string
{
	static constexpr Fixed_String str = String;
	static constexpr auto args = make_args<str, variable_types>();

	constexpr format_string(){}

	constexpr auto operator()(auto&&... params)
	{
		args.output(params...);
	}
};

template <Fixed_String... SubStrs>
using format_multi_string = format_string<make_fixed_string<SubStrs...>()>;

// Temporary
template <Fixed_String Str>
constexpr auto operator"" _fStr()
{
	return Str;
}

template <typename T>
struct formater
{};