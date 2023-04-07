#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "daemon_config.h"
#include "gphoto-widget.h"
#include "gphoto.h"
#include "named_pipe.h"
#include <gphoto2/gphoto2-camera.h>

#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>

#include "common.h"
#include "format.h"
#include "status.h"
#include <sstream>
#include <thread>
// https://gist.github.com/gcmurphy/c4c5222075d8e501d7d1

bool running = true;
void term(int signum)
{
	running = false;
}

using millisecond_duration = std::chrono::duration<double, std::milli>;

struct timelapse_manager
{
	timelapse_manager(daemon_config &config)
		: thread_state(state::idle),
		  do_run(false),
		  config(config)
	{}

	enum class state
	{
		idle,
		stopped,
		running,
		error
	};

	void start(CameraObj &activeCamera, int delayMs, std::string_view timelapse_directory)
	{
		auto containsIllegalTokens = ctre::search<"/">(timelapse_directory);

		if (delayMs > 0 && thread_state != state::running && !containsIllegalTokens)
		{
			filepath = config.image_dir;
			filepath += "/";
			// No specified directory so use current time instead
			if (timelapse_directory.empty())
				filepath += get_time("%H-%M");

			if (!std::filesystem::create_directory(filepath))
			{
				// I don't like accidentally overriding files, though I need to look into whether this includes special files
				capture_count = std::distance(std::filesystem::directory_iterator(filepath), std::filesystem::directory_iterator{});
			}
			filepath += "/";
			delay = delayMs;
			do_run = true;
			thread_state = state::running;
			failed_capture_count = 0;
			failed_capture_max = 3;

			thread = std::thread(
				[&]()
				{
					std::chrono::microseconds delayTime(delayMs);
					millisecond_duration delayDuration(delayTime);

					while (do_run)
					{
						try
						{
							auto image = activeCamera.capture();
							std::string imagepath = filepath;

							imagepath += std::to_string(capture_count);
							capture_count++;
							imagepath += ".jpg";
							image.save(imagepath);
						}
						catch (GPhotoException &e)
						{
							if (failed_capture_count > failed_capture_max)
							{
								thread_state = state::error;
								return;
							}
							failed_capture_count++;
							// LOG CAPTURE ERROR
						}

						std::this_thread::sleep_for(delayDuration);
					}
					thread_state = state::stopped;
					return;
				});
		}
	}

	void stop()
	{
		do_run = false;
		thread.join();
	}

	bool is_running()
	{
		return do_run;
	}

  private:
	std::atomic<bool> do_run;
	std::atomic<state> thread_state;
	std::atomic<useconds_t> delay;

	daemon_config &config;

	std::string filepath;
	std::thread thread;

	int capture_count;
	int failed_capture_count;
	int failed_capture_max;
};

struct indenter
{
	uint32_t indentAmount = 0;

	void operator()(std::string_view name, std::string_view prefix = "", std::string_view suffix = "")
	{
		for (size_t i = 0; i < indentAmount; i++)
		{
			std::cout << "  ";
		}

		std::cout << prefix << name << suffix << "\n";
	}
};

/* TODO
	Figure out if I can save a capture to the camera
	and if not figure out how much ram it have available
*/

void print_folder_contents(CameraObj &cam, GPContext *context, std::string_view folder = "/", indenter printer = indenter{0})
{
	for (auto [name, value] : cam.list_folders(folder, context))
	{
		printer(name, "", "/");
		std::string folderPath(folder);
		if (folder.length() > 1)
		{
			folderPath += "/";
		}
		folderPath += name;

		print_folder_contents(cam, context, folderPath, indenter(printer.indentAmount + 1));
	}

	for (auto [name, value] : cam.list_files(folder, context))
	{
		printer(name, "");
		if (auto [match, file_extension] = ctre::match<".+\\.(\\w+)">(name); match)
		{
			if (file_extension == "JPG")
			{
				auto fileNormal = cam.get_file(folder, name, context, GP_FILE_TYPE_NORMAL);
				auto filePreview = cam.get_file(folder, name, context, GP_FILE_TYPE_PREVIEW);
				std::string previewPath = "thumb_";
				previewPath += name;
				fileNormal.save(name);
				filePreview.save(previewPath);
			}
		}
	}
}

// template <typename T>
// class c_array
// {
//   public:
// 	c_array(T *array, size_t size)
// 		: data(array), length(size) {}

// 	T &operator[](size_t index)
// 	{
// 		return data[index];
// 	}

// 	const T &operator[](size_t index) const
// 	{
// 		return data[index];
// 	}

// 	T *begin()
// 	{
// 		return data;
// 	}

// 	T *end()
// 	{
// 		return data + length;
// 	}

// 	const T *begin() const
// 	{
// 		return data;
// 	}

// 	const T *end() const
// 	{
// 		return data + length;
// 	}

//   private:
// 	T *data;
// 	size_t length;
// };

// TODO Create template
struct cli_args
{
	void set_value(std::stringstream &error_stream, std::string_view flagName, auto& arg_itt)
	{
		++arg_itt;
		if (flagName == "config")
		{
			config_file = arg_itt->data();
		}
		else
		{
			error_stream << "unrecognized argument" << *arg_itt << "\n";
		}
	}

