#include "Socket.h"
#include <sys/epoll.h>
#include <utility>

template <std::size_t Size>
struct Buffer
{
	template <typename... ArgsT>
	bool read(std::tuple<ArgsT...> &_parameters)
	{
		return read_tuple(_parameters, std::make_index_sequence<sizeof...(ArgsT)>());
	}

	// template <typename... Args>
	// bool read(Args &..._parameters)
	// {
	// }

	template <typename T>
	bool read(T &_value)
	{
		char *readEnd = dataPos + sizeof(T);
		if (readEnd < dataEnd)
		{
			read_internal(&_value);
			return true;
		}
		return false;
		// T *out = (T *)(bytes + pos);
		// pos += sizeof(T);
		// _value = *(out);
		//(pos < bytesHeld) ? out : nullptr;
	}

  protected:
	template <typename... ArgsT, std::size_t... I>
	bool read_tuple(std::tuple<ArgsT...> &_parameters, std::index_sequence<I...>)
	{
		char *readEnd = dataPos + (sizeof(ArgsT) + ...);
		if (readEnd < dataEnd)
		{
			(read_internal(std::get<I>(_parameters)), ...);
			return true;
		}
		return false;
	}

	template <typename T>
	void read_internal(T *_value)
	{
		_value = (T *)(dataPos);
		dataPos += sizeof(T);
	}

	char bytes[Size];
	char *bufferEnd = bytes + Size;
	char *dataPos;
	char *dataEnd;
};

template <std::size_t Size>
struct SocketBuffer : Buffer<Size>
{
	SocketBuffer(int fd)
		: descriptor(fd) {}
	using Base = Buffer<Size>;

	// template <typename... Args>
	// bool read(Args &..._parameters)
	// {
	// }

	// int getSize() { return Size; }
	bool write(auto &_value)
	{
		::write(descriptor, (void *)_value, sizeof(_value));
	}

	template <typename T>
	bool read(T &_value)
	{
		if (!Base::read(_value))
		{
			read_fd();
			return Base::read(_value);
		}
	}
	bool reachedEOF(){
		return eof_reached;
	}

  private:
	bool read_fd()
	{

		int unreadBytes = Base::bufferEnd - Base::dataPos;
		char *readStart = Base::bytes;
		size_t readAmount = Size;
		if (unreadBytes != 0)
		{
			readStart = (char*)memmove(Base::bytes, Base::dataPos, unreadBytes);
			readAmount = Size - unreadBytes;
		}

		int bytesRead = ::read(descriptor, readStart, readAmount);
		if (bytesRead == 0)
		{
			eof_reached = true;
		}
		else
		{
			Base::dataEnd = readStart + bytesRead;
		}
		return !eof_reached;
	}

	// template <typename T>
	// void read_internal(T &_value)
	// {
	// 	char *readEnd = dataPos + sizeof(T);
	// 	if (readEnd < dataEnd)
	// 	{
	// 		_value = (T *)(dataPos);
	// 		dataPos += sizeof(T);
	// 		// _value = *(out);
	// 	}
	// 	else
	// 	{
	// 		if (read_fd())
	// 	}
	// }

	// template <typename T>
	// void read_internal(T& _value)
	// {
	// 	T *out = (T *)(bytes + pos);
	// 	pos += sizeof(T);
	// 	_value = *(out);
	// 	//(pos < bytesHeld) ? out : nullptr;
	// }

	int descriptor;

	// char bytes[Size];
	// char *bufferEnd = bytes + Size;
	// char *dataPos;
	// char *dataEnd;
	bool eof_reached = false;
	// uint16_t pos;
};

template <int Domain>
struct ClientConnection : Socket<Domain>
{
	using Base = Socket<Domain>;

	socklen_t addrlen;

	ClientConnection(int server_desc, int epollfd, struct epoll_event& ev){
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

	std::vector<ClientConnection<Domain>> connectedClients;

	struct epoll_event ev;
	int epollfd;
	// struct pollfd pfds;
	using Base = Socket<Domain>;

	template <typename... ArgsT>
	ServerSocket(const int type, const int connectionQueue, ArgsT... Args)
	{
		Base::create_descriptor(Domain, type);
		Base::set_address(Args...);
		SOCKET_ERROR_CHECK(::bind(Base::socket_desc, (struct sockaddr *)&Base::address.address, sizeof(Base::address.address)), "Failed to Bind socket");
		SOCKET_ERROR_CHECK(::listen(Base::socket_desc, connectionQueue), "Failed to listen");

		ev.events = EPOLLIN;
		ev.data.fd = Base::socket_desc;

		epollfd = epoll_create1(0);

		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, Base::socket_desc, &ev) == -1)
		{
			perror("epoll_ctl: listen_sock");
			exit(EXIT_FAILURE);
		}
		// pfds.fd = Base::socket_desc;
		// pfds.events = POLLIN;
	}

	~ServerSocket()
	{
		shutdown(Base::socket_desc, SHUT_RDWR);
	}

	template <std::size_t MaxEvents = 5>
	inline int wait_for(auto _func)
	{
		struct epoll_event events[MaxEvents];
		int eventCount = epoll_wait(epollfd, events, MaxEvents, -1);

		for (int i = 0; i < eventCount; i++)
		{
			struct epoll_event &currentEvent = events[i];

			if (events[i].data.fd == Base::socket_desc)
			{
				connectedClients.emplace_back(Base::socket_desc, epollfd, ev);
				// acceptConnection();
			}
			else
			{
				if (events[i].events & EPOLLRDHUP)
				{
					epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
					close(events[i].data.fd);
				}
				else if (events[i].events & EPOLLIN)
				{
					SocketBuffer<128> buffer(events[i].data.fd);
					_func(buffer);
				}
				else if (events[i].events & EPOLLOUT)
				{
				}
			}
		}

		// if(poll(&pfds, (nfds_t)1, 10) >=0)
		// {
		// 	if(pfds.revents & POLLIN)
		// 	{
		// 		acceptConnection();
		// 	}
		// }
	}

	// inline void acceptConnection()
	// {
		
	// 	socket.connection_desc = accept(Base::socket_desc, (struct sockaddr *)&socket.address, (socklen_t *)socket.addrlen);
	// 	SOCKET_ERROR_CHECK(socket.connection_desc, "Failed to accept");
	// 	ev.events = EPOLLIN | EPOLLHUP | EPOLLOUT;
	// 	ev.data.fd = Base::socket_desc;

	// 	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, Base::socket_desc, &ev) == -1)
	// 	{
	// 		perror("epoll_ctl: listen_sock");
	// 		exit(EXIT_FAILURE);
	// 	}
	// }

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