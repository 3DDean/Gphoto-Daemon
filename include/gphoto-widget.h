#pragma once
#include "Fixed_Array.h"
#include "formater.h"
#include "gphoto-error.h"
#include "tuple.h"
#include <array>
#include <concepts>
#include <functional>
#include <gphoto2/gphoto2-widget.h>
#include <type_traits>

struct unused
{};

template <typename T>
static inline constexpr bool is_unused = std::is_same_v<T, unused>;

template <typename T>
concept is_used = !is_unused<T>;

template <Fixed_String Name>
struct identifier
{};

template <Fixed_String Name, typename T>
struct named_type : public T
{
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

template <Fixed_String Name>
struct struct_name
{};

template <typename Getter, typename Setter>
struct accessor_traits_base;

template <typename CommonType, typename MemberType, typename R2>
struct accessor_traits_base<MemberType(CommonType), R2(CommonType, MemberType)>
{
	using parent_type = CommonType;
	using member_type = MemberType;
	using getter_func = std::function<MemberType(CommonType)>;
	using setter_func = std::function<R2(CommonType, MemberType)>;
};

template <typename CommonType, typename MemberType>
struct accessor_traits_base<MemberType(CommonType), unused>
{
	using parent_type = CommonType;
	using member_type = MemberType;
	using getter_func = std::function<MemberType(CommonType)>;
	using setter_func = unused;
};

template <typename CommonType, typename MemberType, typename R2>
struct accessor_traits_base<unused, R2(CommonType, MemberType)>
{
	using parent_type = CommonType;
	using member_type = MemberType;
	using getter_func = unused;
	using setter_func = std::function<R2(CommonType, MemberType)>;
};

template <typename Getter, typename Setter = unused>
struct accessor_base : accessor_traits_base<Getter, Setter>
{
	static constexpr Fixed_String type_str = "accessor";

	using accessor_traits = accessor_traits_base<Getter, Setter>;

	using typename accessor_traits::member_type;
	using typename accessor_traits::parent_type;

	using typename accessor_traits::getter_func;
	using typename accessor_traits::setter_func;

	constexpr accessor_base(Getter *getter)
		: getter(getter) {}
	constexpr accessor_base(unused, Setter *setter)
		: setter(setter) {}
	constexpr accessor_base(Getter *getter, Setter *setter)
		: getter(getter), setter(setter) {}

	const getter_func getter;
	const setter_func setter;

	constexpr void operator()(parent_type base_obj, member_type value) requires(std::invocable<setter_func, parent_type, member_type>)
	{
		setter(base_obj, value);
	}

	constexpr auto operator()(parent_type base_obj) const requires(std::invocable<getter_func, parent_type>)
	{
		return getter(base_obj);
	}
};

template <typename Getter, typename Setter = unused>
struct accessor : public accessor_base<Getter, Setter>
{
	using accessor_base<Getter, Setter>::accessor_base;
};

template <typename Getter>
accessor(Getter *getter) -> accessor<Getter>;

template <typename Setter>
accessor(unused, Setter *setter) -> accessor<unused, Setter>;

template <typename Getter, typename Setter>
accessor(Getter *getter, Setter *setter) -> accessor<Getter, Setter>;

template <typename MemberT, typename AccessorT>
static inline constexpr void check_type(AccessorT)
{
	static_assert(std::is_same_v<typename AccessorT::value_type, MemberT>);
};

template <typename... ArgsT>
struct member_list
{
	static constexpr std::array<std::string_view, sizeof...(ArgsT)> member_names{ArgsT::name_str...};
	using member_tuple = std::tuple<typename ArgsT::member_type...>;

	std::tuple<ArgsT...> accessors;

	constexpr member_list(ArgsT... Args)
		: accessors(Args...)
	{}


	// Setter operator
	// constexpr void operator()(auto base_obj, member_tuple value){}

	constexpr member_tuple operator()(auto base_obj) const
	{
		return get_members(base_obj);
	}

