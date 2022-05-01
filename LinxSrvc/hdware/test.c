#ifdef GPIO
#include "gpio.h"
#elif defined(ME9S)
#include "me909s.h"
#elif defined(V4L2)
#include "v4l2.h"
#elif defined(CAPTURE)
#include "routine.h"
#else
#error compile command unsupported
#endif

int main(int argc, char** argv)
{
#ifdef GPIO
#elif defined(ME9S)
    mes_main(argc, argv);
#elif defined(V4L2)
    v4l2_device dev;
    int val = v4l2_init_dev(&dev, NULL);
    if (val == 0) {
        if (v4l2_set_buffer_queue(&dev, v4l2_mapping_buffers(&dev, 4)) <= 0)
            return -1;
        if (v4l2_save_image_frame(&dev, "image") <= 0)
            return -2;
        v4l2_close_dev(&dev);
    }
#elif defined(CAPTURE)
    main_capture(argc, argv);
#endif
    return 0;
}
