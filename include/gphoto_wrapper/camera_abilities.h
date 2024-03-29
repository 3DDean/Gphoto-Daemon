#pragma once
#include "gphoto_wrapper/common.h"
#include <gphoto2/gphoto2-abilities-list.h>
#include <string_view>

class camera_abilities_list
{
  public:
	camera_abilities_list();
	GPHOTO_WRAPPER_MOVE_CONSTRUCTOR(camera_abilities_list);

	~camera_abilities_list();
	void load(GPContext *context);
	void load_dir(const char *dir, GPContext *context);
	void reset();
	void detect(GPPortInfoList *info_list, CameraList *l, GPContext *context);
	void append(CameraAbilities abilities);
	int count();
	int lookup_model(std::string_view model);
	void get_abilities(int index, CameraAbilities *abilities);

  private:
	CameraAbilitiesList *ptr;
};
