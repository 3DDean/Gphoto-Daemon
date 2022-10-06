#include "Fixed_Array.h"
#include <concepts>
#include <functional>
#include <type_traits>

// Do not use this, it is intended to throw an error
// I use it when I Need to figure out what a type resolves to
template <typename T>
struct human_readable_type_with_error
{
	static_assert(std::is_same_v<int, T>);
};

template <typename T>
concept member_object_pointer = std::is_member_object_pointer<T>::value;

template <typename T>
concept member_function_pointer = std::is_member_function_pointer<T>::value;

template <typename... Ts>
struct type_sequence
{
	// template <template <typename... ParamPack> class container>
	// using wrap = container<Ts...>;

	// template <template <typename T, typename...> class container>
	// using wrap_each = type_sequence<container<Ts>...>;

	// constexpr static std::size_t size = sizeof...(Ts);
};

template <Fixed_String name, auto Ptr>
struct Member;

template <Fixed_String Name, typename ParentT, typename MemberT, MemberT ParentT::*Ptr>
requires(std::is_member_object_pointer<decltype(Ptr)>::value) struct Member<Name, Ptr>
{
	using type = MemberT;
	static constexpr auto ptr = Ptr;

	// auto operator()(ParentT &parent)
	// {
	// 	return std::mem_fn(ptr)(parent);
	// }
};

template <Fixed_String Name, typename ParentT, typename MemberT, MemberT ParentT::*Ptr>
requires(std::is_member_function_pointer<decltype(Ptr)>::value) struct Member<Name, Ptr>
{
	using type = MemberT;
	static constexpr auto ptr = Ptr;

	auto operator()(ParentT &parent)
	{
		return std::mem_fn(ptr)(parent);
	}
};

template <Fixed_String name, auto Ptr, typename Ret, typename... ArgsT>
struct MemberFunc;