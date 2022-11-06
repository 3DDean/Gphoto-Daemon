#pragma once
#include "Fixed_Array.h"
#include <ctre.hpp>

struct formatting_error
{};

// https://www.partow.net/programming/hashfunctions/
constexpr uint32_t ELFHash(const char *str, unsigned int length)
{
	unsigned int hash = 0;
	unsigned int x = 0;
	unsigned int i = 0;

	for (i = 0; i < length; ++str, ++i)
	{
		hash = (hash << 4) + (*str);

		if ((x = hash & 0xF0000000L) != 0)
		{
			hash ^= (x >> 24);
		}

		hash &= ~x;
	}

	return hash;
}

// Not using the hash functions because I don't think my approach is really working
template <typename T>
struct hash
{
	using hash_t = uint32_t;

	constexpr hash(T &obj)
		// I think this is just hashing the
		: hash_code(ELFHash((char *)&obj, sizeof(T)))
	{}

	constexpr operator hash_t()
	{
		return hash_code;
	}
	hash_t hash_code;
};

constexpr size_t count_vars(std::string_view _str)
{
	std::size_t count = 0;

	for (auto i = _str.begin(); i < _str.end(); i++)
	{
		if (*i == '$')
		{
			count++;
		}
	}
	return count;
}

enum format_type
{
	not_set,
	String,
	Integer,
	Float
};

struct var_type_pair
{
	constexpr var_type_pair(format_type value, std::string_view _view)
		: enum_value(value), view(_view)
	{}

	format_type enum_value;
	std::string_view view;

	constexpr bool compare(std::string_view _str, format_type &_output)
	{
		if (_str == view)
		{
			_output = enum_value;
			return true;
		}
		return false;
	}
};

constexpr format_type get_type_enum(std::string_view _str)
{
	std::array enum_array = {var_type_pair(format_type::String, "str"), var_type_pair(format_type::Integer, "int"), var_type_pair(format_type::Float, "float")};
	format_type result = not_set;

	for (auto var : enum_array)
		if (var.compare(_str, result))
			break;

	return result;
};

struct format_var
{
	using hash_t = size_t;

	constexpr format_var() {}
	constexpr format_var(const format_var &) = default;

	constexpr format_var(std::string_view str)
	{
		auto regex = ctre::match<"(.+):(.+)">;

		auto [wholeMatch, var] = ctre::match<"\\$\\{(.+)\\}">(str);

		if (var)
		{
			auto [whole, firstCap, secondCap] = regex(var);

			if (whole)
			{
				type = get_type_enum(firstCap);
				label = secondCap;
			}
			else
			{
				type = get_type_enum(std::string_view());
				label = var;
			}
		}
		else
		{
			throw formatting_error{};
		}
	}

	bool operator==(const format_var &rhs) const
	{
		return type == rhs.type && label == rhs.label;
	}

	constexpr format_var(std::string_view _type, std::string_view _label)
		: type(get_type_enum(_type)), label(_label) {}

	format_type type;
	std::string_view label;
};

template <class Compare = std::less<>>
struct format_var_label_compare
{
	constexpr inline bool operator()(const format_var &lhs, const format_var &rhs)
	{
		return Compare{}(lhs.label, rhs.label);
	}
};

#include <variant>
template <typename... Types>
using variant_with_ref = std::variant<Types..., std::reference_wrapper<Types>...>;

struct format_arg
{
	using variant = std::variant<format_var, std::string_view>;
	variant data;

	constexpr format_arg() = default;

	template <typename T, typename... ArgsT>
	inline constexpr auto emplace(ArgsT &&...Args)
	{
		data.emplace<T>(Args...);
		data.index();
	};

	constexpr format_arg &operator=(const format_arg &other)
	{
		data = other.data;
		return *this;
	}

	constexpr operator variant()
	{
		return data;
	}
};

template <typename Key, typename T, std::size_t N, class Compare>
struct fixed_map
{
	using value_type = std::pair<Key, T>;

	using Array = std::array<value_type, N>;
	using iterator = typename Array::iterator;

