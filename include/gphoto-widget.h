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
using widget_type = unused;

template <typename T>
struct widget_accessor
{
	static constexpr bool value = false;
};

template <typename T>
struct widget_accessor<int(CameraWidget *, T)>
{
	using member_type = T;
	static constexpr bool value = true;
};
template <typename T>
struct widget_accessor<int(CameraWidget *, T *)>
{
	using member_type = T;
	static constexpr bool value = true;
};
template <typename T>
struct widget_accessor<int(CameraWidget *, T **)>
{
	using member_type = T*;
	static constexpr bool value = true;
};
template <>
struct widget_accessor<int(CameraWidget *)>
{
	using member_type = int;
	static constexpr bool value = true;
};

template <typename T>
static inline constexpr bool is_widget_accessor = widget_accessor<T>::value;

template <Fixed_String Name>
struct identifier
{};

template <Fixed_String Name, typename Getter, typename Setter = unused>
struct accessor_base
{
	static constexpr Fixed_String type_str = "accessor";
	static constexpr Fixed_String name_str = Name;

	using getter_func = std::conditional_t<std::is_function_v<Getter>, std::function<Getter>, unused>;
	using setter_func = std::conditional_t<std::is_function_v<Setter>, std::function<Setter>, unused>;

	constexpr accessor_base(identifier<Name>, Getter *getter)
		: getter(getter) {}
	constexpr accessor_base(identifier<Name>, unused, Setter *setter)
		: setter(setter) {}
	constexpr accessor_base(identifier<Name>, Getter *getter, Setter *setter)
		: getter(getter), setter(setter) {}

	const getter_func getter;
	const setter_func setter;

	template <typename ParentT, typename T>
	requires(std::invocable<setter_func, ParentT, T>) constexpr void operator()(ParentT base_obj, T value)
	{
		setter(base_obj, value);
	}

	template <typename ParentT>
	requires(std::invocable<getter_func, ParentT>) constexpr auto operator()(ParentT base_obj) const
	{
		return getter(base_obj);
	}
};

template <Fixed_String Name, typename Getter, typename Setter = unused>
struct accessor : public accessor_base<Name, Getter, Setter>
{
	using accessor_base<Name, Getter, Setter>::accessor_base;
};

template <Fixed_String Name, typename Getter>
accessor(identifier<Name>, Getter *getter) -> accessor<Name, Getter>;

template <Fixed_String Name, typename Setter>
accessor(identifier<Name>, unused, Setter *setter) -> accessor<Name, unused, Setter>;

template <Fixed_String Name, typename Getter, typename Setter>
accessor(identifier<Name>, Getter *getter, Setter *setter) -> accessor<Name, Getter, Setter>;

// GPhoto2 widget accessors
template <Fixed_String Name, typename Getter, typename Setter>
requires(is_widget_accessor<Getter> || is_widget_accessor<Setter>) struct accessor<Name, Getter, Setter> : accessor_base<Name, Getter, Setter>
{
	using base = accessor_base<Name, Getter, Setter>;
	using base::accessor_base;

	using base::getter;
	using base::setter;

	using typename base::getter_func;
	using typename base::setter_func;

	using value_type = typename widget_accessor<Getter>::member_type;

	using base::operator();

	//I do not know if setter works
	template <typename T>
	requires(std::invocable<setter_func, CameraWidget *, T>) inline constexpr void operator()(CameraWidget *parent, T value)
	{
		gp_error_check(setter(parent, value));
	}

	inline constexpr value_type operator()(CameraWidget *parent) const
	requires(std::invocable<getter_func, CameraWidget *, value_type*>)
	{
		value_type value;
		gp_error_check(getter(parent, &value));
		return value;
	}
};

template <typename MemberT, typename AccessorT>
static inline constexpr void check_type(AccessorT)
{
	static_assert(std::is_same_v<typename AccessorT::value_type, MemberT>);
};

