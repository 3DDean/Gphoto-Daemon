#pragma once

#include <gphoto2/gphoto2-port.h>
#include <string>

class gphoto_port_info
{
  public:
	gphoto_port_info();
	~gphoto_port_info();

	std::string_view getName();
	void setName(const std::string_view name);

	std::string_view getPath();
	void setPath(const std::string_view path);

	GPPortType getType();
	void setType(const GPPortType &type);

	operator GPPortInfo *();

  private:
	GPPortInfo ptr;
};

class gphoto_port_info_list
{
  public:
	gphoto_port_info_list();
	~gphoto_port_info_list();

	void load();
	int count() const;
	int lookup_path(std::string_view path) const;
	int lookup_name(std::string_view path) const;
	gphoto_port_info getPortInfoListInfo(int index) const;
	operator GPPortInfoList*();
	
  private:
	GPPortInfoList *ptr;
};