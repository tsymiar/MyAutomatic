#include "v4l2.h"

int main()
{
    v4l2_device obj;
    int val = v4l2_init_dev(&obj, NULL);
    if (val == 0) {
        if (v4l2_set_buffer_queue(&obj, v4l2_mapping_buffers(&obj, 4)) <= 0)
            return -1;
        if (v4l2_save_image_frame(&obj, "v4l2.yuv") <= 0)
            return-2;
        v4l2_close_dev(&obj);
    }
    return 0;
}
