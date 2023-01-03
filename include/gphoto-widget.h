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
#include <sstream>
#include <type_traits>

struct unused_t
{};
template <auto>
struct function_traits;

template <typename ReturnType, typename... ArgsT, ReturnType (*func)(ArgsT...)>
struct function_traits<func>
{
	using return_type = ReturnType;
	using args = std::tuple<ArgsT...>;
	using function_type = std::function<ReturnType(ArgsT...)>;
};

static constexpr unused_t unused;

template <auto Func>
auto getter(CameraWidget *cameraWidget)
{
	using function = function_traits<Func>;
	using return_type = std::remove_pointer_t<std::tuple_element_t<1, typename function::args>>;
	return_type return_object;
	gp_error_check(Func(cameraWidget, &return_object));
	return return_object;
}

template <auto Func, typename T>
void setter(CameraWidget *cameraWidget, T object)
{
	gp_error_check(Func(cameraWidget, object));
}

struct camera_widget
{
	CameraWidget *cameraWidget;
	camera_widget(CameraWidget *cameraWidget)
		: cameraWidget(cameraWidget)
	{}

	auto get_label() { return getter<gp_widget_get_label>(cameraWidget); }

	auto get_name() { return getter<gp_widget_get_name>(cameraWidget); }
	auto set_name(auto val) { return setter<gp_widget_set_name>(cameraWidget, val); }
	auto get_info() { return getter<gp_widget_get_info>(cameraWidget); }
	auto set_info(auto val) { return setter<gp_widget_set_info>(cameraWidget, val); }

	auto get_id() { return getter<gp_widget_get_id>(cameraWidget); }

	auto get_readonly() { return getter<gp_widget_get_readonly>(cameraWidget); }
	auto set_readonly(auto val) { return setter<gp_widget_set_readonly>(cameraWidget, val); }
	auto get_changed() { return gp_widget_changed(cameraWidget); }
	auto set_changed(auto val) { return setter<gp_widget_set_changed>(cameraWidget, val); }

	auto get_type() { return getter<gp_widget_get_type>(cameraWidget); }

	CameraWidget *ptr() const { return cameraWidget; }
	// using gwidget_child_array = array_accessor<accessor<gp_widget_count_children>, accessor<gp_widget_get_child>>;
};

struct gphoto_widget{};

template <CameraWidgetType>
struct widget_formatter;

template <>
struct variable<gphoto_widget> : variable_base<gphoto_widget, "gp_widget">
{
	using base = variable_base<gphoto_widget, "gp_widget">;
	using base::base;
	constexpr variable(std::size_t id_hash, std::string_view fmt_str)
		: base(id_hash)
	{}

	constexpr auto operator()(auto &formatter, const camera_widget &cameraWidget) const;
};

// TODO integrate variable into format string
struct config_formatter
{
	using formatter = format_string<"${str:label}, ${str:name}, ${str:tooltip}, ${int:id}, ${bool:readonly}, ${bool:changed}, ${gp_widget:type}", concat_t<basic_variables, variable<gphoto_widget>>>;
	// Stream
	config_formatter(auto &stream)
		: stream(stream)
	{}
	~config_formatter()
	{
		stream << "\n";
		stream.close();
	}
	std::ofstream &stream;
	uint32_t indent_amount = 0;

	void push_indent()
	{
		indent_amount++;
	}
	void pop_indent()
	{
		indent_amount--;
	}
	void new_line()
	{
		stream << "\n";
		for (std::size_t i = 0; i < indent_amount; i++)
			stream << " ";
	}
	void write_config(CameraWidget *cameraWidget)
	{
		camera_widget widget(cameraWidget);
		formatter format;
		format(*this,
			   widget.get_label(),
			   widget.get_name(),
			   widget.get_info(),
			   widget.get_id(),
			   widget.get_readonly(),
			   widget.get_changed(),
			   widget);

		uint32_t childCount = gp_widget_count_children(cameraWidget);
		if (childCount > 0)
		{
			for (std::size_t i = 0; i < childCount; i++)
			{
				CameraWidget *child;
				gp_widget_get_child(cameraWidget, i, &child);
				push_indent();
				new_line();
				write_config(child);
				pop_indent();
			}
		}
	}
	
	auto &operator<<(auto val)
	{
		stream << val;
		return *this;
	}
};

template <CameraWidgetType Type>
void get_value(auto &stream, CameraWidget *cameraWidget)
{
	using value_type = typename widget_formatter<Type>::value_type;
	if constexpr (!std::is_same_v<value_type, void>)
	{
		camera_widget widget(cameraWidget);

		stream << widget.get_id() << " ";
		value_type value;
		gp_widget_get_value(cameraWidget, (void *)&value);
		stream << value << "\n";
	}
}

struct value_formatter
{
	value_formatter(auto &stream)
		: stream(stream)
	{}
	~value_formatter(){
		stream << "\n";
		stream.close();
	}
	std::ofstream &stream;

