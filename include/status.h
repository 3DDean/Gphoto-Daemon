#pragma once
#include <fstream>
#include <string>

struct StatusFile
{
	StatusFile(std::string_view status_filepath, std::string_view logpath)
		: filepath(status_filepath),
		  logpath(logpath)
	{}

	void write_log(std::string_view str)
	{
		std::ofstream status_file(logpath, std::ios_base::out);
		if (status_file.is_open())
		{
			status_file << str << "\n";
			status_file.close();
		}
		else
		{
			perror("file not opened");
		}
	}

	void set_state(std::string_view state = "ready", std::string_view response = "")
	{
		std::ofstream status_file(filepath, std::ios_base::trunc | std::ios_base::out);
		if (status_file.is_open())
		{
			status_file << state << response;
			status_file.close();
		}
		else
		{
			perror("file not opened");
		}
	}

	void set_ready(std::string_view response = "")
	{
		set_state("ready ", response);
	}

	void set_busy(std::string_view with)
	{
		set_state("busy ", with);
	}

	void set_finished(std::string_view result)
	{
		set_state("finished\n", result);
		write_log(result);
	}

	void set_failed(std::string_view result)
	{
		set_state("error\n", result);
		write_log(result);
	}

	void set_stopped(std::string_view result)
	{
		set_state("stopped\n", result);
	}

	std::string filepath;
	std::string logpath;
};
