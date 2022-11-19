#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

#if __has_include("ctre.hpp")
#include "ctre.hpp"
#endif

// TODO look into replacing Fixed_Array with std::array
template <typename T, std::size_t N>
class Fixed_Array_Base
{
  public:
	static constexpr std::size_t Size = N;
	using type = T;

	T data[N]{};

	constexpr Fixed_Array_Base() {} // : Fixed_Array_Base(obj, std::make_index_sequence<N>()) {}

	template <typename IndexT, IndexT... index>
	constexpr Fixed_Array_Base(T obj, std::integer_sequence<IndexT, index...>)
		: data{}
	{
	}
	constexpr Fixed_Array_Base(T obj)
		: Fixed_Array_Base(obj, std::make_index_sequence<N>()) {}

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

	constexpr std::size_t size() const noexcept
	{
		return N;
	}
	using iterator = T *;
	using const_iterator = const T *;
	constexpr iterator begin() noexcept { return data; }
	constexpr const_iterator begin() const noexcept { return data; }
	constexpr iterator end() noexcept { return data + N; }
	constexpr const_iterator end() const noexcept { return data + N; }
};

template <typename T, std::size_t N>
struct Fixed_Array : Fixed_Array_Base<T, N>
{
  public:
	constexpr Fixed_Array()
		: Fixed_Array_Base<T, N>() {}

	constexpr Fixed_Array(const T (&src)[N])
		: Fixed_Array_Base<T, N>(src) {}
	constexpr Fixed_Array(const Fixed_Array<T, N> &src)
		: Fixed_Array_Base<T, N>(src) {}
	template <typename... ArgsT>
	constexpr Fixed_Array(const ArgsT &...Args)
		: Fixed_Array_Base<T, N>(Args...)
	{}
};
template <typename T, std::size_t N>
struct Array_Wrapper_Base
{
	static constexpr size_t size = N;
	using type = T;
};

// Figure out if I can remove this for something in the standard libary
template <typename T>
struct Array_Wrapper;

template <typename... ArrT>
constexpr size_t Array_Sum()
{
	return (Array_Wrapper<ArrT>::size + ...);
}

// template <typename T, typename... ArgsT>
// Fixed_Array(const T &, const ArgsT &...) -> Fixed_Array<Array_Type<T>, Array_Sum<T, ArgsT...>()>;

template <typename T, std::size_t N>
struct Array_Wrapper<T[N]> : Array_Wrapper_Base<T, N>
{};

template <typename T, std::size_t N>
struct Array_Wrapper<Fixed_Array<T, N>> : Array_Wrapper_Base<T, N>
{};

template <typename T, std::size_t N>
struct Array_Wrapper<std::array<T, N>> : Array_Wrapper_Base<T, N>
{};

template <std::size_t N>
requires(N > 0) struct Fixed_String : Fixed_Array_Base<char, N>
{
  public:
	using iterator = std::string_view::iterator;

	using Array = Fixed_Array_Base<char, N>;

	constexpr Fixed_String(const char (&src)[N])
		: Array(src) {}

	constexpr Fixed_String(const Fixed_Array<char, N> &src)
		: Array(src) {}

	template <typename... ArgsT>
	explicit constexpr Fixed_String(const ArgsT &...Args)
	{
		copy(Array::data, Args...);
	}

	// TODO create copy error for incorrect size
	template <size_t SrcN, typename... ArgsT>
	inline static constexpr void copy(auto _dst, const auto &_src, const ArgsT &...Args)
	{
		copy(std::copy_n(_src.data, SrcN - 1, _dst), Args...);
	}

	template <size_t SrcN, typename... ArgsT>
	inline static constexpr void copy(auto _dst, const Fixed_String<SrcN> &_src, const ArgsT &...Args)
	{
		copy(std::copy_n(_src.data, SrcN - 1, _dst), Args...);
	}

	template <typename... ArgsT>
	inline static constexpr void copy(auto _dst, const char _src, const ArgsT &...Args)
	{
		_dst[0] = _src;

		copy(_dst + 1, Args...);
	}
	template <size_t SrcN, typename... ArgsT>
	inline static constexpr void copy(auto _dst, const char (&_src)[SrcN], const ArgsT &...Args)
	{
		copy(std::copy_n(_src, SrcN - 1, _dst), Args...);
	}
	inline static constexpr void copy(const char *_dst) {}
	constexpr std::string_view to_string_view() const noexcept { return std::string_view{Array::data}; }
	constexpr const char *to_char_ptr() const noexcept { return Array::data; }

	constexpr operator std::string_view() const noexcept { return to_string_view(); }
	constexpr operator const char *() const noexcept { return to_char_ptr(); }

	constexpr iterator end() const noexcept { return to_string_view().end(); }
	constexpr iterator begin() const noexcept { return to_string_view().begin(); }

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

	constexpr auto hash()
	{
		return ELFHash(Array::data, N);
	}

#ifdef CTRE_V2__CTRE__HPP
	constexpr operator ctll::fixed_string<N - 1>() const noexcept
	{
		return ctll::fixed_string(Array::data);
	}

#endif
};

#ifdef CTRE_V2__CTRE__HPP
namespace regex
{
template <const Fixed_String regex>
static constexpr inline auto regex_match = ctre::match<regex>;

template <const Fixed_String regex>
static constexpr inline auto regex_search = ctre::search<regex>;

template <const Fixed_String regex>
static constexpr inline auto regex_starts_with = ctre::starts_with<regex>;

template <const Fixed_String regex>
static constexpr inline auto regex_range = ctre::range<regex>;

template <const Fixed_String regex>
static constexpr inline auto regex_split = ctre::split<regex>;

template <const Fixed_String regex>
static constexpr inline auto regex_tokenize = ctre::tokenize<regex>;

template <const Fixed_String regex>
static constexpr inline auto regex_iterator = ctre::iterator<regex>;
} // namespace regex
#endif

template <std::size_t N>
struct Array_Wrapper<Fixed_String<N>> : Array_Wrapper_Base<char, N>
{};

// TODO clean this up it's a mess
template <typename... ArgsT>
requires((Array_Wrapper<ArgsT>::size > 0), ...)
	Fixed_String(const ArgsT &...)
->Fixed_String<(Array_Wrapper<ArgsT>::size + ...) - sizeof...(ArgsT) + 1>;

#ifdef CTRE_V2__CTRE__HPP
namespace ctll
{
template <size_t N>
fixed_string(Fixed_String<N>) -> fixed_string<N - 1>;
}
#endif
