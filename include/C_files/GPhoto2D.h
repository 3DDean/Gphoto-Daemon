#include <gphoto2/gphoto2-camera.h>

typedef size_t CameraID; 


int gphoto2_initialize();
int gphoto2_shutdown();
int gphoto2_detect_cameras();

int gphoto2_detected_camera_count();
int gphoto2_open_camera_count();





//TODO ADD function for loading by name and by index
int gphoto2_open_camera_index(uint8_t index, CameraID* output);
int gphoto2_open_camera_str(char* str, CameraID* output);
int gphoto2_exit_camera(Camera* camera);
int gphoto2_wait_for_event(Camera* camera, int timeout);

int gphoto2_capture(Camera* camera, CameraFilePath *path);
int gphoto2_capture_preview(Camera* camera);
