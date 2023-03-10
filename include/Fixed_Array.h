#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <string_view>

#if __has_include("ctre.hpp")
#include "ctre.hpp"
#endif

// TODO look into replacing Fixed_Array with std::array
template <typename T, std::size_t N>
struct Fixed_Array_Base
{
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	static constexpr std::size_t Size = N;
	using type = T;

	T data[N]{};

	constexpr Fixed_Array_Base() {}

	template <typename IndexT, IndexT... index>
	constexpr Fixed_Array_Base(T obj, std::integer_sequence<IndexT, index...>)
		: data{}
	{
	}
	constexpr Fixed_Array_Base(T obj)
		: Fixed_Array_Base(obj, std::make_index_sequence<N>{}) {}

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

	constexpr const T &operator[](std::size_t _index) const { return data[_index]; }
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

	constexpr reference front() { return data[0]; };
	constexpr const_reference front() const { return *data[0]; };

	constexpr reference back() { return data[N - 1]; };
	constexpr const_reference back() const { return data[N - 1]; };
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

template <typename T>
struct array_size;

template <typename T, std::size_t N>
struct array_size<T[N]> :
	public std::integral_constant<std::size_t, N>
{};

template <typename T, std::size_t N>
struct array_size<Fixed_Array<T, N>> :
	public std::integral_constant<std::size_t, N>
{};

template <typename T, std::size_t N>
struct array_size<std::array<T, N>> :
	public std::integral_constant<std::size_t, N>
{};

template <typename T>
inline constexpr std::size_t array_size_v = array_size<T>::value;

template <std::size_t N>
struct Fixed_String : Fixed_Array_Base<char, N>
{
  public:
	using iterator = std::string_view::iterator;

	using Array = Fixed_Array_Base<char, N>;

	constexpr Fixed_String(const char (&src)[N])
		: Array(src) {}

	constexpr Fixed_String(const Fixed_Array<char, N> &src)
		: Array(src) {}

	// template <const char *BasePtr>
	// constexpr Fixed_String(const fixed_string_view<BasePtr> &view)
	// {
	// 	std::copy_n(view.begin(), view.size, Array::data);
	// }

	explicit constexpr Fixed_String(const std::string_view str)
	{
		std::copy(str.begin(), str.end(), Array::data);
		// copy(, str);
	}

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

	template <typename... ArgsT>
	inline static constexpr void copy(auto _dst, const std::string_view &_src, const ArgsT &...Args)
	{
		copy(std::copy(_src.begin(), _src.end(), _dst), Args...);
	}

	inline static constexpr void copy(const char *_dst) {}

	constexpr const std::string_view to_string_view() const noexcept
	{
		if (Array::data[N - 1] == '\0')
			return std::string_view(Array::data, N - 1);
		else
			return std::string_view(Array::data, N);
	}

	constexpr const char *to_char_ptr() const noexcept { return Array::data; }

	constexpr operator std::string_view() const noexcept { return to_string_view(); }
	constexpr operator const char *() const noexcept { return to_char_ptr(); }

	constexpr iterator end() const noexcept { return to_string_view().end(); }
	constexpr iterator begin() const noexcept { return to_string_view().begin(); }

	constexpr bool operator==(const std::string_view &_str) const
	{
		// TODO check for null end character
		if (_str.size() == N - 1)
		{
			for (std::size_t i = 0; i < _str.size(); i++)
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

template <auto &BaseStr>
struct fixed_string_view
{
	std::size_t m_offset;
	std::size_t m_size;

	constexpr fixed_string_view()
		: m_size(0), m_offset(0) {}

	explicit constexpr fixed_string_view(std::size_t m_offset, std::size_t size)
		: m_offset(m_offset), m_size(size) {}

	constexpr fixed_string_view(std::size_t size)
		: m_offset(0), m_size(size) {}

	constexpr fixed_string_view(const char *ptr, std::size_t size)
		: m_offset(ptr - BaseStr.data), m_size(size) {}

	constexpr fixed_string_view(const std::string_view _view)
		: fixed_string_view(_view.data(), _view.size()) {}

	constexpr fixed_string_view(fixed_string_view &&obj)
		: fixed_string_view(obj.m_offset, obj.size()) {}
	constexpr fixed_string_view(const fixed_string_view &obj)
		: fixed_string_view(obj.m_offset, obj.size()) {}

	// constexpr const char *end() const noexcept { return *BasePtr + m_offset + size; }
	// constexpr const char *begin() const noexcept { return *BasePtr + m_offset; }

	static constexpr const char *data() { return BaseStr.data; }
	constexpr std::size_t size() const { return m_size; }

	constexpr const fixed_string_view operator=(const fixed_string_view obj)
	{
		m_offset = obj.m_offset;
		m_size = obj.size();
		return *this;
	}

	constexpr std::string_view to_string_view() const noexcept { return std::string_view(BaseStr.data + m_offset, m_size); }
	static constexpr std::string_view full_str() noexcept { return BaseStr; }

	constexpr operator std::string_view() const noexcept { return to_string_view(); }

	constexpr bool empty() const noexcept { return m_size == 0; }
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
struct array_size<Fixed_String<N>> :
	public std::integral_constant<std::size_t, N>
{};

template <typename T>
inline constexpr auto string_size(const T &str)
{
	const std::size_t size = array_size_v<T>;
	return (str[size - 1] != '\0') ? size : size - 1;
};

template <typename T, std::size_t N>
inline constexpr auto string_start(const T (&str)[N])
{
	return str;
};

template <typename T>
inline constexpr auto string_start(const T &str)
{
	return (char *)str.begin();
};

template <typename... ArgsT>
requires((array_size_v<ArgsT> > 0), ...) Fixed_String(const ArgsT &...)
->Fixed_String<(array_size_v<ArgsT> + ...) - sizeof...(ArgsT) + 1>;

template <Fixed_String... Strings>
static inline constexpr auto make_fixed_string()
{
	using fixed_string = Fixed_String<(string_size(Strings) + ...)>;
	std::array<std::size_t, sizeof...(Strings)> string_sizes{string_size(Strings)...};
	std::array<char *, sizeof...(Strings)> string_ptrs{string_start(Strings)...};

	fixed_string output;

	auto ptr_itt = string_ptrs.begin();
	auto size_itt = string_sizes.begin();
	char *current = output.data;
	for (; ptr_itt < string_ptrs.end(); ptr_itt++, size_itt++)
	{
		current = std::copy_n(*ptr_itt, *size_itt, current);
	}
	return output;
};

	// template<auto& String>
	// using fixed_string_view_t = fixed_string_view<String.data, String.size()>;

	// template <auto& String>
	// static inline constexpr auto make_fixed_view()
	// {

	// }

#ifdef CTRE_V2__CTRE__HPP
namespace ctll
{
template <size_t N>
fixed_string(Fixed_String<N>) -> fixed_string<N - 1>;
}
#endif
