#pragma once
#include "Fixed_Array.h"
#include "format.h"
#include <fstream>

#include "gphoto-error.h"
#include "tuple.h"
#include <string>
#include <type_traits>
// #include <array>
#include <concepts>
#include <functional>
#include <gphoto2/gphoto2-widget.h>
#include <memory>
#include <type_traits>

template <auto>
struct function_traits;

template <typename ReturnType, typename... ArgsT, ReturnType (*func)(ArgsT...)>
struct function_traits<func>
{
	using return_type = ReturnType;
	using args = std::tuple<ArgsT...>;
	using function_type = std::function<ReturnType(ArgsT...)>;
};

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

struct gphoto_widget
{};

template <CameraWidgetType>
struct widget_object;

template <Fixed_String TypeStr, typename ValueType>
struct widget_format_helper
{
	// static constexpr Fixed_String type_str = make_fixed_string<TypeStr>();
	using value_type = ValueType;

	ValueType get_value(CameraWidget *cameraWidget) const
	{
		ValueType value;
		gp_widget_get_value(cameraWidget, (void *)&value);
		return value;
	}
	std::string_view type_str() const
	{
		return TypeStr;
	}
};

template <Fixed_String TypeStr>
struct widget_format_helper<TypeStr, void>
{
	// static constexpr Fixed_String type_str = make_fixed_string<TypeStr>();
	using value_type = void;
	std::string_view type_str() const
	{
		return TypeStr;
	}
};

struct indent;
// It's a choosy widget
template <Fixed_String TypeStr, typename T>
struct widget_with_choices
{
	// static constexpr Fixed_String type_str = make_fixed_string<TypeStr>();
	std::string_view type_str() const
	{
		return TypeStr;
	}

	using value_type = char *;

	using formatter = variable<string>;
	using const_str = const char *;

	const_str get_choice(CameraWidget *widget, std::size_t index) const
	{
		const_str choice;
		gp_widget_get_choice(widget, index, &choice);
		return choice;
	}

	int32_t get_value(CameraWidget *widget) const
	{
		const_str value;
		gp_widget_get_value(widget, (void *)&value);

		for (std::size_t i = 0; i < gp_widget_count_choices(widget); i++)
		{
			const_str option = get_choice(widget, i);
			if (std::string_view(value) == std::string_view(option))
				return i;
		}

		return -1;
	}

	void format_supplementary(auto &output, CameraWidget *widget)
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
struct widget_object<GP_WIDGET_WINDOW> : widget_format_helper<"window", void>
{
};

template <>
struct widget_object<GP_WIDGET_SECTION> : widget_format_helper<"section", void>
{
};

template <>
struct widget_object<GP_WIDGET_TEXT> : widget_format_helper<"text", char *>
{
};

template <>
struct widget_object<GP_WIDGET_RANGE> : widget_format_helper<"range", float>
{
	using formatter = format_string<"${int:min}, ${int:max}, ${int:increment}">;

	void format_supplementary(auto &&stream, CameraWidget *widget)
	{
		float min, max, increment;

		gp_error_check(gp_widget_set_range(widget, min, max, increment));

		formatter format;
		format(stream, min, max, increment);
	}
};

template <>
struct widget_object<GP_WIDGET_TOGGLE> : widget_format_helper<"toggle", int>
{
};

template <>
struct widget_object<GP_WIDGET_RADIO> : widget_with_choices<"radio", char *>
{
};

template <>
struct widget_object<GP_WIDGET_MENU> : widget_with_choices<"menu", char *>
{
};

template <>
struct widget_object<GP_WIDGET_BUTTON> : widget_format_helper<"button", void>
{
};

template <>
struct widget_object<GP_WIDGET_DATE> : widget_format_helper<"date", int>
{
};

template <CameraWidgetType... TypeEnum>
using widget_variant_helper = std::variant<widget_object<TypeEnum>...>;

using widget_variant =
	widget_variant_helper<
		GP_WIDGET_WINDOW,
		GP_WIDGET_SECTION,
		GP_WIDGET_TEXT,
		GP_WIDGET_RANGE,
		GP_WIDGET_TOGGLE,
		GP_WIDGET_RADIO,
		GP_WIDGET_MENU,
		GP_WIDGET_BUTTON,
		GP_WIDGET_DATE>;

// For shared_ptr deleter
struct camera_widget_deleter
{
	void operator()(CameraWidget *ptr) const noexcept
	{
		gp_widget_free(ptr);
	}
};

// Workaround for gphoto internal memory management
struct camera_widget_non_owning
{
	void operator()(CameraWidget *ptr) const noexcept
	{}
};

inline CameraWidget *get_camera_root_config(Camera *camera, GPContext *context)
{
	CameraWidget *window;
	gp_camera_get_config(camera, &window, context);
	return window;
}

struct camera_widget
{
	std::shared_ptr<CameraWidget> ptr;
	widget_variant widget_type;

	camera_widget() {}
	camera_widget(const camera_widget &widget)
		: ptr(widget.ptr), widget_type(widget.widget_type) {}

	camera_widget(Camera *camera, GPContext *context)
		: camera_widget(get_camera_root_config(camera, context), camera_widget_deleter())
	{}

