#pragma once
#include "Fixed_Array.h"
#include <cassert>
#include <ctre.hpp>
#include <variant>

struct formatting_error
{
	constexpr formatting_error(std::string_view _error, std::string_view str) {}
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

	for (auto i = _str.begin(); i < _str.end(); i++)
		if (*i == '$')
			count++;

	return count;
}

struct constant_string : public std::string_view
{
	using base = std::string_view;
	using base::begin;
	std::size_t var_type = 0;

	constexpr constant_string() = default;
	constexpr constant_string(std::string_view str)
		: std::string_view(str), var_type(0)
	{}
	constexpr constant_string(auto var) requires std::invocable<decltype(var.index())> : std::string_view(var), var_type(var.index()) {}
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
	using base = std::string_view;
	using base::begin;
	constexpr str_var() = default;
	constexpr str_var(auto str)
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

using format_arg_variant = std::variant<constant_string, unspecified_var, str_var, int_var, float_var, placeholder>;

struct format_arg : format_arg_variant
{
	using base = format_arg_variant;
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

	inline constexpr bool is_constant() const noexcept { return index() == 0; }

	// inline constexpr void convert_var(const format_arg &arg)
	// {
	// 	emplace<constant_string>(arg);
	// }
	// constexpr auto operator=(const format_arg &arg)
	// {
	// 	if (arg.is_constant())
	// 	{
	// 		base::operator=(std::get<constant_string>(arg));
	// 	}
	// 	else
	// 	{

	// 	}
	// }
};

template <std::size_t N>
struct format_args
{
	static constexpr std::size_t arg_count = N;

	constexpr format_args() = default;

	using arg_array = std::array<format_arg, arg_count>;
	using const_iterator = typename arg_array::const_iterator;
	using iterator = typename arg_array::iterator;

	arg_array args;

	// constexpr auto get_vars() const
	// {
	// 	std::array<std::size_t, var_count> vars;

	// 	auto var_itt = vars.begin();

	// 	for (auto arg_itt = args.begin(); arg_itt < args.end(); arg_itt++)
	// 	{
	// 		if (arg_itt->index() != 0)
	// 		{
	// 			*var_itt = args.end() - arg_itt;
	// 			var_itt++;
	// 		}
	// 	}
	// 	return vars;
	// }

	constexpr std::size_t size() const noexcept { return args.size(); }

	constexpr iterator begin() noexcept { return (args.begin()); }
	constexpr iterator end() noexcept { return (args.end()); }
	constexpr const_iterator begin() const noexcept { return (args.begin()); }
	constexpr const_iterator end() const noexcept { return (args.end()); }
};

static inline constexpr auto count_args(const std::string_view &str)
{
	return count_vars(str) * 2 + 1;
}

template <std::size_t N>
static inline constexpr auto count_args(const format_args<N> &args)
{
	return args.arg_count;
}

static inline constexpr auto count_args(const placeholder &)
{
	return 1;
}

static inline constexpr auto get_args(auto arg_itt, const std::string_view Str)
{
	using iterator = std::string_view::iterator;
	iterator const_start = Str.begin();

	for (auto [match, var] : ctre::range<format_arg::variable_regex.data>(Str))
	{
		arg_itt++->parse_arg(const_start, match.begin());
		const_start = match.end();
		arg_itt++->parse_arg(var);
	}

	if (const_start < Str.end())
		arg_itt->parse_arg(const_start, Str.end());

	arg_itt++;
	return arg_itt;
};

template <std::size_t N>
static inline consteval auto get_args(const std::string_view str)
{
	format_args<N * 2 + 1> fmt_args;
	get_args(fmt_args.args.begin(), str);

	return fmt_args;
};

constexpr std::size_t count_new_args(auto &array)
{
	std::size_t output = 0;
	for (auto val : array)
	{
		output += val - 1;
	}
	return output;
}
constexpr std::size_t sum(auto &array)
{
	std::size_t output = 0;
	for (auto val : array)
	{
		output += val;
	}
	return output;
}

template <auto FirstParam, auto... Params>
static inline constexpr auto get_args_from_parms(auto output_itt)
{
	if constexpr (std::same_as<std::decay_t<decltype(FirstParam)>, placeholder>)
	{
		output_itt->template emplace<placeholder>();
		output_itt++;
	}
	else
		output_itt = get_args(output_itt, FirstParam);

	if constexpr (sizeof...(Params) != 0)
		get_args_from_parms<Params...>(output_itt);
}

template <auto... Args>
struct format_string;

template <format_string String, auto... Params>
static inline constexpr auto get_args()
{
	constexpr std::size_t param_count = sizeof...(Params);

	using param_arg_count_array = std::array<std::size_t, param_count>;
	constexpr param_arg_count_array params_args{count_args(Params)...};

	constexpr size_t new_arg_count = count_new_args(params_args);
	constexpr size_t total_arg_count = String.args.arg_count + new_arg_count;

	using arg_array = std::array<format_arg, sum(params_args)>;
	arg_array new_args;
	get_args_from_parms<Params...>(new_args.begin());

	format_args<total_arg_count> args;

	auto arg_itt = args.begin();

	auto assign = [&arg_itt](auto &itt)
	{
		*arg_itt = itt;
		++arg_itt;
	};

	auto param_arg_itt = new_args.begin();
	auto param_arg_count = params_args.begin();

	for (auto old_arg : String.args)
	{
		if (old_arg.is_constant())
		{
			assign(old_arg);
		}
		else
		{
			if (std::holds_alternative<placeholder>(*param_arg_itt))
			{
				assign(old_arg);
				++param_arg_itt;
			}
			else
			{
				auto end = param_arg_itt + *param_arg_count;
				for (param_arg_itt; param_arg_itt < end; param_arg_itt++)
				{
					assign(*param_arg_itt);
				}
			}
			param_arg_count++;
		}
	}

	return args;
}

template <Fixed_String Str>
struct format_string<Str>
{
	static constexpr Fixed_String str = Str;
	static constexpr format_args args = get_args<count_vars(str)>(Str);

	using args_container_type = decltype(args);

	constexpr std::size_t size() const noexcept { return args.size(); }

	// unfinalized call
	// constexpr auto operator()(auto... Args) const requires(sizeof...(Args) == args.var_count)
	// {}

	constexpr operator args_container_type() const { return args; }
};

template <format_string Format_String, auto... Params>
struct format_string<Format_String, Params...>
{
	static constexpr format_args args = get_args<Format_String, Params...>();

	// count_vars(Args)
};

void format_test()
{
	constexpr format_string<Fixed_String{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
										 "<label for=${str:id}>${label}</label>"}>
		test;
}
