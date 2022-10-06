#pragma once
#include "named_pipe.h"
#include <fcntl.h>
#include <functional>
#include <tuple>
#include <tuple.h>
#include <bit>
#include "Fixed_Array.h"
#include <fstream>
#include <iostream>
#include "ServerSocket.h"
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

template <OpCode MyOpCode, Fixed_String Status, class F>
struct Instruction;

template <OpCode MyOpCode, Fixed_String Status,class F, class... Args>
struct Instruction<MyOpCode, Status, F(Args...)>
{
	Instruction(std::function<F(Args...)> func)
		: func(func) {}

	std::function<F(Args...)> func;

	template<std::size_t BufferSize>
	bool operator()(const uint16_t code, auto &Buffer)
	{
		if (MyOpCode.data == code)
		{
			// output.write((std::string_view)Status);
			func();
			return true;
		}
		return false;
	}
};

// struct log_file
// {
// 	log_file(const char *filePath);
// 	log_file(const std::string filePath);
// 	log_file(const std::filesystem::path filePath);

// 	void write(std::string_view str)
// 	{
// 		::write(named_pipe::pipe_fd, str.data(), str.size());
// 	}

// 	void write(std::string str)
// 	{
// 		::write(named_pipe::pipe_fd, str.data(), str.size());
// 	}
// 	// O_WRONLY
// private:
// std::fstream file;
// };
template <std::size_t Size>
struct buffer
{
	char bytes[Size];
	uint16_t pos;
};

template <typename... Instructions>
struct Commander
{
	using InstructionSet = std::tuple<Instructions...>;
	Commander(const char *input_filepath, const char *output_filepath, InstructionSet instructionSet)
		: socket(SOCK_SEQPACKET, 5, input_filepath), instructions(instructionSet)
		
		// in_pipe(input_filepath), out_pipe(output_filepath), 
	{

		// out_pipe.write("ready");
	}

template <std::size_t Size>
	void read_instructions(SocketBuffer<Size> &buffer)
	{
		// if (buffer.getBytesHeld() > 0)
		// {
			// while(!buffer.reachedEOF())
			// {
				uint16_t opCode;
				if(buffer.read(opCode))
				{
					tupleFunctorSwitch(instructions, opCode, buffer);
				}
			// }
			// for (std::size_t i = 0; i < amountRead / 2; i++)
			// {
			// }
		// }
	}

	void wait_for_instructions(){
		auto func = [&](auto buffer)
		{
			read_instructions(buffer);
		};
		//std::bind(&Commander::read_instructions, this, std::placeholders::_1);

		socket.wait_for(func);
	}

	// void setReady()
	// {
	// 	out_pipe.write("ready");
	// }
	void updateStatus()
	{

	}
	void accept()
	{
		// socket.acceptConnection
		// connection_desc
	}

private:
	ServerSocket<AF_UNIX> socket;
	int connection_desc;
	// read_pipe in_pipe;
	// write_pipe out_pipe;
	InstructionSet instructions;
};