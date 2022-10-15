#pragma once
#include <fstream>
#include <string>

struct StatusMessenger
{
	StatusMessenger(std::string status_filepath) : filepath(status_filepath)
	{}

	void set(std::string str = "ready")
	{
		std::ofstream status_file(filepath, std::ios_base::trunc | std::ios_base::out);
		if(status_file.is_open())
		{
			status_file << str;
			status_file.close();
		}
		else
		{
			perror("file not opened");
		}
	}

	std::string filepath;
};