#pragma once
#include "gphoto-error.h"

#define GPHOTO_WRAPPER_COPY_CONSTRUCTOR(obj_name, ref_function)       \
	obj_name(const obj_name &other) : ptr(other.ptr)                  \
	{                                                                 \
		gp_error_check(ref_function(ptr), "Failed to copy obj_name"); \
	}

#define GPHOTO_WRAPPER_MOVE_CONSTRUCTOR(obj_name) \
	obj_name(obj_name &&other) : ptr(other.ptr)                 \
	{                                                           \
		other.ptr = nullptr;                                    \
	}

#define GPHOTO_WRAPPER_COPY_AND_MOVE_CONSTRUCTOR(obj_name, ref_function) \
	GPHOTO_WRAPPER_COPY_CONSTRUCTOR(obj_name, ref_function)\
	GPHOTO_WRAPPER_MOVE_CONSTRUCTOR(obj_name);

