#pragma once
#include <gphoto2/gphoto2-result.h>
#include <stdio.h>
#include <string>
// const char *gp_port_result_as_string (int result);

#include <exception>

class GPhotoException : public std::exception
{
  public:
	GPhotoException(const int errorCode, const std::string &usrMsg)
		: message("User Error: " + usrMsg +
				  "\nGPhotoError : " + gp_result_as_string(errorCode))
	{}

	GPhotoException(const int errorCode)
		: message("GPhotoError : ")
	{
		message += gp_result_as_string(errorCode);
	}

	virtual const char *what() const noexcept override
	{
		return message.c_str();
	}

  private:
	std::string message;
};

template <typename... ArgsT>
inline void gp_error_check(int val, std::string msg = "Undefined Error", ArgsT... Args)
{
	if (val < GP_OK)
	{
		throw GPhotoException(val, msg);
	}
}
