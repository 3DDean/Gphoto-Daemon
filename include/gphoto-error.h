#pragma once
#include <gphoto2/gphoto2-result.h>
#include <stdio.h>
#include <string>
// const char *gp_port_result_as_string (int result);

template<typename... ArgsT>
inline void gp_error_check(int val, std::string msg = "Undefined Error", ArgsT... Args)
{
	if (val < GP_OK)
	{
		perror(gp_result_as_string(val));
	}
}