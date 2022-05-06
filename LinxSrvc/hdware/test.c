#ifdef GPIO
#include "gpio.h"
#elif defined(ME9S)
#include "me909s.h"
#elif defined(FIFO) 
#include "fifo.h"
#elif defined(VIDEO) || defined(SNAP)
#include "test.h"
#else
#error compile command unsupported
#endif

int main(int argc, char** argv)
{
#ifdef GPIO
    //
#elif defined(ME9S)
    mes_main(argc, argv);
#elif defined(FIFO)
    fifo_main((long long)argc);
#elif defined(VIDEO)
    main_capture(argc, argv);
#elif defined(SNAP)
    snap_image_test("image");
#endif
    return 0;
}
