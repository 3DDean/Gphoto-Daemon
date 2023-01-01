#pragma once
#include "Fixed_Array.h"
#include "format.h"
#include "formater.h"
#include "gphoto-error.h"
#include "tuple.h"
#include <array>
#include <concepts>
#include <functional>
#include <gphoto2/gphoto2-widget.h>
#include <type_traits>

struct unused_t
{};
static constexpr unused_t unused;

template <auto T>
using remove_unused_const = std::conditional_t<std::same_as<decltype(T), const unused_t>, unused_t, decltype(T)>;

template <typename T>
static inline constexpr bool is_unused = std::is_same_v<T, unused_t>;

template <typename T>
concept is_used = !is_unused<T>;

template <Fixed_String Name>
struct identifier
{};

template <Fixed_String Name, typename T>
struct named_type : public T
{
	constexpr named_type() {}
	constexpr named_type(T &obj)
		: T(obj) {}

	constexpr named_type(identifier<Name>, T &obj)
		: T(obj) {}

	template <typename... ArgsT>
	constexpr named_type(identifier<Name>, ArgsT &&...Args)
		: T(Args...)
	{}
};

template <Fixed_String Name, typename T>
named_type(identifier<Name>, T) -> named_type<Name, T>;

template <typename Getter, typename Setter>
struct accessor_traits_base;

template <typename CommonType, typename MemberType, typename R2>
struct accessor_traits_base<MemberType (*)(CommonType), R2 (*)(CommonType, MemberType)>
{
	using parent_type = CommonType;
	using member_type = MemberType;
	using getter_func = std::function<MemberType (*)(CommonType)>;
	using setter_func = std::function<R2(CommonType, MemberType)>;
};

template <typename CommonType, typename MemberType>
struct accessor_traits_base<MemberType (*)(CommonType), unused_t>
{
	using parent_type = CommonType;
	using member_type = MemberType;
	using getter_func = std::function<MemberType(CommonType)>;
	using setter_func = unused_t;
};

template <typename CommonType, typename MemberType, typename R2>
struct accessor_traits_base<unused_t, R2 (*)(CommonType, MemberType)>
{
	using parent_type = CommonType;
	using member_type = MemberType;
	using getter_func = unused_t;
	using setter_func = std::function<R2(CommonType, MemberType)>;
};

template <auto Getter, auto Setter>
struct accessor_base : accessor_traits_base<remove_unused_const<Getter>, remove_unused_const<Setter>>
{
	static constexpr Fixed_String type_str = "accessor";

	using accessor_traits = accessor_traits_base<remove_unused_const<Getter>, remove_unused_const<Setter>>;

	using typename accessor_traits::member_type;
	using typename accessor_traits::parent_type;

	using typename accessor_traits::getter_func;
	using typename accessor_traits::setter_func;

	constexpr void operator()(parent_type base_obj, member_type value) requires(std::invocable<decltype(Setter), parent_type, member_type>)
	{
		Setter(base_obj, value);
	}

	constexpr auto operator()(parent_type base_obj) const requires(std::invocable<decltype(Getter), parent_type>)
	{
		return Getter(base_obj);
	}

	constexpr void set(parent_type base_obj, member_type value) requires(std::invocable<decltype(Setter), parent_type, member_type>)
	{
		Setter(base_obj, value);
	}

	constexpr auto get(parent_type base_obj) const requires(std::invocable<decltype(Getter), parent_type>)
	{
		return Getter(base_obj);
	}
};

template <auto Getter, auto Setter = (const unused_t){}>
struct accessor : public accessor_base<Getter, Setter>
{
	using accessor_base<Getter, Setter>::accessor_base;
};

template <typename SizeAccessor, typename ElementAccessor>
struct array_accessor
{
	using member_type = std::vector<typename ElementAccessor::member_type>;

	named_type<"count", SizeAccessor> size;
	named_type<"elements", ElementAccessor> elements;

	constexpr auto operator()(auto base_obj) const // requires(std::invocable<getter_func, parent_type>)
	{
		std::size_t count = size(base_obj);
		member_type output(size(base_obj));
		for (std::size_t i = 0; i < count; i++)
		{
			output[i] = elements(base_obj, i);
		}
		return output;
	}
};

// Cause of the clang crash
template <typename MemberT, typename AccessorT>
static inline constexpr void check_type(AccessorT)
{
	static_assert(std::is_same_v<typename AccessorT::value_type, MemberT>);
};

