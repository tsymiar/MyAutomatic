#include "v4l2.h"

v4l2_device dev;

void main()
{
	v4l2_open_dev(DEFAULT_DEVICE, &dev);
	v4l2_mapping_buffers(&dev, 4);
	v4l2_set_queue(&dev, 4);
	v4l2_save_got_frame(&dev, "v4l.jpg");
	v4l2_close_dev(&dev);
}
