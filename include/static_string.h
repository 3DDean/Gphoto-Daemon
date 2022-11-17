#pragma once
#include "Fixed_Array.h"

//This file exists as a bridge to better utilize fixed strings in formatting functions

template<Fixed_String Str>
struct static_string_type
{
	static constexpr Fixed_String str = Str;

	constexpr std::string_view to_string_view() const noexcept { return std::string_view{str}; }

	constexpr operator std::string_view() const noexcept { return to_string_view(); }
};

template<Fixed_String Str>
static inline constexpr auto static_string = static_string_type<Str>{};
