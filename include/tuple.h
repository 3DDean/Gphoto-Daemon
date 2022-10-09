#pragma once
#include <stdint.h>
#include <tuple>
#include <utility>

template <std::size_t I, typename T, typename... ArgsT>
concept IndexedFunctorCall = requires(T _obj, ArgsT &..._args)
{
	{
		_obj.template operator()<I>(_args...)
		} -> std::same_as<bool>;
};
template <typename T, typename... ArgsT>
concept FunctorCall = requires(T _obj, ArgsT &..._args)
{
	{
		_obj.operator()(_args...)
		} -> std::same_as<bool>;
};

template <std::size_t I = 0, typename ElementT, typename... ArgsT>
requires IndexedFunctorCall<I, ElementT, ArgsT...>
static inline constexpr bool callSwitchFunctor(ElementT &_element, ArgsT &..._args)
{
	return _element.template operator()<I>(_args...);
}

template <std::size_t I = 0, typename ElementT, typename... ArgsT>
requires IndexedFunctorCall<I, ElementT, ArgsT...>
static inline constexpr bool callSwitchFunctor(ElementT *_element, ArgsT &..._args)
{
	return _element->template operator()<I>(_args...);
}

template <std::size_t I = 0, typename ElementT, typename... ArgsT>
requires FunctorCall<ElementT, ArgsT...>
static inline constexpr bool callSwitchFunctor(ElementT &element, ArgsT &..._args)
{
	return element.template operator()(_args...);
}

template <std::size_t I = 0, typename... Ts, typename... ArgsT>
static inline constexpr bool tupleFunctorSwitch(std::tuple<Ts...> &_tuple, ArgsT &..._args)
{
	if constexpr (I < sizeof...(Ts))
	{
		auto& func = std::get<I>(_tuple);

		if (callSwitchFunctor<I>(func, _args...))
		{
			return true;
		}
		return tupleFunctorSwitch<I + 1>(_tuple, _args...);
	}
	return false;
}


// template<typename T, T... ints, typename... ArgsT>
// auto mk_tuple(std::integer_sequence<T, ints...> int_seq, ArgsT... _instructions)
// {
// 	std::tuple<ArgsT...>()
// }

// template<typename... ArgsT>
// auto mk_tuple(ArgsT&... elements)
// {
// 	return mk_tuple(std::make_index_sequence<sizeof...(elements)>(), elements...);
// }