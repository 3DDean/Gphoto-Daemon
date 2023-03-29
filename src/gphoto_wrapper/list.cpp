#include "gphoto_wrapper/list.h"
#include <gphoto2/gphoto2-list.h>

gphoto_list::gphoto_list()
    : fileList(nullptr)
{
    gp_list_new(&fileList);
}

gphoto_list::~gphoto_list()
{
    if (fileList != nullptr)
    {
        gp_list_unref(fileList);
    }
}

gphoto_list::iterator::iterator(int index, gphoto_list &obj)
    : index(index),
      obj(obj)
{}

gphoto_list::iterator &gphoto_list::iterator::operator++()
{
    index += 1;
    return *this;
}

gphoto_list::name_value_pair gphoto_list::iterator::operator*()
{
    return obj.get_pair(index);
}

bool gphoto_list::iterator::operator!=(const iterator &other) const
{
    return index != other.index;
}

gphoto_list::name_value_pair gphoto_list::get_pair(int index)
{
    const char *name = nullptr;
    const char *value = nullptr;

    gp_list_get_name(fileList, index, &name);
    gp_list_get_value(fileList, index, &value);

    return name_value_pair((name != nullptr) ? name : "", (value != nullptr) ? value : "");
}

std::string_view gphoto_list::get_name(int index)
{
    const char *name = nullptr;
    gp_list_get_name(fileList, index, &name);
    return (name != nullptr) ? name : "";
}

std::string_view gphoto_list::get_value(int index)
{
    const char *value = nullptr;
    gp_list_get_value(fileList, index, &value);
    return (value != nullptr) ? value : "";
}

int gphoto_list::count()
{
    return gp_list_count(fileList);
}

gphoto_list::iterator gphoto_list::begin()
{
    return iterator(0, *this);
}

gphoto_list::iterator gphoto_list::end()
{
    return iterator(count(), *this);
}

gphoto_list::operator CameraList *()
{
    return fileList;
}
