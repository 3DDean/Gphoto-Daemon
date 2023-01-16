#pragma once
#include "Buffer.h"
#include "Socket.h"
#include <sys/epoll.h>
#include <utility>

template <std::size_t Size>
struct MsgBuffer
{
	using Type = MsgBuffer<Size>;
	MsgBuffer(int socket)
		: sock(socket) {}

	BufferReader<Type> rec_msg()
	{
		std::printf("Waiting for message\n");

		nbytes = recvfrom(sock, message, Size, 0, (struct sockaddr *)&addr, (socklen_t *)&addrlen);

		std::printf("Processing Message of %i from %s\n", nbytes, addr.sun_path);
		return BufferReader<Type>(*this);
	}
	void get_sender(struct sockaddr_un &address, size_t &length)
	{
		address = addr;
		length = addrlen;
	}

	char message[Size];
	struct sockaddr_un addr;
	size_t addrlen;
	int nbytes;
	int sock;
};

template <std::size_t Size>
struct BufferReader<MsgBuffer<Size>> : BufferReader_Base
{
	using Buffer_Type = MsgBuffer<Size>;
	BufferReader(MsgBuffer<Size> &_buffer)
		: buffer(_buffer), BufferReader_Base(buffer.message, buffer.nbytes)
	{
	}

	MsgBuffer<Size> &buffer;
};

template <int Domain>
struct ClientConnection : Socket<Domain>
{
	using Base = Socket<Domain>;

	socklen_t addrlen;

	ClientConnection(int server_desc, int epollfd, struct epoll_event &ev)
	{
		Base::socket_desc = accept(server_desc, (struct sockaddr *)&Base::address.address, (socklen_t *)addrlen);
		SOCKET_ERROR_CHECK(Base::socket_desc, "Failed to accept");
		ev.events = EPOLLIN | EPOLLHUP | EPOLLOUT;
		ev.data.fd = Base::socket_desc;

		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, Base::socket_desc, &ev) == -1)
		{
			perror("epoll_ctl: listen_sock");
			exit(EXIT_FAILURE);
		}
	}
};

template <int Domain>
struct ServerSocket : Socket<Domain>
{
	struct epoll_event ev;
	int epollfd;

	using Base = Socket<Domain>;

	template <typename... ArgsT>
	ServerSocket(const int type, const int connectionQueue, ArgsT... Args)
	{
		Base::create_descriptor(Domain, type);
		Base::set_address(Args...);
		if (::bind(Base::socket_desc, (struct sockaddr *)&Base::address.address, sizeof(Base::address.address)) == -1)
		{
			// will break with changes
			unlink(Args...);
			SOCKET_ERROR_CHECK(::bind(Base::socket_desc, (struct sockaddr *)&Base::address.address, sizeof(Base::address.address)), "Failed to Bind socket");
		}

		// SOCKET_ERROR_CHECK(::listen(Base::socket_desc, connectionQueue), "Failed to listen");

		ev.events = EPOLLIN;
		ev.data.fd = Base::socket_desc;

		epollfd = epoll_create1(0);

		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, Base::socket_desc, &ev) == -1)
		{
			perror("epoll_ctl: listen_sock");
			exit(EXIT_FAILURE);
		}
	}

	~ServerSocket()
	{
		shutdown(Base::socket_desc, SHUT_RDWR);
	}

	inline void closeConnection(int &connection_desc)
	{
		close(connection_desc);
	}

	template <typename T>
	ssize_t read(int &connection_desc, void *buf, size_t count)
	{
		return ::read(connection_desc, buf, count);
	}
};