template <std::size_t N>
struct identifier_list
{
};

template <typename T>
static consteval inline auto get_consteval(auto accessors)
{
	return std::get<T>(accessors);
}

template <typename... ArgsT>
struct member_accessors
{
	using member_tuple = std::tuple<typename ArgsT::member_type...>;

	static constexpr std::tuple<ArgsT...> accessors{};

	// Setter operator
	// constexpr void operator()(auto base_obj, member_tuple value){}

	constexpr member_tuple operator()(auto base_obj) const
	{
		return get_members(base_obj);
	}

	template <typename parent_type>
	static constexpr inline member_tuple get_members(parent_type base_obj) requires(std::invocable<ArgsT, parent_type> &&...)
	{
		return member_tuple{std::get<ArgsT>(accessors).operator()(base_obj)...};

		// return get_members(base_obj, std::make_index_sequence<sizeof...(ArgsT)>{});
	}

  private:
	template <typename T, T... Index>
	static constexpr inline member_tuple get_members(auto base_obj, std::integer_sequence<T, Index...>)
	{
		return member_tuple{std::get<Index>(accessors).operator()(base_obj)...};
	}
};

template <typename T>
static inline constexpr bool is_gphoto_widget_func = false;

template <typename R, typename... ArgsT>
static inline constexpr bool is_gphoto_widget_func<R (*)(CameraWidget *, ArgsT...)> = true;

template <typename T>
concept gphoto_widget_func = is_gphoto_widget_func<T>;

template <typename T>
concept gphoto_widget_func_or_unused = is_gphoto_widget_func<T> || is_unused<T>;

template <typename MemberType>
struct accessor_traits_base<int (*)(CameraWidget *, MemberType *), unused_t>
{
	using parent_type = CameraWidget *;
	using member_type = MemberType;
	using getter_func = std::function<int (&)(parent_type, MemberType *)>;
	using setter_func = unused_t;
};

template <typename MemberType>
struct accessor_traits_base<int (*)(CameraWidget *, MemberType *), int (*)(CameraWidget *, MemberType)>
{
	using parent_type = CameraWidget *;
	using member_type = MemberType;
	using getter_func = std::function<int(CameraWidget *, MemberType *)>;
	using setter_func = std::function<int(CameraWidget *, MemberType)>;
};

template <typename MemberType>
struct accessor_traits_base<int (*)(CameraWidget *, int, MemberType *), unused_t>
{
	using parent_type = CameraWidget *;
	using member_type = MemberType;
	using index_type = int;
	using getter_func = std::function<int(CameraWidget *, int, MemberType *)>;
	using setter_func = unused_t;
};

// GPhoto2 widget accessors
template <gphoto_widget_func auto Getter, gphoto_widget_func_or_unused auto Setter> // requires gphoto_widget_func<Getter> && gphoto_widget_func_or_unused<Setter>
struct accessor<Getter, Setter> : accessor_base<Getter, Setter>
{
	using base = accessor_base<Getter, Setter>;

	using typename base::getter_func;
	using typename base::member_type;
	using typename base::setter_func;

	using base::operator();

	// I do not know if setter works
	template <typename T>
	requires(std::invocable<setter_func, CameraWidget *, T>) inline constexpr void operator()(CameraWidget *parent, T value)
	{
		gp_error_check(Setter(parent, value));
	}

	inline constexpr member_type operator()(CameraWidget *parent) const
		requires(std::invocable<decltype(Getter), CameraWidget *, member_type *>)
	{
		member_type value;
		gp_error_check(Getter(parent, &value));
		return value;
	}

	inline constexpr member_type operator()(CameraWidget *parent, int index) const
		requires(std::invocable<decltype(Getter), CameraWidget *, int, member_type *>)
	{
		member_type value;
		gp_error_check(Getter(parent, index, &value));
		return value;
	}
};

struct NoAttributes
{
	void operator()(auto &output, CameraWidget *widget, uint32_t)
	{
	}
};

struct RangeAttributes
{
	using formatter = format_string<"${int:min}, ${int:max}, ${int:increment}">;
	static constexpr Fixed_String label = "range";

