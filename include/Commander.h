#pragma once
#include "named_pipe.h"
#include <fcntl.h>
#include <functional>
#include <tuple>
#include <tuple.h>
#include <bit>
struct OpCode
{
	constexpr OpCode(uint16_t _data)
		: data(_data) {}

	constexpr OpCode(const char (&code)[2]): data(std::bit_cast<uint16_t>(code)){}

	template<std::size_t size>
	constexpr OpCode(const char (&code)[size]) : OpCode({code[0], code[1]})
	{	}
	uint16_t data;
	// bool operator==(const uint16_t target){
	// 	return data == target;
	// }
};
template <std::size_t size>
constexpr OpCode make_opCode(const char (&code)[size])
{
	uint16_t expectedOutput = 0;
	char *byte = (char *)&expectedOutput;
	byte[0] = 'A';
	byte[1] = 'B';

	return OpCode(expectedOutput);
}
template <OpCode MyOpCode, class F>
struct Instruction;

template <OpCode MyOpCode, class F, class... Args>
struct Instruction<MyOpCode, F(Args...)>
{
	Instruction(std::function<F(Args...)> func)
		: func(func) {}

	std::function<F(Args...)> func;

	template<std::size_t BufferSize>
	bool operator()(const uint16_t code, auto &Buffer)
	{
		if (MyOpCode.data == code)
		{
			func();
			return true;
		}
		return false;
	}
};

template <typename... Instructions>
struct Commander
{
	using InstructionSet = std::tuple<Instructions...>;
	Commander(const char *filepath, InstructionSet instructionSet)
		: in_pipe(filepath, O_RDONLY | O_NONBLOCK), instructions(instructionSet)
	{}

	void read_instructions()
	{
		Buffer<512>& buffer = in_pipe.read();
		if (buffer.getBytesHeld() > 0)
		{
			while(buffer.isGood())
			{
				uint16_t opCode;
				if(buffer.getParameters(opCode))
				{
					tupleFunctorSwitch(instructions,opCode, buffer);
				}
			}
			// for (std::size_t i = 0; i < amountRead / 2; i++)
			// {
			// }
		}
	}

private:
	named_pipe in_pipe;
	InstructionSet instructions;
};