  private:
	Array data;

	const iterator first = data.begin();
	iterator last = data.begin();

	struct internal_compare
	{
		constexpr inline bool operator()(const value_type &lhs, const Key &rhs) { return Compare{}(lhs.first, rhs); }
	};

  public:
	constexpr size_t size() { return last - data.begin(); }

	constexpr auto begin() { return data.begin(); }
	constexpr auto end() { return last; }

	constexpr auto lower_bound(Key &_str) { return std::lower_bound(begin(), last, _str, internal_compare{}); }

	constexpr void insert(auto hint, Key &key, T value)
	{
		if (hint < data.end() && hint >= data.begin())
		{
			if (hint < last)
			{
				std::move_backward(hint, last, last + 1);
			}
			*hint = value_type{key, value};
			last++;
		}
		else
		{
			throw std::out_of_range("insert hint is not within the range of storage");
		}
	}
};

template <typename Key, std::size_t N, class Compare, typename T = uint32_t>
struct ref_counter : fixed_map<Key, T, N, Compare>
{
	using base = fixed_map<Key, T, N, Compare>;
	using base::end;
	using base::insert;
	using base::lower_bound;
	using typename base::iterator;

	ref_counter(auto &array)
	{
		for (auto var : array)
		{
			iterator itt = lower_bound(var);
			if (itt != end() && var == (*itt).first)
			{
				(*itt).second++;
			}
			else
			{
				insert(itt, var, 1u);
			}
		}
	}
};

template <std::size_t N>
struct formatted_string
{
	std::array<format_arg, N> args;
	formatted_string(const std::string_view _str)
	{}
};

static inline constexpr auto process_arg(format_var &var, auto &&iterator)
{
	if (var.type == format_type::String)
	{
		(*(iterator - 1))++;
		(*(iterator + 1))++;
	}
};

static inline constexpr auto process_arg(std::string_view &var, auto &iterator)
{
	*iterator += var.size();
};

template <size_t N>
struct format_string_data
{
	std::array<format_arg, N> args;
	std::array<size_t, N> arg_str_sizes;
};

template <Fixed_String Str>
constexpr auto format_string()
{
	constexpr size_t arg_count = count_vars(Str) * 2 + 1;
	format_string_data<arg_count> output;

	auto &args = output.args;
	auto &arg_str_sizes = output.arg_str_sizes;

	auto const_start = Str.begin();
	auto arg_itt = args.begin();

	auto set_variable = [&arg_itt, &const_start](const auto start, const auto end)
	{
		for (auto i = start; i < end; i++)
		{
			switch (*i)
			{
			case '$':
			case '{':
				break;
			case '}':
				arg_itt++->template emplace<std::string_view>(const_start, start);
				const_start = i + 1;

				arg_itt++->template emplace<format_var>(std::string_view(start, const_start));
				return i;
			}
		}
		throw formatting_error{};
	};

	for (auto i = Str.begin(); i < Str.end(); i++)
	{
		switch (*i)
		{
		case '$':
			i = set_variable(i, Str.end());
			break;
		case '{':
		case '}':
			throw formatting_error{};
		}
	}

	if (const_start < Str.end())
		arg_itt->template emplace<std::string_view>(const_start, Str.end());

	std::fill(arg_str_sizes.begin(), arg_str_sizes.end(), 0);

	arg_itt = args.begin();
	auto size_itt = arg_str_sizes.begin();

	do
	{
		std::visit<void>([size_itt](auto &&arg)
						 { process_arg(arg, size_itt); },
						 arg_itt->data);

		arg_itt++;
		size_itt++;
	} while (arg_itt < args.end());

	return arg_str_sizes;
}

void format_test()
{
	static constexpr Fixed_String unprocessed_str{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
												  "<label for=${str:id}>${label}</label>"};

	constexpr size_t var_estimate = count_vars(unprocessed_str);

	auto formatted_string = format_string<Fixed_String{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
													   "<label for=${str:id}>${label}</label>"}>();
}