	template <typename parent_type>
	constexpr member_tuple get_members(parent_type base_obj) requires(std::invocable<ArgsT, parent_type> &&...)
	{
		return get_members(base_obj, std::make_index_sequence<sizeof...(ArgsT)>());
	}

  private:
	template <typename T, T... index>
	constexpr member_tuple get_members(auto base_obj, std::integer_sequence<T, index...>)
	{
		return member_tuple{(std::get<index>(accessors)).operator()(base_obj)...};
	}
};

template <typename T>
static inline constexpr bool is_gphoto_widget_func = false;

template <typename R, typename... ArgsT>
static inline constexpr bool is_gphoto_widget_func<R(CameraWidget *, ArgsT...)> = true;

template <typename T>
concept gphoto_widget_func = is_gphoto_widget_func<T>;

template <typename T>
concept gphoto_widget_func_or_unused = is_gphoto_widget_func<T> || is_unused<T>;

template <typename MemberType>
struct accessor_traits_base<int(CameraWidget *, MemberType *), unused>
{
	using parent_type = CameraWidget *;
	using member_type = MemberType;
	using getter_func = std::function<int(parent_type, MemberType *)>;
	using setter_func = unused;
};

template <typename MemberType>
struct accessor_traits_base<int(CameraWidget *, MemberType *), int(CameraWidget *, MemberType)>
{
	using parent_type = CameraWidget *;
	using member_type = MemberType;
	using getter_func = std::function<int(CameraWidget *, MemberType *)>;
	using setter_func = std::function<int(CameraWidget *, MemberType)>;
};

template <typename MemberType>
struct accessor_traits_base<int(CameraWidget *, int, MemberType *), unused>
{
	using parent_type = CameraWidget *;
	using member_type = MemberType;
	using getter_func = std::function<int(CameraWidget *, int, MemberType *)>;
	using setter_func = unused;
};

// GPhoto2 widget accessors
template <gphoto_widget_func Getter, gphoto_widget_func_or_unused Setter>
struct accessor<Getter, Setter> : accessor_base<Getter, Setter>
{
	using base = accessor_base<Getter, Setter>;
	using base::accessor_base;

	using base::getter;
	using base::setter;

	using typename base::getter_func;
	using typename base::member_type;
	using typename base::setter_func;

	using base::operator();

	// I do not know if setter works
	template <typename T>
	requires(std::invocable<setter_func, CameraWidget *, T>) inline constexpr void operator()(CameraWidget *parent, T value)
	{
		gp_error_check(setter(parent, value));
	}

	inline constexpr member_type operator()(CameraWidget *parent) const
		requires(std::invocable<getter_func, CameraWidget *, member_type *>)
	{
		member_type value;
		gp_error_check(getter(parent, &value));
		return value;
	}
};


struct WidgetRange
{
	static constexpr Fixed_String label = "range";

	void get(CameraWidget *widget)
	{
		gp_error_check(gp_widget_get_range(widget, &min, &max, &increment));
	}
	void set(CameraWidget *widget, float low, float high, float incr)
	{
		min = low;
		max = high;
		increment = incr;

		gp_error_check(gp_widget_set_range(widget, low, high, incr));
	}

	float min;
	float max;
	float increment;
};

struct WidgetChoices
{
	using const_str = const char *;
	static constexpr Fixed_String label = "choices";

	void get(CameraWidget *widget)
	{
		std::size_t newCount = gp_widget_count_choices(widget);

		if (choices != nullptr && newCount != count)
		{
			delete choices;
		}
		count = newCount;
		if (count != 0)
		{
			choices = new const_str[count];
			for (std::size_t i = 0; i < count; i++)
			{
				gp_widget_get_choice(widget, i, &choices[i]);
			}
		}
		else
		{
			choices = nullptr;
		}
	}

