#pragma once
#include "Fixed_Array.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
// #include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>
#define PORT 8081

#define SOCKET_ERROR_CHECK(VALUE, ERROR_MESSAGE) \
	if (VALUE == -1)                             \
	{                                            \
		perror(ERROR_MESSAGE);                   \
		exit(EXIT_FAILURE);                      \
	}

#define SOCKET_LOGGER(...) std::cout << __VA_ARGS__ << std::endl;

template <typename AddressT>
struct Socket_Address;

template <>
struct Socket_Address<struct sockaddr_in>
{
	struct sockaddr_in address;
	inline void init(const char *addr)
	{
		address.sin_family = AF_INET;
		address.sin_port = htons(PORT);

		SOCKET_ERROR_CHECK(inet_pton(AF_INET, addr, &address.sin_addr), "Address Convsersion Error");
	}

	inline void init(const in_addr_t addr)
	{
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(PORT);
	}
};

template <>
struct Socket_Address<struct sockaddr_un>
{
	struct sockaddr_un address;

	inline void init(const char *pathname)
	{
		address.sun_family = AF_UNIX;
		strcpy(address.sun_path, pathname);
	}
};

template <typename AddressT, std::array supported_types>
struct Socket_Base
{
	using Socket_Type = AddressT;
	int socket_desc;
	Socket_Address<AddressT> address;

	inline void create_descriptor(const int domain, const int type)
	{
		socket_desc = socket(domain, type, 0);
		SOCKET_ERROR_CHECK(socket_desc, "\n Socket creation error \n");
	}
	template <typename... ArgsT>
	inline void set_address(ArgsT... Args)
	{
		address.init(Args...);
	}
};
// 	int addrlen = sizeof(address);

template <int Domain>
struct Socket;


template <int Domain>
struct ClientSocket : Socket<Domain>
{
	using Base = Socket<Domain>;
	int connection_desc;

	template <typename... ArgsT>
	inline void connect(const int type, ArgsT... Args)
	{
		Base::create_descriptor(Domain, type);
		Base::set_address(Args...);
		connection_desc = ::connect(Base::socket_desc, (struct sockaddr *)&Base::address, sizeof(Base::address));
		if (connection_desc < 0)
		{
			SOCKET_LOGGER("Connection Failed");
		}
		else
		{
			SOCKET_LOGGER("Succsessfull Connection");
		}
	}
	inline void close()
	{
		::close(connection_desc);
	}

	template <typename T>
	void send(T _data)
	{
		::send(connection_desc, (void *)&_data, sizeof(T), 0);
	}

	template <typename T>
	ssize_t read(void *buf, size_t count)
	{
		return ::read(connection_desc, buf, count);
	}
};

template <>
struct Socket<AF_INET> : Socket_Base<struct sockaddr_in, std::array{SOCK_STREAM, SOCK_DGRAM, SOCK_RAW}>
{};

template <>
struct Socket<AF_UNIX> : Socket_Base<struct sockaddr_un, std::array{SOCK_STREAM, SOCK_DGRAM, SOCK_SEQPACKET}>
{};
