#pragma once
#include "Fixed_Array.h"
#include <tuple.h>
#include "status.h"


template <Fixed_String Status, class F>
struct Instruction;

template <Fixed_String Status, class F, class... Args>
struct Instruction<Status, F(Args...)>
{
	Instruction(std::function<F(Args...)> func)
		: func(func) {}

	std::function<F(Args...)> func;

	template <std::size_t CMD_ID>
	bool operator()(const uint16_t code, StatusMessenger& status, auto &buffer)
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


// template <typename... Instructions>
// struct Commander
// {
// 	using InstructionSet = std::tuple<StatusListener, Instructions...>;
// 	Commander(const char *input_filepath, const char *output_filepath, Instructions... _instructions)
// 		: socket(SOCK_DGRAM, 5, input_filepath), instructions(std::make_tuple(StatusListener(socket.socket_desc), _instructions...))

// 	// in_pipe(input_filepath), out_pipe(output_filepath),
// 	{

// 		// out_pipe.write("ready");
// 	}

// 	template <std::size_t Size>
// 	void read_instructions(SocketBuffer<Size> &buffer)
// 	{
// 		// if (buffer.getBytesHeld() > 0)
// 		// {
// 		// while(!buffer.reachedEOF())
// 		// {
// 		uint16_t opCode;
// 		if (buffer.read(opCode))
// 		{
// 			tupleFunctorSwitch(instructions, opCode, buffer);
// 		}
// 		// }
// 		// for (std::size_t i = 0; i < amountRead / 2; i++)
// 		// {
// 		// }
// 		// }
// 	}

// 	void wait_for_instructions()
// 	{
// 		auto func = [&](auto buffer)
// 		{
// 			read_instructions(buffer);
// 		};
// 		//std::bind(&Commander::read_instructions, this, std::placeholders::_1);

// 		socket.wait_for(func);
// 	}

// 	// void setReady()
// 	// {
// 	// 	out_pipe.write("ready");
// 	// }
// 	void updateStatus()
// 	{
// 	}
// 	void accept()
// 	{
// 		// socket.acceptConnection
// 		// connection_desc
// 	}

//   private:
// 	ServerSocket<AF_UNIX> socket;
// 	int connection_desc;
// 	// read_pipe in_pipe;
// 	// write_pipe out_pipe;
// 	InstructionSet instructions;
// };