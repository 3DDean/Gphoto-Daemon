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
static inline constexpr bool invoke_functor(ElementT &_element, ArgsT &..._args)
{
	return _element.template operator()<I>(_args...);
}

template <std::size_t I = 0, typename ElementT, typename... ArgsT>
requires IndexedFunctorCall<I, ElementT, ArgsT...>
static inline constexpr bool invoke_functor(ElementT *_element, ArgsT &..._args)
{
	return _element->template operator()<I>(_args...);
}

template <std::size_t I = 0, typename ElementT, typename... ArgsT>
requires FunctorCall<ElementT, ArgsT...>
static inline constexpr bool invoke_functor(ElementT &element, ArgsT &..._args)
{
	return element.template operator()(_args...);
}

template <std::size_t I = 0, typename... Ts, typename... ArgsT>
static inline constexpr bool tuple_switch(std::tuple<Ts...> &_tuple_of_functors, ArgsT &..._args)
{
	if constexpr (I < sizeof...(Ts))
	{
		auto &func = std::get<I>(_tuple_of_functors);

		if (invoke_functor<I>(func, _args...))
		{
			return true;
		}
		return tuple_switch<I + 1>(_tuple_of_functors, _args...);
	}
	return false;
}

template <std::size_t I = 0, typename... Ts, typename... ArgsT>
static inline constexpr bool tuple_switch(auto&& comparator, auto &&_func, std::tuple<Ts...> &_tuple)
{
	if constexpr (I < sizeof...(Ts))
	{
		auto &&element = std::get<I>(_tuple);

		if (invoke_functor<I>(comparator, element))
		{
			_func(element);
			// invoke_functor<I>(_func, element);

			return true;
		}
		return tuple_switch<I + 1>(comparator,_func, _tuple);
	}
	return false;
}


//I don't know why this is uncommented
// template <std::size_t I = 0, typename... Ts, typename... ArgsT>
// static inline constexpr bool tuple_switch(auto &_func, std::tuple<Ts...> &_tuple, ArgsT &..._args)
// {
// 	if constexpr (I < sizeof...(Ts))
// 	{
// 		if (invoke_functor<I>(_func, std::get<I>(_tuple), _args...))
// 		{
// 			return true;
// 		}
// 		return tuple_switch<I + 1>(_func, _tuple, _args...);
// 	}
// 	return false;
// }

template <std::size_t I = 0, typename... Ts, typename... ArgsT>
static inline constexpr bool tuple_foreach(std::tuple<Ts...> &_tuple, ArgsT &..._args)
{
	if constexpr (I < sizeof...(Ts))
	{
		auto &func = std::get<I>(_tuple);

		if (invoke_functor<I>(func, _args...))
		{
			return tuple_foreach<I + 1>(_tuple, _args...);
		}
		return false;
	}
	return false;
}

template <std::size_t I = 0, typename... Ts, typename... ArgsT>
static inline constexpr bool tuple_foreach(auto &_func, const std::tuple<Ts...> &_tuple, ArgsT &..._args)
{
	if constexpr (I < sizeof...(Ts))
	{
		if (invoke_functor<I>(_func, std::get<I>(_tuple), _args...))
		{
			return tuple_foreach<I + 1>(_func, _tuple, _args...);
		}
		return false;
	}
	return false;
}

template <typename... T>
struct tuple_cat;

template <typename... Ts1, typename... Ts2>
struct tuple_cat<std::tuple<Ts1...>, std::tuple<Ts2...>>
{
	using type = std::tuple<Ts1..., Ts2...>;
};
