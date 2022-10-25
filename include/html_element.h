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

	// template< class It, class End >
	// void create_part(html_part_type part_type, It first, End last)
	// {
	// 	parts.emplace(part_type, first, last);
	// }
};

void html_proto_helper(const auto &str, html_parts &html_parts)
{
	// std::vector<html_part> html_parts;
	std::string_view lastMatch(str, 0);

	for (auto [match, variableName] : ctre::range<"\\$\\{(.+?)\\}">(str))
	{
		html_parts.create_part(html_part_type::constant, lastMatch.begin(), match.begin());
		html_parts.create_part(html_part_type::substitutable, variableName.begin(), variableName.end());

		lastMatch = std::string_view(match.end());
	}
	html_parts.create_part(html_part_type::constant, lastMatch);
}

template <typename... ArgsT>
void html_proto_helper(const ArgsT &...Args)
{
	html_parts parts;
	(html_proto_helper(Args, parts), ...);

	int i = 0;
	i++;
}

void html_test()
{
	html_proto_helper("<input type=\"${type}\" id=\"${id}\" name=\"${name}\" ${value}>",
					  "<label for=\"${id}\">${label}</label>");
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