	void operator()(auto &output, CameraWidget *widget, uint32_t indent)
	{
	for (std::size_t i = 0; i < indent; i++)
		output << " ";

		float min;
		float max;
		float increment;

		gp_error_check(gp_widget_set_range(widget, min, max, increment));

		formatter format;
		format(output, min, max, increment);
	}
};
//TODO Add in a hash checker
struct ChoicesAttributes
{
	using const_str = const char *;
	static constexpr Fixed_String label = "choices";

	void operator()(auto &output, CameraWidget *widget, uint32_t indent)
	{
		for (std::size_t i = 0; i < indent; i++)
			output << " ";
		std::size_t count = gp_widget_count_choices(widget);

		const_str choice;
		gp_widget_get_choice(widget, 0, &choice);
		output << "\"" << choice << "\"";
		for (std::size_t i = 1; i < count; i++)
		{
			gp_widget_get_choice(widget, i, &choice);
			output << ", \"" << choice << "\"";
		}
		output << "\n";
	}
};

template <typename T>
using WidgetValueProperty = T; //"value", T, gp_widget_get_value, gp_widget_set_value>;

template <CameraWidgetType WidgetT, Fixed_String TypeStr, typename OtherProperties = std::tuple<>>
struct gphoto_widget_common
{};

#include "html_element.h"

void iterate_choices(CameraWidget *widget, auto func)
{
	std::size_t count = gp_widget_count_choices(widget);

	if (count != 0)
	{
		using const_str = const char *;

		const_str *str;
		for (std::size_t i = 0; i < count; i++)
		{
			gp_widget_get_choice(widget, i, str);
			func(i, str);
		}
	}
}

template <CameraWidgetType>
struct gphoto_widget
{};

/**< \brief Window widget*   This is the toplevel configuration widget. It should likely contain multiple #GP_WIDGET_SECTION entries. */
template <>
struct gphoto_widget<GP_WIDGET_WINDOW>
{
	static constexpr Fixed_String type_str = make_fixed_string<"window">();
	;
	using value_type = void;
	using attributes_accessor = NoAttributes;

	// <div class=${str:class} id=${str:id} name=${str:name}>${array:members}</div>"
	static constexpr Fixed_String htmlEquivalent = "tab_book";
};
/**< \brief Section widget (think Tab) */
template <>
struct gphoto_widget<GP_WIDGET_SECTION>
{
	static constexpr Fixed_String type_str = make_fixed_string<"section">();
	;
	using value_type = void;
	using attributes_accessor = NoAttributes;

	static constexpr Fixed_String htmlEquivalent = "tab_page";
};
/**< \brief Text widget. */
template <>
struct gphoto_widget<GP_WIDGET_TEXT>
{
	static constexpr Fixed_String type_str = make_fixed_string<"text">();
	;
	using value_type = char *;
	using attributes_accessor = NoAttributes;
};
/**< \brief Slider widget. */
template <>
struct gphoto_widget<GP_WIDGET_RANGE>
{
	static constexpr Fixed_String type_str = make_fixed_string<"range">();
	;
	using value_type = float;
	using attributes_accessor = RangeAttributes;
};

/**< \brief Toggle widget (think check box) */
template <>
struct gphoto_widget<GP_WIDGET_TOGGLE>
{
	static constexpr Fixed_String type_str = make_fixed_string<"toggle">();
	;
	using value_type = int;
	using attributes_accessor = NoAttributes;
};

/**< \brief Radio button widget. */
template <>
struct gphoto_widget<GP_WIDGET_RADIO>
{
	static constexpr Fixed_String type_str = make_fixed_string<"radio">();
	;
	using value_type = char *;
	using attributes_accessor = ChoicesAttributes;
};

/**< \brief Menu widget (same as RADIO). */
template <>
struct gphoto_widget<GP_WIDGET_MENU>
{
	static constexpr Fixed_String type_str = make_fixed_string<"menu">();
	;
	using value_type = char *;
	using attributes_accessor = ChoicesAttributes;
};
/**< \brief Button press widget. */
template <>
struct gphoto_widget<GP_WIDGET_BUTTON>
{
	static constexpr Fixed_String type_str = make_fixed_string<"button">();
	;
	using value_type = void;
	using attributes_accessor = NoAttributes;
};
/**< \brief Date entering widget. */
template <>
struct gphoto_widget<GP_WIDGET_DATE>
{
	static constexpr Fixed_String type_str = make_fixed_string<"date">();
	;
	using value_type = int;
	using attributes_accessor = NoAttributes;
};

