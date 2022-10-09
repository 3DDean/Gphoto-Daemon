#pragma once
#include "Fixed_Array.h"
#include "ServerSocket.h"
#include "named_pipe.h"
#include <bit>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <tuple.h>
#include <tuple>
struct OpCode
{
	constexpr OpCode(uint16_t _data)
		: data(_data) {}

	constexpr OpCode(const char (&code)[2])
		: data(std::bit_cast<uint16_t>(code)) {}

	template <std::size_t size>
	constexpr OpCode(const char (&code)[size])
		: OpCode({code[0], code[1]})
	{}
	uint16_t data;
	// bool operator==(const uint16_t target){
	// 	return data == target;
	// }
};

template <Fixed_String Status, class F>
struct Instruction;

template <Fixed_String Status, class F, class... Args>
struct Instruction<Status, F(Args...)>
{
	Instruction(std::function<F(Args...)> func)
		: func(func) {}

	std::function<F(Args...)> func;

	template <std::size_t CMD_ID>
	bool operator()(const uint16_t code, auto &buffer)
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
			// output.write((std::string_view)Status);
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

template <typename SocketType>
struct StatusMessenger
{
	using AddrT = typename SocketType::Address_Type;

	StatusMessenger(int socket)
		: server_socket(socket)
	{
		strcpy(status, "ready");
	}

	// struct Client
	// {
	// 	Client(AddrT addr, size_t length)
	// 		: address(addr), len(length) {}

	// 	AddrT address;
	// 	size_t len;

	// 	bool operator<(const Client &obj2) const
	// 	{
	// 		if(len == obj2.len)
	// 		{
	// 			for(std::size_t i = 0; i < len; i++)
	// 			{
	// 				if(address[i] != obj2.address[i])
	// 				{
	// 					return address[i] < obj2.address[i];
	// 				}
	// 			}
	// 			//The two values are exactly the same
	// 			return false;
	// 		}
	// 		else
	// 		{
	// 			return len < obj2.len;
	// 		}
	// 	}
	// };

	void broadcastStatus()
	{
		std::printf("Broadcastings %i, %s\n", clients.size(), status);

		std::size_t len = strlen(status);


    	auto is_notValid = [&](auto const& var)
		{ 
			int writtenBytes = sendto(server_socket, (void*)status, len, 0, (struct sockaddr *)&var.address, var.len);
			if(writtenBytes == -1)
			{
				std::printf("Removing Connection: %s",var.address.sun_path);
			}
			return writtenBytes == -1;
			
		};
 
    	const auto count = std::erase_if(clients, is_notValid);

		// for(const auto& var : clients)
		// {
		// 	int writtenBytes = sendto(server_socket, (void*)status, len, 0, (struct sockaddr *)&var.address, var.len);
		// 	if(writtenBytes == -1)
		// 	{
		// 		clients.erase(var);
		// 		perror("failed to write bytes\n");
		// 	}

		// }
	}

	template <std::size_t CMD_ID>
	bool operator()(const uint16_t code, auto &buffer)
	{
		std::printf("%i %i\n", CMD_ID, code);
		if (CMD_ID == code)
		{
			std::printf("Adding Client to status\n");
			auto MsgBuffer = buffer.buffer;
			uint8_t state;
			buffer.read(state);

			if (state == 0)
			{
				std::printf("Adding Client %s\n", MsgBuffer.addr.sun_path);
				clients.emplace(MsgBuffer.addr, MsgBuffer.addrlen);
				broadcastStatus();
			}
			else
			{
				clients.erase(AddrT(MsgBuffer.addr, MsgBuffer.addrlen));
				std::printf("Removing Client from status\n");
			}
			// std::flush(std::cout);
			return true;
		}
		return false;
	}

	int server_socket;
	// std::string status;
	char status[128];
	int statusLength;
	std::set<AddrT> clients;
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