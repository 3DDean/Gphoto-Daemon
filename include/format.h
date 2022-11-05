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

	constexpr format_var(const format_var &) = default;

	constexpr format_var() {}

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
				// format_type::not_set;
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
	using var_variant = variant_with_ref<format_var, std::string_view>;
	variant_with_ref<format_var, std::string_view> data;

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

template <Fixed_String Str>
inline constexpr auto format_string()
{
	constexpr size_t var_estimate = count_vars(Str);
	std::array<format_arg, var_estimate + var_estimate + 1> args;
	
	{
		std::array<std::string_view, var_estimate> vars;
		auto var_itt = vars.begin();
		
		auto set_variable = [&var_itt](const auto start, const auto end)
		{
			// auto start = _str.begin();

			for (auto i = start; i < end; i++)
			{
				switch (*i)
				{
				case '$':
				case '{':
					break;
				case '}':
					*var_itt = std::string_view(start, i + 1);
					var_itt++;
					return i;
				}
			}
			throw formatting_error{};
		};

		std::string_view str(Str);
		for (auto i = str.begin(); i < str.end(); i++)
		{
			switch (*i)
			{
			case '$':
				i = set_variable(i, str.end());
				break;
			case '{':
			case '}':
				throw formatting_error{};
			}
		}
		auto output_itt = args.begin();
		auto constantStart = str.begin();

		for (auto var : vars)
		{
			output_itt->template emplace<std::string_view>(constantStart, var.begin());
			output_itt++;
			output_itt->template emplace<format_var>(var);
			output_itt++;

			constantStart = var.end();
		}
		if (constantStart < str.end())
		{
			output_itt->template emplace<std::string_view>(constantStart, str.end());
		}
	}

	return args;
};

void format_test()
{
	static constexpr Fixed_String unprocessed_str{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
												  "<label for=${str:id}>${label}</label>"};

	constexpr size_t var_estimate = count_vars(unprocessed_str);

	auto formatted_string = format_string<Fixed_String{"<input type=${str:type} id=${str:id} name=${str:name} ${value}>",
																 "<label for=${str:id}>${label}</label>"}>();
}
