#pragma once
#include <array>
#include <utility>

template <auto Array, typename index_sequence = std::make_index_sequence<Array.size()>>
struct to_integer_sequence;

template <typename T, std::size_t N, std::array<T, N> Array, typename IndexT, IndexT... Indices>
struct to_integer_sequence<Array, std::integer_sequence<IndexT, Indices...>>
{
	using type = std::integer_sequence<IndexT, Array[Indices]...>;
};

template <auto Array>
using to_integer_sequence_t = to_integer_sequence<Array>::type;

static inline constexpr std::size_t type_not_found = -1;

template <std::size_t I, typename T, typename Head, typename... ArgsT>
constexpr inline std::size_t get_index()
{
	if constexpr (std::same_as<T, Head>)
		return I;
	else if constexpr (sizeof...(ArgsT) > 0)
		return get_index<I + 1, T, ArgsT...>();
	else
		return type_not_found;
}

template <typename T, template <typename...> class container, typename... ArgsT>
constexpr inline std::size_t get_index(container<ArgsT...>)
{
	return get_index<0, T, ArgsT...>();
}
