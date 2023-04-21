#pragma once
#include <stdexcept>
#include <string.h>

struct linux_exception : public std::exception
{
  public:
	linux_exception(const std::string &usrMsg, const int errorCode);
	virtual ~linux_exception(){};
	virtual const char *what() const noexcept override;

	int error_num(){return error_code;}
  private:
	std::string message;
	const int error_code;
};
