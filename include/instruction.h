#pragma once
#include "fixed_array.h"
#include "status.h"
#include "tuple.h"
#include "utility.h"
#include <functional>
#include <sstream>

// TODO implement string_conversion error
template <typename T>
struct from_string
{
	T operator()(std::string_view arg){};
};
template <>
struct from_string<int>
{
	int operator()(std::string_view arg)
	{
		return std::stoi(arg.data());
	};
};

template <class F>
struct instruction
{
	constexpr instruction(F func) {}

	constexpr instruction(const char *command_name, F func) {}
};

template <class F, class... Args>
struct instruction<F(Args...)>
{
	constexpr instruction(auto &&func)
		: func(func) {}

	const std::function<F(Args...)> func;
};

template <class ParentT, class ReturnT, typename... ArgsT>
struct instruction<ReturnT (ParentT::*)(ArgsT...)>
{
	using funcT = ReturnT (ParentT::*)(ArgsT...);

	constexpr instruction(const char *command_name, funcT &&func)
		: name_str(command_name), func(func) {}

	inline bool operator()(status_manager &config, std::vector<std::string_view> &args, ParentT &parent)
	{
		if (args.size() == sizeof...(ArgsT))
		{
			state_object status_output = config.get_status_object(name_str);
			try
			{
				if constexpr (std::is_same_v<void, ReturnT>)
				{
					call_func(parent, args, std::make_index_sequence<sizeof...(ArgsT)>{});
				}
				else
				{
					status_output.append_result(call_func(parent, args, std::make_index_sequence<sizeof...(ArgsT)>{}));
				}
				return true;
			}
			catch (std::exception &e)
			{
				status_output.set_result("Error", e.what());
				return false;
			}
		}

		return false;
	}

	constexpr std::size_t arg_count() const noexcept
	{
		return sizeof...(ArgsT);
	}
	const std::string_view name() const noexcept
	{
		return name_str;
	}

  private:
	const std::function<ReturnT(ParentT &, ArgsT...)> func;
	const std::string_view name_str;

	template <typename IndexT, IndexT... Indices>
	auto convert_args(std::vector<std::string_view> &args, std::integer_sequence<IndexT, Indices...>)
	{
		return std::tuple<ArgsT...>{from_string<ArgsT>(args[Indices])...};
	}

	template <typename IndexT, IndexT... Indices>
	auto call_func(ParentT &parent, std::vector<std::string_view> &args, std::integer_sequence<IndexT, Indices...>)
	{
		return func(parent, (from_string<ArgsT>{}(args[Indices]))...);
	}
};

template <typename... Instructions>
struct instruction_set
{
	instruction_set(Instructions &&...commands)
		: instructions(commands...)
	{
	}

	template <typename... ArgsT>
	bool parse_command(status_manager &config,
					   std::string_view &command,
					   std::vector<std::string_view> &cmdArgs,
					   ArgsT &&...args)
	{
		auto command_comparator = [command](auto &&element)
		{
			return command == element.name();
		};
		auto command_executor = [&config, &cmdArgs, &args...](auto &&element)
		{
			element(config, cmdArgs, args...);
		};

		return tuple_switch(command_comparator, command_executor, instructions);
	}

  private:
	std::tuple<Instructions...> instructions;
};
