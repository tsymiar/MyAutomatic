#ifdef GPIO
#include "gpio.h"
#elif defined(ME9S)
#include "me909s.h"
#elif defined(PIPE_FIFO)
#include <stdlib.h>
#include "pipefifo.h"
#elif defined(VIDEO) || defined(SNAP)
#include "test.h"
#else
#error compile command unsupported
#endif

int main(int argc, char** argv)
{
#ifdef GPIO
    if (argc > 3) {
        set_gpio_by_direction(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    } else if (argc == 3) {
        set_gpio_export(atoi(argv[1]), atoi(argv[2]));
    } else if (argc == 2) {
        gpio_hint(get_gpio_value(atoi(argv[1])));
    } else {
        gpio_hint(-1);
    }
#elif defined(ME9S)
    meat_main();
#elif defined(PIPE_FIFO)
    if (argc > 2) {
        pipe_fifo_test((long long)atoi(argv[1]), atoi(argv[2]));
    } else {
        pipe_fifo_test(-1, 0);
    }
#elif defined(VIDEO)
    video_capture(argc, argv);
#elif defined(SNAP)
    snap_image_test("image");
#endif
    return 0;
}
