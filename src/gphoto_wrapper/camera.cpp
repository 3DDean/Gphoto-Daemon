#include "gphoto_wrapper/camera.h"

CameraObj::CameraObj()
	: context(nullptr),
	  ptr(nullptr),
	  capture_count(0),
	  preview_count(0)
{}

CameraObj::CameraObj(GPContext *contextPtr,
					 CameraAbilities abilities,
					 gphoto_port_info &info,
					 std::string_view nameStr,
					 std::string_view port,
					 std::filesystem::path image_dir)
	: image_path(image_dir),
	  name(nameStr),
	  port(port),
	  context(contextPtr),
	  ptr(nullptr),
	  capture_count(0),
	  preview_count(0)
{
	context = contextPtr;
	gp_error_check(gp_camera_new(&ptr), "Failed to create new camera");
	set_abilities(abilities);
	set_port_info(info);
	gp_error_check(gp_camera_init(ptr, context), "Failed to init camera device");

	name = nameStr;
	// gp_camera_exit(ptr, context);
}

CameraObj::CameraObj(CameraObj &&copyTarget)
	: context(copyTarget.context),
	  ptr(copyTarget.ptr),
	  name(copyTarget.name),
	  image_path(copyTarget.image_path),
	  capture_count(copyTarget.capture_count),
	  preview_count(copyTarget.preview_count)
{
	copyTarget.ptr = nullptr;
	copyTarget.context = nullptr;
}

CameraObj::~CameraObj()
{
	if (ptr != nullptr)
	{
		gp_camera_exit(ptr, context);
		gp_camera_unref(ptr);
	}
}

void CameraObj::init(GPContext *contextPtr, std::string_view nameStr, std::filesystem::path camera_dir)
{
	context = contextPtr;
	gp_error_check(gp_camera_new(&ptr), "Failed to create new camera");
	gp_error_check(gp_camera_init(ptr, context), "Failed to init camera device");

	image_path = camera_dir;

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
gphoto_file CameraObj::capture_image()
{
	// std::string time = get_time();

	CameraFilePath filePath;
	gp_error_check(gp_camera_capture(ptr, GP_CAPTURE_IMAGE, &filePath, context), "Image Capture Failed");

	// TODO WAIT FOR EVENT SO THAT I CAN CONSUME IT ALL
	waitForEvent(10);

	return get_file(&filePath, context, GP_FILE_TYPE_NORMAL);
}

// TODO Determine if there is a memory leak here
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
CameraAbilities *CameraObj::get_abilities()
{
	CameraAbilities *abilities;
	int ret = gp_camera_get_abilities(ptr, abilities);
	gp_error_check(ret, "Failed to get camera abilities");
	return abilities;
}

GPPortInfo *CameraObj::get_port_info()
{
	GPPortInfo *info;
	int ret = gp_camera_get_port_info(ptr, info);
	gp_error_check(ret, "Failed to get port info");
	return info;
}

int CameraObj::get_port_speed()
{
	int port_speed = gp_camera_get_port_speed(ptr);
	gp_error_check(port_speed, "Failed to get port speed");
	return port_speed;
}

void CameraObj::set_abilities(CameraAbilities abilities)
{
	int ret = gp_camera_set_abilities(ptr, abilities);
	gp_error_check(ret, "Failed to set camera abilities");
}

void CameraObj::set_port_info(gphoto_port_info &info)
{
	int ret = gp_camera_set_port_info(ptr, info);
	gp_error_check(ret, "Failed to set port info");
}

void CameraObj::set_port_speed(int speed)
{
	int ret = gp_camera_set_port_speed(ptr, speed);
	gp_error_check(ret, "Failed to set port speed");
}

std::string CameraObj::capture_preview()
{
	int ret;
	gphoto_file preview_capture;

	waitForEvent(10);

	gp_error_check(gp_camera_capture_preview(ptr, preview_capture, context), "Could not capture preview.\n");

	std::filesystem::path captureName = image_path;
	captureName /= "preview";
	captureName += std::to_string(preview_count) += ".jpg";

	preview_capture.save(captureName.c_str());

	++preview_count;

	return captureName.c_str();
}

// The functional difference between capture and timelapse is that
// saves the image before taking another image
std::string CameraObj::capture(int delay, int count)
{
	gphoto_file last_capture = capture_image();
	std::filesystem::path capture_path = image_path;
	capture_path /= "image";
	capture_path += std::to_string(capture_count) += ".jpg";

	++capture_count;
	last_capture.save(capture_path.c_str());
	return capture_path.string();
}

bool CameraObj::toggle_timelapse(int delay, int count)
{
}

void CameraObj::process_command(status_manager &config,
					 std::string_view &command,
					 std::vector<std::string_view> &cmdArgs)
{
	instruction_set commands(
		instruction("capture", &CameraObj::capture),
		instruction("capture_preview", &CameraObj::capture_preview),
		instruction("toggle_timelapse", &CameraObj::toggle_timelapse),
		instruction("setting_config", &CameraObj::set_config_value));

	commands.parse_command(config, command, cmdArgs, *this);
}

void timelapse_manager::start(CameraObj &activeCamera, int delayMs, std::string_view timelapse_directory)
{
	// auto containsIllegalTokens = ctre::search<"/">(timelapse_directory);

	// if (delayMs > 0 && thread_state != state::running && !containsIllegalTokens)
	// {
	// 	filepath = config.image_dir;
	// 	filepath += "/";
	// 	// No specified directory so use current time instead
	// 	if (timelapse_directory.empty())
	// 		filepath += get_time("%H-%M");

	// 	if (!std::filesystem::create_directory(filepath))
	// 	{
	// 		// I don't like accidentally overriding files, though I need to look into whether this includes special files
	// 		capture_count = std::distance(std::filesystem::directory_iterator(filepath), std::filesystem::directory_iterator{});
	// 	}
	// 	filepath += "/";
	// 	delay = delayMs;
	// 	do_run = true;
	// 	thread_state = state::running;
	// 	failed_capture_count = 0;
	// 	failed_capture_max = 3;

	// 	thread = std::thread(
	// 		[&]()
	// 		{
	// 			std::chrono::microseconds delayTime(delayMs);
	// 			millisecond_duration delayDuration(delayTime);

	// 			while (do_run)
	// 			{
	// 				try
	// 				{
	// 					auto image = activeCamera.capture();
	// 					std::string imagepath = filepath;

	// 					imagepath += std::to_string(capture_count);
	// 					capture_count++;
	// 					imagepath += ".jpg";
	// 					image.save(imagepath);
	// 				}
	// 				catch (GPhotoException &e)
	// 				{
	// 					if (failed_capture_count > failed_capture_max)
	// 					{
	// 						thread_state = state::error;
	// 						return;
	// 					}
	// 					failed_capture_count++;
	// 					// LOG CAPTURE ERROR
	// 				}

	// 				std::this_thread::sleep_for(delayDuration);
	// 			}
	// 			thread_state = state::stopped;
	// 			return;
	// 		});
	// }
}

void timelapse_manager::stop()
{
	do_run = false;
	thread.join();
}

bool timelapse_manager::is_running()
{
	return do_run;
}