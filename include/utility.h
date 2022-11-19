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
using to_integer_sequence_t = typename to_integer_sequence<Array>::type;

struct type_not_found : std::integral_constant<std::size_t, std::size_t(-1)>
{
};

template <std::size_t Index>
using index_constant = std::integral_constant<std::size_t, Index>;

template <std::size_t Index, typename T, typename... TypeList>
struct index_finder;

template <std::size_t Index, typename T, typename Head, typename... Tail>
struct index_finder<Index, T, Head, Tail...> : std::conditional_t<std::is_same_v<T, Head>, index_constant<Index>, index_finder<Index + 1, T, Tail...>>
{};

template <std::size_t Index, typename T>
struct index_finder<Index, T> : std::integral_constant<type_not_found, type_not_found{}>
{};

template <typename T, typename Container>
struct index_of;

template <typename T, template <typename...> class container, typename... ArgsT>
struct index_of<T, container<ArgsT...>> : index_finder<0, T, ArgsT...>
{
};

template <typename T, typename Container>
static constexpr auto index_of_v = index_of<T, Container>::value;

template <typename T, typename Container>
struct contains
{
	using index_result = index_of<T, Container>;
	static constexpr auto index = index_result::value;

	static constexpr bool value = !std::is_same_v<typename index_result::value_type, type_not_found>;
	using value_type = bool;
	using type = contains;
	constexpr operator value_type() const noexcept { return value; }
	constexpr value_type operator()() const noexcept { return value; }
};

struct ignore_t
{};

inline constexpr ignore_t ignore;
