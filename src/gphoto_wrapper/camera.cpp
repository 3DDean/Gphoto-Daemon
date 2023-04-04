#include "gphoto_wrapper/camera.h"

CameraObj::CameraObj(CameraObj &cam)
	: context(cam.context),
	  ptr(cam.ptr)
{
	gp_camera_ref(cam.ptr);
}

CameraObj::CameraObj()
	: cameraPath("/"),
	  context(nullptr),
	  ptr(nullptr)
{}

CameraObj::~CameraObj()
{
	if (ptr != nullptr)
	{
		gp_camera_exit(ptr, context);
		gp_camera_unref(ptr);
	}
}

void CameraObj::init(GPContext *contextPtr, Camera *cameraPtr, std::string_view nameStr)
{
	context = contextPtr;
	ptr = cameraPtr;
	name = nameStr;
}

int CameraObj::exit()
{
	gp_camera_exit(ptr, context);
	return GP_OK;
}

bool CameraObj::create_config_file(const daemon_config &config)
{
	camera_widget root_widget(ptr, context);

	auto config_file = config.new_camera_widget_file(name);
	config_writer widget_writer(config_file);
	widget_writer.write(name);
	std::stack<indent> indent_stack;

	uint32_t widget_count = 1;

	auto writer_func = [&widget_writer, &widget_count](camera_widget &widget)
	{
		widget_writer.write(widget);
	};
	auto push_func = [&]()
	{
		indent_stack.emplace(widget_writer);
	};
	auto pop_func = [&indent_stack]()
	{
		indent_stack.pop();
	};

	process_widget(root_widget, writer_func, push_func, pop_func);
	widget_writer << "\n";

	return true;
}

bool CameraObj::create_value_file(const daemon_config &config)
{
	camera_widget root_widget(ptr, context);
	auto config_file = config.new_camera_value_file(name);
	value_writer widget_writer(config_file);

	auto writer_func = [&widget_writer](camera_widget &widget)
	{
		widget_writer.write(widget);
	};
	auto push_func = []() {};
	auto pop_func = []() {};

	process_widget(root_widget, writer_func, push_func, pop_func);

	return true;
}

void CameraObj::set_config_value(std::string_view name, std::string_view value)
{
	camera_widget widget(ptr, context, name);
	// TODO Rewrite this to send output message to log

	gp_error_check(widget.set_value(value), "Failed to set value");
	gp_error_check(gp_camera_set_single_config(ptr, name.data(), widget, context));

	// TODO PUSH OUT TO LOG FILE
}

int CameraObj::triggerCapture()
{
	int ret;
	do
	{
		// TODO figure out if this is a bug
		ret = gp_camera_trigger_capture(ptr, context);
	} while (ret == GP_ERROR_CAMERA_BUSY);

	if ((ret != GP_OK) && (ret != GP_ERROR_NOT_SUPPORTED))
	{
		printf("Could not trigger capture.\n");
	}

	return ret;
}

// This currently returns early as the noises my camera makes scares my dog, so testing file handling is a bit difficult atm
gphoto_file CameraObj::capture()
{
	// std::string time = get_time();

	CameraFilePath filePath;
	gp_error_check(gp_camera_capture(ptr, GP_CAPTURE_IMAGE, &filePath, context), "Image Capture Failed");

	// TODO WAIT FOR EVENT SO THAT I CAN CONSUME IT ALL
	waitForEvent(10);

	return get_file(&filePath, context, GP_FILE_TYPE_NORMAL);
}

gphoto_file CameraObj::capture_preview()
{
	int ret;
	gphoto_file file;

	waitForEvent(10);

	gp_error_check(gp_camera_capture_preview(ptr, file, context), "Could not capture preview.\n");

	return file;
}
//TODO Determine if there is a memory leak here
int CameraObj::waitForEvent(int timeout)
{
	// std::vector<gp_event_variant> events;
	int ret;
	while (1)
	{
		CameraEventType evttype;
		void *data = NULL;

		ret = gp_camera_wait_for_event(ptr, timeout, &evttype, &data, context);
		std::cout << "DataPtr : " << data << "\n";
		switch (evttype)
		{
		case GP_EVENT_UNKNOWN: /**< unknown and unhandled event. argument is a char* or NULL */
			if (data)
				std::cout << "Unknown Event, Content" << (char *)data << "\n";
			break;
		case GP_EVENT_TIMEOUT: /**< timeout, no arguments */
			std::cout << "Event Timeout\n";
			return GP_OK;
		case GP_EVENT_FILE_ADDED: /**< CameraFilePath* = file path on camfs */
		{
			CameraFilePath *filepath = (CameraFilePath *)data;

			// auto [whole, match] = ctre::match<".+(\\.\\w+)">(filepath->name);
			// std::string filepath = config->image_dir;

			std::cout << "File Added\n\t" << filepath->folder << "/" << filepath->name << "\n";
		}
		break;
		case GP_EVENT_FOLDER_ADDED: /**< CameraFilePath* = folder on camfs */
		{
			std::cout << "Folder Added\n";
			CameraFilePath *filepath = (CameraFilePath *)data;
			std::cout << "Folder Added\n\t" << filepath->folder << "/" << filepath->name << "\n";
		}
		break;
		case GP_EVENT_CAPTURE_COMPLETE: /**< last capture is complete */
			std::cout << "Capture Complete\n";
			return GP_OK;
		case GP_EVENT_FILE_CHANGED: /**< CameraFilePath* = file path on camfs */
		{
			CameraFilePath *filepath = (CameraFilePath *)data;
			std::cout << "File Changed\n\t" << filepath->folder << "/" << filepath->name << "\n";
		}
		break;
		default:
			if (data)
				free(data);
			break;
		}

		if (ret < GP_OK)
			break;
	}
	return ret;
}
