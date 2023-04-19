#pragma once
#include <iostream>
#include <string.h>
#include <string>
#include <tuple>
#include <unistd.h>
#include <mutex>

struct c_array
{
	char *ptr;
	std::size_t nbytes;

	c_array(char *ptr = nullptr, std::size_t size = 0)
		: ptr(ptr), nbytes(size) {}

	char *begin()
	{
		return ptr;
	}
	char *end() { return ptr + nbytes; }
	std::size_t size() { return nbytes; }
};

struct memory_block : public c_array
{
	memory_block(std::size_t size)
		: c_array(new char[size], size)
	{}
};
struct buffer;

struct memory_chunk
{
	// memory_chunk(char* ptr)
	// 	: first(ptr), last(ptr) {}

	memory_chunk(char *first, char *last)
		: first(first), last(last) {}
	memory_chunk(char *first, std::size_t size)
		: first(first), last(first + size) {}

	char *begin() { return first; }
	char *end() { return last; }
	std::size_t size() { return last - first; }

	// Remove that memory for the memory chunk
	void consume(std::size_t amount)
	{
		char *target_pos = first + amount;
		if (target_pos < last)
			first = target_pos;
		else
			throw std::runtime_error("Buffer corruption detected");
	}

	void consume(const char *target)
	{
		consume(target - first);
	}
	void set_last(char *ptr) { last = ptr; }

  private:
	char *first, *last;
};

struct buffer
{
	buffer(std::size_t size = 512)
		: memory(size),
		  data(memory.begin(), memory.begin()),
		  unused(memory.begin(), memory.end())
	{}

	memory_block memory;
	memory_chunk data;
	memory_chunk unused;

	std::mutex data_mtx, unused_mtx;

	template <typename T>
	void write_to(T* input)
	{
		std::lock_guard lk(unused_mtx);

		int amount_read = input->read_to(unused.begin(), unused.size());

		unused.consume(amount_read);
	}

	void move_data()
	{
		std::lock_guard lk(data_mtx);
		int amount = data.size();
		memcpy(memory.begin(), data.begin(), amount);
		data = memory_chunk(memory.begin(), memory.begin() + amount);
	}

	auto read()
	{
		std::unique_lock data_lock(data_mtx);
		{
			std::lock_guard lk(unused_mtx);
			data.set_last(unused.begin());
		}

		return std::pair<std::unique_lock<std::mutex> &, memory_chunk &>(data_lock, data);
	}
	
};