	std::string_view config_file;
};

cli_args process_args(int argc, const char **argv)
{
	cli_args args;

	std::vector<std::string_view> arg_strs;
	std::stringstream error_stream;

	for(std::size_t i = 0; i < argc; i++)
		arg_strs.emplace_back(argv[i]);

	for(auto itt = arg_strs.begin(); itt < arg_strs.end(); itt++)
	{
		std::string_view arg(*itt);
		try
		{
			if (arg.starts_with("--"))
				args.set_value(error_stream, arg.substr(2, arg.size() - 2), itt);
		}
		catch (const std::out_of_range &e)
		{
			// TODO exit
		}
	}
	auto error_view = error_stream.view();
	if (error_view.empty())
		return args;

	std::cout << error_view;
	exit(-2);
}


int main(int argc, const char **argv)
{
	daemon_config config;

	{
		cli_args args = process_args(argc, argv);
		config.init(args.config_file);
	}
	// Linux signal handling, more information in signal.h
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	sigset_t continueMask;
	sigemptyset(&continueMask);
	sigaddset(&continueMask, SIGCONT);
	sigaddset(&continueMask, SIGUSR1);
	sigaddset(&continueMask, SIGUSR2);

	GPhoto gphoto;

	CameraObj activeCamera;

	if (gphoto.cameraCount() > 0)
	{
		gphoto.openCamera(0, activeCamera);
		activeCamera.create_config_file(config);
		activeCamera.create_value_file(config);
		activeCamera.config = &config;
	}
	else
	{
		std::printf("No cameras detected\n");
		return -1;
	}

	using stack_allocated_buffer = stack_buffer<512>;

	StatusFile application_status(config.get_status_file_path(), "log.txt");

	read_pipe<stack_allocated_buffer> instruction_pipe(config.get_pipe_file_path(), O_NONBLOCK);

	// TODO Set up response que
	// TODO Update values file when camera config options are updated
	// TODO Fix radio buttons
	application_status.set_state();

	timelapse_manager timelapseManager(config);

	int capture_count = 0;
	int preview_count = 0;

	pipe_buffer<512> piperBuffer;
	while (running)
	{
		// sigsuspend(&continueMask);
		for (auto pipeData : instruction_pipe)
		{
			for (auto match : ctre::split<"\n">(instruction_pipe.get_string_view()))
			{
				std::string_view matchingStr = match;
				if (auto [match2, commandResult, valueResult] = ctre::match<"(\\w+)\\s*(.+)?">(matchingStr); match2)
				{
					std::string command((std::string_view)commandResult);
					std::string value((std::string_view)valueResult);

					if (command == "capture_preview")
					{
						try
						{
							application_status.set_busy("capturing_preview");
							gphoto_file preview_capture = activeCamera.capture_preview();
							std::string captureName = config.image_dir;
							captureName += "/";
							captureName += "preview";
							captureName += std::to_string(preview_count);
							captureName += ".jpg";

							preview_capture.save(captureName);

							application_status.set_finished("");
							++preview_count;
						}
						catch (GPhotoException &e)
						{
							application_status.set_failed(e.what());
						}
					}
					else if (command == "capture")
					{
						try
						{
							application_status.set_busy("capturing");
							gphoto_file last_capture = activeCamera.capture();
							std::string capture_path = config.image_dir;
							capture_path += "/";
							std::string captureName = "capture";
							captureName += std::to_string(capture_count);
							captureName += ".jpg";

							capture_path += captureName;
							last_capture.save(capture_path);
							application_status.set_finished(captureName);
							++capture_count;
						}
						catch (GPhotoException &e)
						{
							application_status.set_failed(e.what());
						}
					}
					else if (command == "capture_timelapse")
					{
						if (!timelapseManager.is_running())
						{
							try
							{
								auto [match, delay, dirname] = ctre::match<"(\\w+) (\\w+)?">(value);
								int delayAmount = std::stoi(value.data());
								timelapseManager.start(activeCamera, delayAmount, dirname);

								application_status.set_busy("capture_timelapse");
							}
							catch (std::exception &e)
							{
								application_status.set_failed(e.what());
							}
						}
						else
						{
							timelapseManager.stop();
							application_status.set_stopped("capture_timelapse");
						}
					}
					else
					{
						try
						{
							application_status.set_busy("setting_config");

							activeCamera.set_config_value(command, value);
							application_status.set_finished("");
						}
						catch (std::exception &e)
						{
							application_status.set_failed(e.what());
						}

						// TODO inform the instruction pipe that this data has been consumed and can be written over if need be
					}
					pipeData.consume(match.end());
				}
				// TODO Command parsing

				std::cout << matchingStr << "\n";
			}
		}
		instruction_pipe.clear();
		std::chrono::microseconds delayTime(10000);
		millisecond_duration delayDuration(delayTime);
		std::this_thread::sleep_for(delayDuration);
	}

	return 0;
}
