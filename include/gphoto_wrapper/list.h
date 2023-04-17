#pragma once
#include "gphoto_wrapper/common.h"
#include <gphoto2/gphoto2-list.h>
#include <string_view>

class gphoto_list
{
  public:
	gphoto_list();
	GPHOTO_WRAPPER_COPY_AND_MOVE_CONSTRUCTOR(gphoto_list, gp_list_ref);
	~gphoto_list();

	using name_value_pair = std::pair<std::string_view, std::string_view>;

	struct iterator
	{
		iterator(int index, gphoto_list &obj);
		iterator &operator++();
		inline name_value_pair operator*()
		{
			return obj.get_pair(index);
		}
		bool operator!=(const iterator &other) const;

	  private:
		int index;
		gphoto_list &obj;
	};

	name_value_pair get_pair(int index);
	std::string_view get_name(int index);
	std::string_view get_value(int index);

	int count();
	iterator begin();
	iterator end();

	operator CameraList *();

  private:
	CameraList *ptr;
};