auto widget_base_members = std::make_tuple(accessor(identifier<"label">(), gp_widget_get_label),
										   accessor(identifier<"name">(), gp_widget_get_name, gp_widget_set_name),
										   accessor(identifier<"info">(), gp_widget_get_info, gp_widget_set_info),
										   accessor(identifier<"id">(), gp_widget_get_id),
										   accessor(identifier<"readonly">(), gp_widget_get_readonly, gp_widget_set_readonly),
										   accessor(identifier<"changed">(), gp_widget_changed, gp_widget_set_changed));

template <typename PropType>
using WidgetGetter = int(CameraWidget *, PropType *);

template <typename PropType>
using WidgetSetter = int(CameraWidget *, PropType);

using WidgetCountGetter = int(CameraWidget *);

template <Fixed_String label, typename Type, auto Getter, auto... Args>
struct WidgetProperty;

template <Fixed_String Label, typename Type, WidgetGetter<Type> Getter>
struct WidgetProperty<Label, Type, Getter>
{
	static constexpr Fixed_String label = Label;

	WidgetProperty() {}

	void get(CameraWidget *widget)
	{
		gp_error_check(Getter(widget, &value));
	}

	Type value;
};

template <Fixed_String Label, typename Type, WidgetGetter<Type> Getter, WidgetSetter<const Type> Setter>
struct WidgetProperty<Label, Type, Getter, Setter>
{
	static constexpr Fixed_String label = Label;

	WidgetProperty() {}

	void get(CameraWidget *widget)
	{
		gp_error_check(Getter(widget, &value));
	}
	void set(CameraWidget *widget, Type val)
	{
		value = val;
		gp_error_check(Setter(widget, val));
	}

	Type value;
};

template <Fixed_String Label, typename Type>
struct WidgetProperty<Label, Type, gp_widget_get_value, gp_widget_set_value>
{
	static constexpr Fixed_String label = Label;

	WidgetProperty() {}

	void get(CameraWidget *widget)
	{
		gp_error_check(gp_widget_get_value(widget, (void **)&value));
	}
	void set(CameraWidget *widget, Type val)
	{
		value = val;
		gp_error_check(gp_widget_set_value(widget, (const void *)&val));
	}

	Type value;
};

template <Fixed_String Label, WidgetCountGetter Getter, auto Setter>
struct WidgetProperty<Label, int, Getter, Setter>
{
	static constexpr Fixed_String label = Label;
	WidgetProperty() {}

	void get(CameraWidget *widget)
	{
		value = Getter(widget);
	}
	void set(CameraWidget *widget, int val)
	{
		value = val;
		gp_error_check(Setter(widget, val));
	}

	int value;
};

template <typename T>
using WidgetValueProperty = WidgetProperty<"value", T, gp_widget_get_value, gp_widget_set_value>;

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

using CommonProperties = std::tuple<WidgetProperty<"label", const char *, gp_widget_get_label>,
									WidgetProperty<"name", const char *, gp_widget_get_name, gp_widget_set_name>,
									WidgetProperty<"info", const char *, gp_widget_get_info, gp_widget_set_info>,
									WidgetProperty<"id", int, gp_widget_get_id>,
									WidgetProperty<"readonly", int, gp_widget_get_readonly, gp_widget_set_readonly>,
									WidgetProperty<"changed", int, gp_widget_changed, gp_widget_set_changed>>;

template <CameraWidgetType WidgetT, Fixed_String TypeStr, typename OtherProperties = std::tuple<>>
struct gphoto_widget_common
{
	static constexpr CameraWidgetType WidgetType = WidgetT;
	static constexpr Fixed_String TypeString = TypeStr;
	using Properties = typename tuple_cat<CommonProperties, OtherProperties>::type;
	static constexpr std::size_t prop_count = std::tuple_size_v<Properties>;
	using type = gphoto_widget_common;

	Properties properties;

	template <typename T, T... ints>
	gphoto_widget_common(CameraWidget *widget, std::integer_sequence<T, ints...>)
	{
		(std::get<ints>(properties).get(widget), ...);
	};

