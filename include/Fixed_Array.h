#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
// #include <ctll/fixed_string.hpp>

// TODO look into replacing Fixed_Array with std::array
template <typename T, std::size_t N>
class Fixed_Array_Base
{
  public:
	static constexpr std::size_t Size = N;
	using type = T;

	T data[N]{};

	constexpr Fixed_Array_Base(const T (&src)[N]) { std::copy_n(src, N, data); }
	constexpr Fixed_Array_Base(const Fixed_Array_Base<T, N> &src) { std::copy_n(src.data, N, data); }

	template <typename... ArgsT>
	constexpr Fixed_Array_Base(const ArgsT &...Args)
	{
		copy(data, Args...);
	}

	template <size_t SrcN, typename... ArgsT>
	inline static constexpr void copy(auto _dst, const Fixed_Array_Base<T, SrcN> &_src, const ArgsT &...Args)
	{
		copy(std::copy_n(_src.data, SrcN, _dst), Args...);
	}
	template <size_t SrcN, typename... ArgsT>
	inline static constexpr void copy(auto _dst, const T (&_src)[SrcN], const ArgsT &...Args)
	{
		copy(std::copy_n(_src, SrcN, _dst), Args...);
	}
	inline static constexpr void copy(const T *_dst) {}

	constexpr T &operator[](std::size_t _index) { return data[_index]; }
};

template <typename T, std::size_t N>
struct Fixed_Array : Fixed_Array_Base<T, N>
{
  public:
	constexpr Fixed_Array(const T (&src)[N])
		: Fixed_Array_Base<T, N>(src) {}
	constexpr Fixed_Array(const Fixed_Array<T, N> &src)
		: Fixed_Array_Base<T, N>(src) {}
	template <typename... ArgsT>
	constexpr Fixed_Array(const ArgsT &...Args)
		: Fixed_Array_Base<T, N>(Args...)
	{}
};

template <typename ArrT>
class Array_Metadata;

template <typename ArrT>
using Array_Type = typename Array_Metadata<ArrT>::type;

template <typename... ArrT>
constexpr size_t Array_Sum()
{
	return (Array_Metadata<ArrT>::size + ...);
}

template <typename T, typename... ArgsT>
Fixed_Array(const T &, const ArgsT &...) -> Fixed_Array<Array_Type<T>, Array_Sum<T, ArgsT...>()>;

template <typename T, std::size_t N>
struct Array_Metadata_Base
{
	static constexpr size_t size = N;
	using type = T;
};

template <typename T, std::size_t N>
struct Array_Metadata<T[N]> : Array_Metadata_Base<T, N>
{};

template <typename T, std::size_t N>
struct Array_Metadata<Fixed_Array<T, N>> : Array_Metadata_Base<T, N>
{};

template <typename... Args>
static constexpr auto MergeFixedStr(Args &&..._tkn)
{
	Fixed_Array output(_tkn...);
	constexpr std::size_t size = output.Size - 2;
	return output;
}
template <std::size_t N>
struct Fixed_String : Fixed_Array_Base<char, N>
{
  public:
	using Array = Fixed_Array_Base<char, N>;

	constexpr Fixed_String(const char (&src)[N])
		: Array(src) {}
	constexpr Fixed_String(const Fixed_Array<char, N> &src)
		: Array(src) {}

	template <typename... ArgsT>
	constexpr Fixed_String(const ArgsT &...Args)
	{
		copy(Array::data, Args...);
	}

	template <size_t SrcN, typename... ArgsT>
	inline static constexpr void copy(auto _dst, const Fixed_String<SrcN> &_src, const ArgsT &...Args)
	{
		copy(std::copy_n(_src.data, SrcN - 1, _dst), Args...);
	}
	template <size_t SrcN, typename... ArgsT>
	inline static constexpr void copy(auto _dst, const char (&_src)[SrcN], const ArgsT &...Args)
	{
		copy(std::copy_n(_src, SrcN - 1, _dst), Args...);
	}
	inline static constexpr void copy(const char *_dst) {}

	constexpr operator std::string_view() const noexcept { return std::string_view{Array::data}; }

	// constexpr operator ctll::fixed_string<N - 1>() const noexcept
	// {
	// 	return ctll::fixed_string(Array::data);
	// }

	constexpr bool operator==(std::string_view &_str)
	{
		if (_str.size() == N)
		{
			for (std::size_t i = 0; i < N; i++)
			{
				if (_str[i] != Array::data[i])
					return false;
			}
			return true;
		}
		return false;
	}
};

template <std::size_t N>
struct Array_Metadata<Fixed_String<N>> : Array_Metadata_Base<char, N>
{};

template <typename... ArgsT>
Fixed_String(const ArgsT &...) -> Fixed_String<Array_Sum<ArgsT...>() - sizeof...(ArgsT) + 1>;

// namespace ctll
// {
// template <size_t N>
// fixed_string(Fixed_String<N>) -> fixed_string<N - 1>;
// }