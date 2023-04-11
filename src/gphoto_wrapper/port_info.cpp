// gphoto_wrapper.cpp
#include "gphoto_wrapper/port_info.h"
#include "gphoto-error.h"

gphoto_port_info::gphoto_port_info()
{
	// gp_error_check(gp_port_info_new(nullptr), "Failed to create port info");
}

gphoto_port_info::~gphoto_port_info()
{
	// gp_error_check(gp_port_info_free(ptr), "Failed to free port info");
}

std::string_view gphoto_port_info::getName()
{
	char *name = nullptr;
	gp_error_check(gp_port_info_get_name(ptr, &name), "Failed to get port name");
	return name;
}

void gphoto_port_info::setName(const std::string_view name)
{
	gp_error_check(gp_port_info_set_name(ptr, name.data()), "Failed to set port name");
}

std::string_view gphoto_port_info::getPath()
{
	char *path = nullptr;
	gp_error_check(gp_port_info_get_path(ptr, &path), "Failed to get port path");
	return path;
}

void gphoto_port_info::setPath(const std::string_view path)
{
	gp_error_check(gp_port_info_set_path(ptr, path.data()), "Failed to set port path");
}

GPPortType gphoto_port_info::getType()
{
	GPPortType type;
	gp_error_check(gp_port_info_get_type(ptr, &type), "Failed to get port type");
	return type;
}

void gphoto_port_info::setType(const GPPortType &type)
{
	gp_error_check(gp_port_info_set_type(ptr, type), "Failed to set port type");
}

gphoto_port_info::operator GPPortInfo *()
{
	return &ptr;
}
gphoto_port_info::operator GPPortInfo&()
{
	return ptr;
}

gphoto_port_info_list::gphoto_port_info_list()
	: ptr(nullptr)
{
}

gphoto_port_info_list::~gphoto_port_info_list()
{
	if(ptr)
		gp_error_check(gp_port_info_list_free(ptr), "Failed to free port info list");
}

void gphoto_port_info_list::load()
{
	if(ptr != nullptr)
		gp_error_check(gp_port_info_list_free(ptr), "Failed to free port info list");
	
	gp_error_check(gp_port_info_list_new(&ptr), "Failed to create port info list");

	gp_error_check(gp_port_info_list_load(ptr), "Failed to load port info list");
}

int gphoto_port_info_list::count() const
{
	int count = gp_port_info_list_count(ptr);
	gp_error_check(count, "Failed to get port info list count");
	return count;
}

int gphoto_port_info_list::lookup_path(std::string_view path) const
{
	int index = gp_port_info_list_lookup_path(ptr, path.data());
	gp_error_check(index, "Failed to get port info list path");
	return index;
}

int gphoto_port_info_list::lookup_name(std::string_view name) const
{
	int index = gp_port_info_list_lookup_name(ptr, name.data());
	gp_error_check(index, "Failed to get port info list name");
	return index;
}

gphoto_port_info gphoto_port_info_list::getPortInfoListInfo(int index) const
{
	gphoto_port_info portInfo;
	gp_error_check(gp_port_info_list_get_info(ptr, index, portInfo), "Failed to get port info list info");

	return portInfo;
}

gphoto_port_info_list::operator GPPortInfoList*()
{
	return ptr;
}
