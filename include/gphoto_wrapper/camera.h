#pragma once
#include "daemon_config.h"
#include "format.h"
#include "gphoto-widget.h"
#include <chrono>
#include <gphoto2/gphoto2-camera.h>
#include <iomanip>
#include <sstream>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

struct CameraObj
{
	CameraObj(CameraObj &cam);
	CameraObj();

	~CameraObj();

	void init(GPContext *contextPtr, Camera *cameraPtr, std::string_view nameStr);

	bool create_config_file(const daemon_config &config);
	bool create_value_file(const daemon_config &config);
	void set_config_value(std::string_view name, std::string_view value);

	int exitCamera();
	int triggerCapture();
	int capture_preview();
	int waitForEvent(int timeout);
	int capture();

	daemon_config *config;

  private:
	std::string name;
	std::string cameraPath;
	GPContext *context = nullptr;
	Camera *ptr = nullptr;


	// Linear implementation
	void process_widget(camera_widget root_widget, auto &&element_func, auto &&push_func, auto &&pop_func)
	{
		using widget_iterator = std::tuple<camera_widget, std::size_t, std::size_t>;

		std::stack<widget_iterator> widget_stack;

		uint32_t widget_count = 1;

		auto &&push_to_stack = [&](camera_widget widget)
		{
			widget_stack.emplace(widget, 0, widget.get_child_count());
			push_func();
		};
		element_func(root_widget);

		push_to_stack(root_widget);
		while (!widget_stack.empty())
		{
			bool changed_top = false;
			auto &&[parent_widget, itt, end] = widget_stack.top();

			while (itt < end)
			{
				camera_widget widget(parent_widget.get_child(itt), camera_widget_non_owning());
				++itt;
				element_func(widget);

				if (widget.get_child_count() > 0)
				{
					push_to_stack(widget);
					changed_top = true;
					break;
				}
			}

			if (itt == end && !changed_top)
			{
				widget_stack.pop();
				pop_func();
			}
		}
	}
};
