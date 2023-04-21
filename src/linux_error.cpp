#include "linux_error.h"

linux_exception::linux_exception(const std::string &usrMsg, const int errorCode)
	: message(usrMsg),
	  error_code(errorCode)
{
	message += " : ";
	message += strerror(error_code);
}

const char *linux_exception::what() const noexcept
{
	return message.data();
}
