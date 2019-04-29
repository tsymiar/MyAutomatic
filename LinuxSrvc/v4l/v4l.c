#include "v4l2.h"

v4l2_device obj;

void main()
{
    int val = v4l2_init_dev(&obj, NULL);
    if (val == 0) {
        v4l2_set_buffer_queue(&obj, v4l2_mapping_buffers(&obj, 4));
        v4l2_save_image_frame(&obj, "v4l.jpg");
        v4l2_close_dev(&obj);
    }
}