	void write_value(CameraWidget *cameraWidget)
	{
		camera_widget widget(cameraWidget);
		
		switch (widget.get_type())
		{
		case GP_WIDGET_TEXT:
			get_value<GP_WIDGET_TEXT>(stream, cameraWidget);
			break;
		case GP_WIDGET_RANGE:
			get_value<GP_WIDGET_RANGE>(stream, cameraWidget);
			break;
		case GP_WIDGET_TOGGLE:
			get_value<GP_WIDGET_TOGGLE>(stream, cameraWidget);
			break;
		case GP_WIDGET_RADIO:
			get_value<GP_WIDGET_RADIO>(stream, cameraWidget);
			break;
		case GP_WIDGET_MENU:
			get_value<GP_WIDGET_MENU>(stream, cameraWidget);
			break;
		case GP_WIDGET_BUTTON:
			get_value<GP_WIDGET_BUTTON>(stream, cameraWidget);
			break;
		case GP_WIDGET_DATE:
			get_value<GP_WIDGET_DATE>(stream, cameraWidget);
			break;
		}
		uint32_t childCount = gp_widget_count_children(cameraWidget);
		if (childCount > 0)
		{
			for (std::size_t i = 0; i < childCount; i++)
			{
				CameraWidget *child;
				gp_widget_get_child(cameraWidget, i, &child);

				write_value(child);
			}
		}
	}
};

// TODO Add in a hash checker
struct ChoicesAttributes
{
	using formatter = variable<string>;
	using const_str = const char *;

	void operator()(auto &output, CameraWidget *widget)
	{
		formatter format;

		std::size_t count = gp_widget_count_choices(widget);

		const_str choice;
		gp_widget_get_choice(widget, 0, &choice);

		format(output, choice);
		for (std::size_t i = 1; i < count; i++)
		{
			gp_widget_get_choice(widget, i, &choice);
			output << ", ";
			format(output, choice);
		}
	}
};

template <>
struct widget_formatter<GP_WIDGET_WINDOW>
{
	static constexpr Fixed_String type_str = make_fixed_string<"window">();
	using value_type = void;
};

template <>
struct widget_formatter<GP_WIDGET_SECTION>
{
	static constexpr Fixed_String type_str = make_fixed_string<"section">();
	using value_type = void;
};

template <>
struct widget_formatter<GP_WIDGET_TEXT>
{
	static constexpr Fixed_String type_str = make_fixed_string<"text">();
	using value_type = char *;
};

template <>
struct widget_formatter<GP_WIDGET_RANGE>
{
	static constexpr Fixed_String type_str = make_fixed_string<"range">();
	using formatter = format_string<"${int:min}, ${int:max}, ${int:increment}">;
	static constexpr Fixed_String label = "range";

	using value_type = float;

	void operator()(auto &output, CameraWidget *widget)
	{
		float min, max, increment;

		gp_error_check(gp_widget_set_range(widget, min, max, increment));

		formatter format;
		format(output, min, max, increment);
	}
};

template <>
struct widget_formatter<GP_WIDGET_TOGGLE>
{
	static constexpr Fixed_String type_str = make_fixed_string<"toggle">();
	using value_type = int;
};

template <>
struct widget_formatter<GP_WIDGET_RADIO> : ChoicesAttributes
{
	static constexpr Fixed_String type_str = make_fixed_string<"radio">();
	using value_type = char *;
};

template <>
struct widget_formatter<GP_WIDGET_MENU> : ChoicesAttributes
{
	static constexpr Fixed_String type_str = make_fixed_string<"menu">();

	using value_type = char *;
};

template <>
struct widget_formatter<GP_WIDGET_BUTTON>
{
	static constexpr Fixed_String type_str = make_fixed_string<"button">();
	using value_type = void;
};
template <>
struct widget_formatter<GP_WIDGET_DATE>
{
	static constexpr Fixed_String type_str = make_fixed_string<"date">();
	using value_type = int;
};


template <typename WidgetFormatter, typename FormatterT>
auto widget_common(WidgetFormatter widgetFormatter, FormatterT &formatter, CameraWidget *cameraWidget)
{
	formatter << WidgetFormatter::type_str;

	if constexpr (requires { widgetFormatter(formatter, cameraWidget); })
	{
		formatter.push_indent();
		formatter.new_line();

		widgetFormatter(formatter, cameraWidget);
		formatter.pop_indent();
	}
}

constexpr auto variable<gphoto_widget>::operator()(auto &formatter, const camera_widget &cameraWidgetObject) const
{
	CameraWidget *cameraWidget = cameraWidgetObject.ptr();
	CameraWidgetType widgetType;
	gp_widget_get_type(cameraWidget, &widgetType);

	switch (widgetType)
	{
	case GP_WIDGET_WINDOW:
		widget_common(widget_formatter<GP_WIDGET_WINDOW>(), formatter, cameraWidget);
		break;
	case GP_WIDGET_SECTION:
		widget_common(widget_formatter<GP_WIDGET_SECTION>(), formatter, cameraWidget);
		break;
	case GP_WIDGET_TEXT:
		widget_common(widget_formatter<GP_WIDGET_TEXT>(), formatter, cameraWidget);
		break;
	case GP_WIDGET_RANGE:
		widget_common(widget_formatter<GP_WIDGET_RANGE>(), formatter, cameraWidget);
		break;
	case GP_WIDGET_TOGGLE:
		widget_common(widget_formatter<GP_WIDGET_TOGGLE>(), formatter, cameraWidget);
		break;
	case GP_WIDGET_RADIO:
		widget_common(widget_formatter<GP_WIDGET_RADIO>(), formatter, cameraWidget);
		break;
	case GP_WIDGET_MENU:
		widget_common(widget_formatter<GP_WIDGET_MENU>(), formatter, cameraWidget);
		break;
	case GP_WIDGET_BUTTON:
		widget_common(widget_formatter<GP_WIDGET_BUTTON>(), formatter, cameraWidget);
		break;
	case GP_WIDGET_DATE:
		widget_common(widget_formatter<GP_WIDGET_DATE>(), formatter, cameraWidget);
		break;
	}
}
