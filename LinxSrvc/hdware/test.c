#ifdef V4L2
#include "v4l2.h"
#elif defined(GPIO)
#include "gpio.h"
#elif defined(ME9S)
#include "me909s.h"
#else
#error compile command unsupported
#endif

int main(int argc, char** argv)
{
#ifdef V4L2
    v4l2_device obj;
    int val = v4l2_init_dev(&obj, NULL);
    if (val == 0) {
        if (v4l2_set_buffer_queue(&obj, v4l2_mapping_buffers(&obj, 4)) <= 0)
            return -1;
        if (v4l2_save_image_frame(&obj, "image") <= 0)
            return -2;
        v4l2_close_dev(&obj);
    }
#elif defined(GPIO)

#elif defined(ME9S)
    mes_main(argc, argv);
#endif
    return 0;
}
