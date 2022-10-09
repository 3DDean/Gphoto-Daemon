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
	using Type = Socket_Address<struct sockaddr_un>;
	using Socket_Type = struct sockaddr_un;
	static constexpr size_t maxPathLen = sizeof(Socket_Type::sun_path);
	static constexpr size_t familySize = sizeof(Socket_Type::sun_family);

	struct sockaddr_un address;
	size_t len = sizeof(Socket_Type);

	Socket_Address(){}

	Socket_Address(struct sockaddr_un address, size_t len) : address(address), len(len)
	{}

	Socket_Address(Type& address) : address(address.address), len(address.len)
	{}

	Socket_Address(const char *pathname)
	{
		address.sun_family = AF_LOCAL;
		strcpy(address.sun_path, pathname);
	}

	size_t pathSize() const
	{
		return len - familySize;
	}


	bool operator<(const Type &obj2) const
	{
		for (std::size_t i = 0; i < pathSize(); i++)
		{
			if (address.sun_path[i] != obj2.address.sun_path[i])
			{
				return address.sun_path[i] < obj2.address.sun_path[i];
			}
		}
		return false;
		//The two values are exactly the same
	}
};

template <typename AddressT, std::array supported_types>
struct Socket_Base
{
	using Address_Type = Socket_Address<AddressT>;
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
		address = Address_Type(Args...);
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

// template <int Domain>
// struct Server
// {
// 	Socket<Domain> socket;
// };

// template <int Domain>
// struct Client
// {
// 	Socket<Domain> socket;
// };

// struct SocketConnection
// {
// 	int socket_desc;
// 	struct sockaddr_un address;
// 	int connection_desc;
// 	const size_t bufferLen = 1024;
// 	char buffer[1024] = {0};

// 	SocketConnection(const char *pathname)
// 	{
// 		SOCKET_LOGGER("Creating Outbound Socket");

// 		socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
// 		SOCKET_ERROR_CHECK(socket_desc, "\n Socket creation error \n");

// 		address.sun_family = AF_UNIX;
// 		strcpy(address.sun_path, pathname);

// 		SOCKET_LOGGER("Attempting Connection");

// 		connection_desc = connect(socket_desc, (struct sockaddr *)&address, sizeof(address));
// 		if (connection_desc < 0)
// 		{
// 			SOCKET_LOGGER("Connection Failed");
// 		}
// 		else
// 		{
// 			SOCKET_LOGGER("Succsessfull Connection");
// 		}

// 		// SOCKET_ERROR_CHECK(connection_desc, "\nConnection Failed \n");
// 	}

// 	~SocketConnection()
// 	{
// 		close(connection_desc);
// 	}
// 	template <typename T>
// 	void sendData(T _data)
// 	{
// 		send(socket_desc, (void *)&_data, sizeof(T), 0);
// 	}
// };

// struct Server
// {
// 	int socket_desc;
// 	int connection_desc;
// 	struct sockaddr_un address;
// 	int opt = 1;
// 	int addrlen = sizeof(address);

// 	Server(const char *pathname)
// 	{
// 		// SOCKET_LOGGER("Creating Listening Socket");
// 		// socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
// 		// SOCKET_ERROR_CHECK(socket_desc, "Failed to create socket");

// 		// // Forcefully attaching socket to the port 8080
// 		// SOCKET_ERROR_CHECK(setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)),
// 		// 				   "Failed to set socket option")

// 		// address.sun_family = AF_UNIX;
// 		// strcpy(address.sun_path, pathname);

// 		SOCKET_ERROR_CHECK(bind(socket_desc, (struct sockaddr *)&address, sizeof(address)), "Failed to Bind socket");
// 		SOCKET_ERROR_CHECK(listen(socket_desc, 5), "Failed to listen");
// 		SOCKET_LOGGER("Creating Listening");

// 		connection_desc = accept(socket_desc, (struct sockaddr *)&address, (socklen_t *)&addrlen);
// 		SOCKET_ERROR_CHECK(connection_desc, "Failed to accept");
// 	}
// 	~Server()
// 	{
// 		// closing the connected socket
// 		close(connection_desc);
// 		// closing the listening socket
// 		shutdown(socket_desc, SHUT_RDWR);
// 	}
// 	ssize_t readSock(void *buf, size_t count)
// 	{
// 		return read(connection_desc, buf, count);
// 	}
// };