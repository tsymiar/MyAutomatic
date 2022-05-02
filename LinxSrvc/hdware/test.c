#ifdef GPIO
#include "gpio.h"
#elif defined(ME9S)
#include "me909s.h"
#elif defined(FIFO) 
#include "fifo.h"
#elif defined(V4L2) || defined(CAPTURE)
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
    fifo_main(NULL);
#elif defined(V4L2)
    v4l2_test("image");
#elif defined(CAPTURE)
    main_capture(argc, argv);
#endif
    return 0;
}
