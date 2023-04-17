#include "gphoto_wrapper/camera_abilities.h"
#include "gphoto-error.h"

camera_abilities_list::camera_abilities_list() : ptr(nullptr)
{
	gp_abilities_list_new(&ptr);
}

camera_abilities_list::~camera_abilities_list()
{
	if(ptr != nullptr)
		gp_abilities_list_free(ptr);
}

void camera_abilities_list::load(GPContext *context)
{
	int ret = gp_abilities_list_load(ptr, context);
	gp_error_check(ret, "Failed to load camera abilities list.");
}

void camera_abilities_list::load_dir(const char *dir, GPContext *context)
{
	int ret = gp_abilities_list_load_dir(ptr, dir, context);
	gp_error_check(ret, "Failed to load camera abilities list from directory.");
}
void camera_abilities_list::reset()
{
	int ret = gp_abilities_list_reset(ptr);
	gp_error_check(ret, "Failed to reset camera abilities list");
}

void camera_abilities_list::detect(GPPortInfoList *info_list, CameraList *l, GPContext *context)
{
	int ret = gp_abilities_list_detect(ptr, info_list, l, context);
	gp_error_check(ret, "Failed to detect cameras.");
}

void camera_abilities_list::append(CameraAbilities abilities)
{
	int ret = gp_abilities_list_append(ptr, abilities);
	gp_error_check(ret, "Failed to append camera abilities to list.");
}

int camera_abilities_list::count()
{
	int list_count = gp_abilities_list_count(ptr);
	gp_error_check(list_count, "Failed to get camera abilities size count.");
	return list_count;
}

int camera_abilities_list::lookup_model(std::string_view model)
{
	int modelIndex = gp_abilities_list_lookup_model(ptr, model.data());
	gp_error_check(modelIndex, "Failed to lookup camera model");
	return modelIndex;
}

void camera_abilities_list::get_abilities(int index, CameraAbilities *abilities)
{
	int ret = gp_abilities_list_get_abilities(ptr, index, abilities);
	gp_error_check(ret, "Failed to get camera abilities.");
}
