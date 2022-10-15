#pragma once
#include "Fixed_Array.h"
#include "tuple.h"
#include <array>
#include <concepts>
#include <gphoto2/gphoto2-result.h>
#include <gphoto2/gphoto2-widget.h>

#include <type_traits>

void formater_write(auto &formater, const char *str)
{
	formater.put('\"');
	formater << str;
	formater.put('\"');
}

template <typename T>
requires std::is_arithmetic_v<T>
void formater_write(auto &formater, const T str)
{
	formater << std::to_string(str);
}
void formater_write(auto &formater, const CameraWidgetCallback)
{
	formater << "Camera Call Back";
}

template <std::size_t Size>
void formater_write(auto &output_stream, const Fixed_String<Size> &value)
{
	output_stream << value.data;
}

template <typename PropType>
using WidgetGetter = int(CameraWidget *, PropType *);

template <typename PropType>
using WidgetSetter = int(CameraWidget *, PropType);

using WidgetCountGetter = int(CameraWidget *);

inline void gp_error_check(int val)
{
	if (val != GP_OK)
	{
		perror(gp_result_as_string(val));
	}
}

// const char *gp_port_result_as_string (int result);

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

template <Fixed_String label, typename Type, auto... Args>
void formater_write(auto &output_stream, WidgetProperty<label, Type, Args...> value)
{
	if constexpr (label == std::string_view("readonly"))
	{
		if (value.value == 0)
		{
			output_stream << "w";
		}
		else
		{
			output_stream << "r";
		}
	}
	else if constexpr (label == std::string_view("changed"))
	{
		if (value.value == 0)
		{
			output_stream << "u";
		}
		else
		{
			output_stream << "c";
		}
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

using CommonProperties = std::tuple<WidgetProperty<"label", const char *, gp_widget_get_label>,
									WidgetProperty<"name", const char *, gp_widget_get_name, gp_widget_set_name>,
									WidgetProperty<"info", const char *, gp_widget_get_info, gp_widget_set_info>,
									WidgetProperty<"id", int, gp_widget_get_id>,
									WidgetProperty<"readonly", int, gp_widget_get_readonly, gp_widget_set_readonly>,
									WidgetProperty<"changed", int, gp_widget_changed, gp_widget_set_changed>>;

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

void formater_write(auto &output_stream, const WidgetRange &value)
{
	formater_write(output_stream,value.min);
	output_stream << ",";
	formater_write(output_stream,value.max);
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
			if(i < value.count -1 )
				output_stream << ",";
		}
	}
}

template <typename... T>
struct tuple_cat;

template <typename... Ts1, typename... Ts2>
struct tuple_cat<std::tuple<Ts1...>, std::tuple<Ts2...>>
{
	using type = std::tuple<Ts1..., Ts2...>;
};

// This will be a data store for
template <CameraWidgetType WidgetType>
struct gphoto_widget;

template <CameraWidgetType WidgetT, Fixed_String TypeStr, typename OtherProperties = std::tuple<>>
struct gphoto_widget_common
{
	static constexpr CameraWidgetType WidgetType = WidgetT;
	static constexpr Fixed_String TypeString = TypeStr;
	using Properties = typename tuple_cat<CommonProperties, OtherProperties>::type;
	static constexpr std::size_t prop_count = std::tuple_size_v<Properties>;
	// std::conditional_t<std::is_same_v<OtherProperties, void>, CommonProperties, std::tuple_cat<CommonProperties, OtherProperties>>;
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
	using Widget = gphoto_widget<WidgetType>;

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
			formater << Widget::TypeString;
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
	int ret = gp_widget_get_type(cameraWidget, &widgetType);
	if (ret != GP_OK)
	{
		throw ret;
	}

	auto widgetTypes = gp_widget_getter_tuple();
	std::string output = "Type Not found";
	tupleFunctorSwitch(widgetTypes, _formater, widgetType, cameraWidget);
}

// void formater_write(auto &formater, CameraWidgetType &widgetType)
// {
// }

// std::string to_string(CameraWidgetType widgetType)
// {
// 	auto widgetTypes = gp_widget_tuple();
// 	std::string output = "Type Not found";
// 	tupleFunctorSwitch(widgetTypes, widgetType, output);
// 	return output;
// }
// std::string to_string(char*& )
// {
// 	auto widgetTypes = gp_widget_tuple();
// 	std::string output = "Type Not found";
// 	tupleFunctorSwitch(widgetTypes, widgetType, output);
// 	return output;
// }

// } // namespace std