	gphoto_widget_common(CameraWidget *widget)
		: gphoto_widget_common(widget, std::make_index_sequence<prop_count>{}){};
};

// static constexpr std::array AvailableWidgets = {GP_WIDGET_WINDOW, GP_WIDGET_SECTION, GP_WIDGET_TEXT, GP_WIDGET_RANGE, GP_WIDGET_TOGGLE, GP_WIDGET_RADIO, GP_WIDGET_MENU, GP_WIDGET_BUTTON, GP_WIDGET_DATE};
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

template <CameraWidgetType WidgetType>
struct gphoto_widget_type_comparitor
{
	using Widget = typename gphoto_widget<WidgetType>::type;

	constexpr bool operator()(auto &formater, CameraWidgetType widgetType, CameraWidget *cameraWidget)
	{
		bool result = Widget::WidgetType == widgetType;
		if (result)
		{
			output(formater, Widget(cameraWidget));
		}
		return result;
	}

	constexpr void output(auto &formater, Widget widget)
	{
		formater << Widget::TypeString;
		ouput(formater, widget.properties);
	}

	template <std::size_t I = 0, typename T>
	constexpr void ouput(auto &formater, T &properties)
	{
		auto &currentProp = std::get<I>(properties);
		using CurrentPropT = std::decay_t<decltype(currentProp)>;
		formater << currentProp;

		constexpr auto currentLabel = CurrentPropT::label;

		if constexpr (currentLabel == std::string_view("info"))
		{
		}

		if constexpr (I + 1 < Widget::prop_count)
		{
			ouput<I + 1>(formater, properties);
		}
	}
};

using gp_widget_getter_tuple = std::tuple<gphoto_widget_type_comparitor<GP_WIDGET_WINDOW>,
										  gphoto_widget_type_comparitor<GP_WIDGET_SECTION>,
										  gphoto_widget_type_comparitor<GP_WIDGET_TEXT>,
										  gphoto_widget_type_comparitor<GP_WIDGET_RANGE>,
										  gphoto_widget_type_comparitor<GP_WIDGET_TOGGLE>,
										  gphoto_widget_type_comparitor<GP_WIDGET_RADIO>,
										  gphoto_widget_type_comparitor<GP_WIDGET_MENU>,
										  gphoto_widget_type_comparitor<GP_WIDGET_BUTTON>,
										  gphoto_widget_type_comparitor<GP_WIDGET_DATE>>;

void gp_get_widget_type_and_data(auto &_formater, CameraWidget *cameraWidget)
{
	CameraWidgetType widgetType;
	gp_error_check(gp_widget_get_type(cameraWidget, &widgetType));

	auto widgetTypes = gp_widget_getter_tuple();
	tupleFunctorSwitch(widgetTypes, _formater, widgetType, cameraWidget);
}

template <Fixed_String label, typename Type, auto... Args>
void formater_write(auto &output_stream, WidgetProperty<label, Type, Args...> value)
{
	if constexpr (label == std::string_view("readonly"))
	{
		if (value.value == 0)
			output_stream << "w";
		else
			output_stream << "r";
	}
	else if constexpr (label == std::string_view("changed"))
	{
		if (value.value == 0)
			output_stream << "u";
		else
			output_stream << "c";
	}
	else if constexpr (label == std::string_view("value"))
	{
		formater_write(output_stream, value.value);
	}
	else
	{
		formater_write(output_stream, value.value);
	}
}
void formater_write(auto &formater, const CameraWidgetCallback)
{
	formater << "Camera Call Back";
}

void formater_write(auto &output_stream, const WidgetRange &value)
{
	formater_write(output_stream, value.min);
	output_stream << ",";
	formater_write(output_stream, value.max);
	output_stream << ",";
	formater_write(output_stream, value.increment);
}

void formater_write(auto &output_stream, const WidgetChoices &value)
{
	if (value.count != 0)
	{
		for (std::size_t i = 0; i < value.count; i++)
		{
			formater_write(output_stream, value.choices[i]);
			if (i < value.count - 1)
				output_stream << ",";
		}
	}
}