	std::size_t count;
	const_str *choices = nullptr;
};

template <typename T>
using WidgetValueProperty = T;//"value", T, gp_widget_get_value, gp_widget_set_value>;

template <CameraWidgetType WidgetT, Fixed_String TypeStr, typename OtherProperties = std::tuple<>>
struct gphoto_widget_common{};

template <CameraWidgetType WidgetType>
struct gphoto_widget;
/**< \brief Window widget*   This is the toplevel configuration widget. It should likely contain multiple #GP_WIDGET_SECTION entries. */
template <>
struct gphoto_widget<GP_WIDGET_WINDOW> :
	gphoto_widget_common<GP_WIDGET_WINDOW, "window">
{
};
/**< \brief Section widget (think Tab) */
template <>
struct gphoto_widget<GP_WIDGET_SECTION> :
	gphoto_widget_common<GP_WIDGET_SECTION, "section">
{
};
/**< \brief Text widget. */
template <>
struct gphoto_widget<GP_WIDGET_TEXT> :
	gphoto_widget_common<GP_WIDGET_TEXT, "text", std::tuple<WidgetValueProperty<char *>>>
{
};
/**< \brief Slider widget. */
template <>
struct gphoto_widget<GP_WIDGET_RANGE> :
	gphoto_widget_common<GP_WIDGET_RANGE, "range", std::tuple<WidgetValueProperty<float>, WidgetRange>>
{
	// gp_widget_get_range
};
/**< \brief Toggle widget (think check box) */
template <>
struct gphoto_widget<GP_WIDGET_TOGGLE> :
	gphoto_widget_common<GP_WIDGET_TOGGLE, "toggle", std::tuple<WidgetValueProperty<int>>>
{
};
/**< \brief Radio button widget. */
template <>
struct gphoto_widget<GP_WIDGET_RADIO> :
	gphoto_widget_common<GP_WIDGET_RADIO, "radio", std::tuple<WidgetValueProperty<char *>, WidgetChoices>>
{
	// gp_widget_count_choices
};
/**< \brief Menu widget (same as RADIO). */
template <>
struct gphoto_widget<GP_WIDGET_MENU> :
	gphoto_widget_common<GP_WIDGET_MENU, "menu", std::tuple<WidgetValueProperty<char *>, WidgetChoices>>
{
	// gp_widget_count_choices
};
/**< \brief Button press widget. */
template <>
struct gphoto_widget<GP_WIDGET_BUTTON> :
	gphoto_widget_common<GP_WIDGET_BUTTON, "button", std::tuple<WidgetValueProperty<CameraWidgetCallback>>>
{
};
/**< \brief Date entering widget. */
template <>
struct gphoto_widget<GP_WIDGET_DATE> :
	gphoto_widget_common<GP_WIDGET_DATE, "date", std::tuple<WidgetValueProperty<int>>>
{
};

static inline constexpr std::array gp_widget_type_enum{
	GP_WIDGET_WINDOW,
	GP_WIDGET_SECTION,
	GP_WIDGET_TEXT,
	GP_WIDGET_RANGE,
	GP_WIDGET_TOGGLE,
	GP_WIDGET_RADIO,
	GP_WIDGET_MENU,
	GP_WIDGET_BUTTON,
	GP_WIDGET_DATE};

auto widget_base_members = member_list(named_type(identifier<"label">(), accessor(gp_widget_get_label)),
									   named_type(identifier<"name">(), accessor(gp_widget_get_name, gp_widget_set_name)),
									   named_type(identifier<"info">(), accessor(gp_widget_get_info, gp_widget_set_info)),
									   named_type(identifier<"id">(), accessor(gp_widget_get_id)),
									   named_type(identifier<"readonly">(), accessor(gp_widget_get_readonly, gp_widget_set_readonly)),
									   named_type(identifier<"changed">(), accessor(gp_widget_changed, gp_widget_set_changed)),
									   named_type(identifier<"widget_type">(), accessor(gp_widget_get_type)));

using CommonPropertiesTypes = std::tuple<named_type<"label", const char *>,
										 named_type<"name", const char *>,
										 named_type<"info", const char *>,
										 named_type<"id", int>,
										 named_type<"readonly", int>,
										 named_type<"changed", int>>;