auto get_widget_type(CameraWidget *cameraWidget)
{
	CameraWidgetType widgetType;
	std::string_view type_str;
	gp_widget_get_type(cameraWidget, &widgetType);

	switch (widgetType)
	{
	case GP_WIDGET_WINDOW:
		type_str = gphoto_widget<GP_WIDGET_WINDOW>::type_str;
		break;
	case GP_WIDGET_SECTION:
		type_str = gphoto_widget<GP_WIDGET_SECTION>::type_str;
		break;
	case GP_WIDGET_TEXT:
		type_str = gphoto_widget<GP_WIDGET_TEXT>::type_str;
		break;
	case GP_WIDGET_RANGE:
		type_str = gphoto_widget<GP_WIDGET_RANGE>::type_str;
		break;
	case GP_WIDGET_TOGGLE:
		type_str = gphoto_widget<GP_WIDGET_TOGGLE>::type_str;
		break;
	case GP_WIDGET_RADIO:
		type_str = gphoto_widget<GP_WIDGET_RADIO>::type_str;
		break;
	case GP_WIDGET_MENU:
		type_str = gphoto_widget<GP_WIDGET_MENU>::type_str;
		break;
	case GP_WIDGET_BUTTON:
		type_str = gphoto_widget<GP_WIDGET_BUTTON>::type_str;
		break;
	case GP_WIDGET_DATE:
		type_str = gphoto_widget<GP_WIDGET_DATE>::type_str;
		break;
	}

	return std::pair<CameraWidgetType, std::string_view>(widgetType, type_str);
}

//Stupid solution, I will need to somehow merge this with get_widget_type, probably with a dedicated formatter class
//custom formatter type for the formatter that selects each the appropiate widget
//So each Widget_Type has it's own class and is called by the appropriate formatter
auto get_widget_options(auto& output, CameraWidget *cameraWidget, uint32_t indent)
{
	CameraWidgetType widgetType;
	gp_widget_get_type(cameraWidget, &widgetType);

	switch (widgetType)
	{
	case GP_WIDGET_WINDOW:
		gphoto_widget<GP_WIDGET_WINDOW>::attributes_accessor()(output, cameraWidget, indent);
		break;
	case GP_WIDGET_SECTION:
		gphoto_widget<GP_WIDGET_SECTION>::attributes_accessor()(output, cameraWidget, indent);
		break;
	case GP_WIDGET_TEXT:
		gphoto_widget<GP_WIDGET_TEXT>::attributes_accessor()(output, cameraWidget, indent);
		break;
	case GP_WIDGET_RANGE:
		gphoto_widget<GP_WIDGET_RANGE>::attributes_accessor()(output, cameraWidget, indent);
		break;
	case GP_WIDGET_TOGGLE:
		gphoto_widget<GP_WIDGET_TOGGLE>::attributes_accessor()(output, cameraWidget, indent);
		break;
	case GP_WIDGET_RADIO:
		gphoto_widget<GP_WIDGET_RADIO>::attributes_accessor()(output, cameraWidget, indent);
		break;
	case GP_WIDGET_MENU:
		gphoto_widget<GP_WIDGET_MENU>::attributes_accessor()(output, cameraWidget, indent);
		break;
	case GP_WIDGET_BUTTON:
		gphoto_widget<GP_WIDGET_BUTTON>::attributes_accessor()(output, cameraWidget, indent);
		break;
	case GP_WIDGET_DATE:
		gphoto_widget<GP_WIDGET_DATE>::attributes_accessor()(output, cameraWidget, indent);
		break;
	}
}

using gwidget_label = accessor<gp_widget_get_label>;
using gwidget_name = accessor<gp_widget_get_name, gp_widget_set_name>;
using gwidget_info = accessor<gp_widget_get_info, gp_widget_set_info>;

using gwidget_id = accessor<gp_widget_get_id>;

using gwidget_readonly = accessor<gp_widget_get_readonly, gp_widget_set_readonly>;
using gwidget_changed = accessor<gp_widget_changed, gp_widget_set_changed>;

using gwidget_widget_type = accessor<gp_widget_get_type>;

using gwidget_child_array = array_accessor<accessor<gp_widget_count_children>, accessor<gp_widget_get_child>>;

using widget_formatter = format_string<"${str:label}, ${str:name}, ${str:tooltip}, ${int:id}, ${bool:readonly}, ${bool:changed}, ${type}">;