	template <typename DeleterT>
	camera_widget(CameraWidget *ptr, DeleterT deleter = camera_widget_deleter{}) requires(!std::is_pointer_v<DeleterT>)
		: ptr(ptr, deleter)
	{
		switch (get_type())
		{
		case GP_WIDGET_WINDOW:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_WINDOW>);
			break;
		case GP_WIDGET_SECTION:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_SECTION>);
			break;
		case GP_WIDGET_TEXT:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_TEXT>);
			break;
		case GP_WIDGET_RANGE:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_RANGE>);
			break;
		case GP_WIDGET_TOGGLE:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_TOGGLE>);
			break;
		case GP_WIDGET_RADIO:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_RADIO>);
			break;
		case GP_WIDGET_MENU:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_MENU>);
			break;
		case GP_WIDGET_BUTTON:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_BUTTON>);
			break;
		case GP_WIDGET_DATE:
			widget_type = widget_variant(std::in_place_index<GP_WIDGET_DATE>);
			break;
		}
	}

	auto get_label() const { return getter<gp_widget_get_label>(ptr.get()); }

	auto get_name() const { return getter<gp_widget_get_name>(ptr.get()); }
	auto set_name(auto val) { return setter<gp_widget_set_name>(ptr.get(), val); }
	auto get_info() const { return getter<gp_widget_get_info>(ptr.get()); }
	auto set_info(auto val) { return setter<gp_widget_set_info>(ptr.get(), val); }

	auto get_id() const { return getter<gp_widget_get_id>(ptr.get()); }

	auto get_readonly() const { return getter<gp_widget_get_readonly>(ptr.get()); }
	auto set_readonly(int val) { return setter<gp_widget_set_readonly>(ptr.get(), val); }
	auto get_changed() const { return gp_widget_changed(ptr.get()); }
	auto set_changed(int val) { return setter<gp_widget_set_changed>(ptr.get(), val); }

	auto get_child_count() const { return gp_widget_count_children(ptr.get()); }
	auto get_child(std::size_t index) const
	{
		CameraWidget *child;
		gp_widget_get_child(ptr.get(), index, &child);
		return child;
	}

	CameraWidgetType get_type() const { return getter<gp_widget_get_type>(ptr.get()); }

	std::string_view get_type_str() const
	{
		return std::visit<std::string_view>(
			[](auto &&arg)
			{
				using T = std::decay_t<decltype(arg)>;
				return arg.type_str();
			},
			widget_type);
	}

	void use_value(auto &&func) const
	{
		std::visit<void>([&](auto &&arg)
						 {
			using T = typename std::decay_t<decltype(arg)>::value_type;
			if constexpr (!std::is_same_v<T, void>)
			{
				auto value = arg.get_value(ptr.get());
				func(value);
			} },
						 widget_type);
	}

	void format_supplementary(auto &&stream)
	{
		std::visit<void>([&](auto &&arg)
						 {
							 using T = typename std::decay_t<decltype(arg)>::value_type;
							 if constexpr (requires { arg.format_supplementary(stream, ptr.get()); })
							 {
								indent indent_obj = indent(stream);
								stream.new_line();
								arg.format_supplementary(stream, ptr.get());
							 } },
						 widget_type);
	}
};

struct config_writer
{
	using formatter = format_string<"${str:label}, ${str:name}, ${str:tooltip}, ${int:id}, ${bool:readonly}, ${bool:changed}, ${type}">;
	// Stream
	config_writer(auto &stream)
		: stream(stream)
	{}
	~config_writer()
	{
		stream << "\n";
		stream.close();
	}
	std::ofstream &stream;
	formatter format;
	uint32_t indent_amount = 0;

	void new_line()
	{
		stream << "\n";
		if (indent_amount > 500)
			throw "Overflow";

		for (std::size_t i = 0; i < indent_amount; ++i)
			stream << " ";
	}
	void write(std::string_view camera_name)
	{
		stream << camera_name;
	}

	void write(camera_widget widget)
	{
		new_line();
		format(*this,
			   widget.get_label(),
			   widget.get_name(),
			   widget.get_info(),
			   widget.get_id(),
			   widget.get_readonly(),
			   widget.get_changed(),
			   widget.get_type_str());

		widget.format_supplementary(*this);
	}

	auto &operator<<(auto val)
	{
		stream << val;
		return *this;
	}
};

struct value_writer
{
	using formatter = format_string<"${str:id}, ${value}\n">;

	value_writer(auto &stream)
		: stream(stream) {}

	std::ofstream &stream;
	formatter format;

	void write(camera_widget widget)
	{
		auto format_func = [&](auto &&value)
		{
			stream << widget.get_name() << " ";
			stream << value << "\n";
		};

		widget.use_value(format_func);
	}
	auto &operator<<(auto val)
	{
		stream << val;
		return *this;
	}
};

struct indent
{
	uint32_t &indent_amount;
	indent(config_writer &writer)
		: indent_amount(writer.indent_amount)
	{
		++indent_amount;
	}

	indent(indent &&old) = default;
	indent(indent &) = delete;
	indent(const indent &) = delete;
	indent &operator=(const indent &) = delete;
	indent &operator=(indent &) = delete;

	~indent() { --indent_amount; }
};
