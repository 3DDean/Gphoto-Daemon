#pragma once
#include "fixed_array.h"
#include "status.h"
#include "utility.h"
#include <functional>
#include <sstream>
#include <tuple.h>

// TODO implement string_conversion error
template <typename T>
struct from_string
{
	T operator()(std::string_view arg)
	{};
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
		: name(command_name), func(func) {}

	bool operator()(ParentT &parent, std::vector<std::string_view> &args)
	{
		if (args.size() == sizeof...(ArgsT))
		{
			try
			{
				if constexpr (std::is_same_v<void, ReturnT>)
				{
					call_func(parent, args, std::make_index_sequence<sizeof...(ArgsT)>{});
					std::cout << "Command finished\n";
				}
				else
				{
					std::cout << call_func(parent, args, std::make_index_sequence<sizeof...(ArgsT)>{}) << "\n";
				}
				return true;
			}
			catch (std::exception &e)
			{
				return false;
			}
		}
		return false;
	}

  private:
	const std::function<ReturnT(ParentT &, ArgsT...)> func;
	const std::string_view name;

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
