#pragma once
#include <exception>
#include <fstream>
#include <sstream>
#include <string>

struct file_access_exception : public std::exception
{
	file_access_exception(std::string_view state, std::string_view file)
		: message(state)
	{
		message += " ";
		message += file;
	}

	virtual const char *what() const noexcept override
	{
		return message.c_str();
	}

  private:
	std::string message;
};

struct state_object
{
	state_object(std::string_view status_path, std::string_view command)
		: command(command),
		  status_path(status_path)
	{
		std::ofstream status_file(status_path.data(), std::ios_base::trunc | std::ios_base::out);

		if (!status_file.is_open())
			throw file_access_exception("failed to open", status_path);

		status_file << "executing " << command << "\n";
		status_file.close();
		result << "finished " << command << "\n";
	}

	~state_object()
	{
		std::ofstream status_file(status_path.data(), std::ios_base::trunc | std::ios_base::out);

		// if (!status_file.is_open())
		// 	throw file_access_exception("failed to open", status_path);

		status_file << result.str();
		status_file.close();
	};

	void set_result(std::string_view state, std::string_view arg)
	{
		result.seekp(0);
		result << state << " " << command << "\n";
		result << arg;
	}
	void append_result(auto str)
	{
		result << str;
	}

  private:
	std::string_view status_path;
	std::string_view command;
	std::ostringstream result;
};

struct status_manager
{
	status_manager(std::string_view status_path)
		: status_path(status_path)
	{
	}
	inline auto get_status_object(std::string_view command_name)
	{
		return state_object(status_path, command_name);
	}

  private:
	std::string status_path;
};

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
