#pragma once
#include "Fixed_Array.h"
#include "status.h"
#include <tuple.h>

template <Fixed_String Status, class F>
struct Instruction;

template <Fixed_String Status, class F, class... Args>
struct Instruction<Status, F(Args...)>
{
	Instruction(std::function<F(Args...)> func)
		: func(func) {}

	std::function<F(Args...)> func;

	template <std::size_t CMD_ID>
	bool operator()(const uint16_t code, StatusMessenger &status, auto &buffer)
	{
		if (CMD_ID == code)
		{
			if constexpr (sizeof...(Args) != 0)
			{
				std::tuple<Args...> args;
				buffer.read(args);
				call_func(args, std::make_index_sequence<sizeof...(Args)>());
			}
			else
			{
				func();
			}
			return true;
		}
		return false;
	}

	template <typename T, T... index>
	inline void call_func(std::tuple<Args...> &tuple, std::integer_sequence<T, index...> int_seq)
	{
		func(std::get<index>(tuple)...);
	}
};
