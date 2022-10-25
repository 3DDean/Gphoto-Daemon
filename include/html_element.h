#pragma once
#include "Fixed_Array.h"
#include <ctre.hpp>
#include <vector>

enum html_part_type
{
	constant,
	substitutable
};

struct html_part
{
	// constexpr html_part(html_part_type part_type, const std::string_view str) : part_type(part_type), str(str)
	// {}
	// template< class It, class End >
	// constexpr html_part(html_part_type part_type, const It first, const End last) : part_type(part_type), str(first, last)
	// {}

	template <typename... ArgsT>
	constexpr html_part(html_part_type part_type, ArgsT... Args)
		: part_type(part_type), str(Args...)
	{}

	html_part_type part_type;
	std::string_view str;
};

struct html_parts
{
	std::vector<html_part> parts;
	std::size_t constant_size = 0;

	template <typename... ArgsT>
	void create_part(html_part_type part_type, ArgsT... Args)
	{
		parts.emplace_back(part_type, Args...);

		if (part_type == html_part_type::constant)
		{
			constant_size += parts.back().str.size();
		}
	}
};

template <auto Str>
constexpr void html_proto_sub_helper(html_parts &html_parts)
{
	std::string_view lastMatch(Str, 0);

	for (auto [match, variableName] : ctre::range<"\\$\\{(.+?)\\}">(Str))
	{
		html_parts.create_part(html_part_type::constant, lastMatch.begin(), match.begin());
		html_parts.create_part(html_part_type::substitutable, variableName.begin(), variableName.end());

		lastMatch = std::string_view(match.end());
	}
	html_parts.create_part(html_part_type::constant, lastMatch);
}

static constexpr std::size_t get_size(auto FirstIt, auto LastIt, std::size_t count = 0)
{
	if (FirstIt == LastIt)
		return count;
	return get_size(++FirstIt, LastIt, ++count);
}

static constexpr auto get_size(const auto &Range)
{
	return get_size(Range.begin(), Range.end());
}

template <typename T>
struct regex_results_helper;

template <typename CharT, typename... Captures>
struct regex_results_helper<ctre::regex_results<CharT, Captures...>>
{
	using ctre_type = ctre::regex_results<CharT, Captures...>;

	static constexpr std::size_t size = sizeof...(Captures) + 1;

	using type = std::array<std::string_view, size>;

	template <typename T, T... index>
	constexpr type operator()(auto &result, std::integer_sequence<T, index...>)
	{
		return type{ctre::get<index>(result)...};
	}
	constexpr type operator()(auto &result) { return operator()(result, std::make_index_sequence<size>()); }
};

template <std::size_t Size>
static constexpr auto to_array(const auto Range)
{
	using value_type = typename decltype(Range.begin())::value_type;
	using result_converter = regex_results_helper<value_type>;
	using result_type = typename result_converter::type;
	result_converter converter;
	std::array<result_type, Size> temp;

	int i = 0;
	for (auto var : Range)
	{
		temp[i] = converter(var);
		i++;
	}

	return temp;
}

template <Fixed_String... ArgsT>
struct html_element_maker
{
	static constexpr Fixed_String fullStr{ArgsT...};

	static constexpr auto range = ctre::range<"\\$\\{(.+?)\\}">(fullStr.data);
	static constexpr std::size_t capture_count = get_size(range);
	static constexpr auto array = to_array<capture_count>(range);
};

void html_test()
{
	using testType = html_element_maker<"<input type=\"${type}\" id=\"${id}\" name=\"${name}\" ${value}>", "<label for=\"${id}\">${label}</label>">;
	auto range = testType::range;
	auto array = testType::array;

	int i = 0;
	i++;
}

// template <Fixed_String... Values>
// struct html_proto
// {
// 	template <typename... ArgsT>
// 	constexpr html_proto(const ArgsT &...Args)
// 	{
// 		// ctre::range<"(\\$\\{.+?\\})">;
// 	}
// };

// template <Fixed_String Attribute, Fixed_String Values>
// struct html_attribute
// {
// };
// /* type = "(\w+)"
// html_attribute<"class", "$1">
// */

// template <Fixed_String... Values>
// struct html_element;

// template <>
// struct html_element<"tab", "book">
// {
// 	static constexpr Fixed_String element{"tab-book"};
// };

// template <>
// struct html_element<"tab", "page">
// {
// 	static constexpr Fixed_String element{"tab-page"};
// };

// template <>
// struct html_element<"input", "text">
// {
// 	html_element(int minLength, int maxLength, int size)
// 	{

// 	}
// 	static constexpr Fixed_String element{"input"};
// 	static constexpr Fixed_String type{"text"};
// };

// template <>
// struct html_element<"input", "range">
// {
// 	static constexpr Fixed_String element{"input"};
// 	static constexpr Fixed_String type{"range"};
// };

// template <>
// struct html_element<"input", "checkbox">
// {
// 	static constexpr html_proto html_prototype{"<input type=\"${type}\" id=\"${id}\" name=\"${name}\" ${value}>",
// 												"<label for=\"${id}\">${label}</label>"};

// 	void create(auto format, char* id, char* name)
// 	{
// 		//Todo translate this
// 		// format << " << id

// 	// 	// scales" name="scales" checked>"
//     //   <label for="scales">Scales</label>
// 	}

// 	static constexpr Fixed_String element{"input"};
// 	static constexpr Fixed_String type{"checkbox"};
// };

// template <>
// struct html_element<"input", "radio">
// {
// 	static constexpr Fixed_String element{"input"};
// 	static constexpr Fixed_String type{"radio"};
// };

// template <>
// struct html_element<"select">
// {
// 	static constexpr Fixed_String element{"select"};
// };

// template <>
// struct html_element<"input", "button">
// {
// 	static constexpr Fixed_String element{"input"};
// 	static constexpr Fixed_String type{"button"};
// };

// template <>
// struct html_element<"input", "date">
// {
// 	static constexpr Fixed_String element{"input"};
// 	static constexpr Fixed_String type{"date"};
// };