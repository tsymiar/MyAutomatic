/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* videodev2.h */
#include <linux/videodev2.h>
#include <netinet/in.h>         /* socket */
#include <arpa/inet.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {
    IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,
} io_method;

struct buffer {
    void* start;
    size_t length;
};

typedef void (*Callback)(void*, const void*, int);

static char* dev_name = NULL;
static io_method io = IO_METHOD_MMAP;
static int fd = -1;
struct buffer* data_buff = NULL;
static unsigned int n_buffers = 0;

static void errno_exit(const char* s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void* arg)
{
    int stat;
    do {
        stat = ioctl(fd, request, arg);
    } while (-1 == stat && EINTR == errno);
    return stat;
}

static void deal_image(Callback callback, void* flag, const void* _ptr, int size)
{
    if (callback != NULL && flag != NULL && size > 0 && _ptr != NULL) {
        callback(flag, _ptr, size);
    }
}

static int read_frame(Callback callback, void* flag)
{
    struct v4l2_buffer buf;
    unsigned int i;

    switch (io) {
    case IO_METHOD_READ:
        if (data_buff == NULL)
            break;
        if (-1 == read(fd, data_buff[0].start, data_buff[0].length)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                errno_exit("read");
            }
        }

        deal_image(callback, flag, data_buff[0].start, data_buff[0].length);

        break;

    case IO_METHOD_MMAP:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit("VIDIOC_DQBUF");
            }
        }

        assert(buf.index < n_buffers);

        deal_image(callback, flag, data_buff[buf.index].start, buf.length);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");

        break;

    case IO_METHOD_USERPTR:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit("VIDIOC_DQBUF");
            }
        }

        for (i = 0; i < n_buffers; ++i)
            if (buf.m.userptr == (unsigned long)data_buff[i].start
                && buf.length == data_buff[i].length)
                break;

        assert(i < n_buffers);

        deal_image(callback, flag, (void*)buf.m.userptr, buf.length);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");

        break;
    }

    return 1;
}

static void mainloop(Callback callback, void* flag)
{
    unsigned int count;

    count = 100;

    while (count-- > 0) {
        for (;;) {
            fd_set fds;
            struct timeval tv;
            int stat;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            /* Timeout. */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            stat = select(fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == stat) {
                if (EINTR == errno)
                    continue;

                errno_exit("select");
            }

            if (0 == stat) {
                fprintf(stderr, "select timeout\n");
                exit(EXIT_FAILURE);
            }

            if (flag == NULL || read_frame(callback, flag))
                break;

            /* EAGAIN - continue select loop. */
        }
    }
}

static void end_capturing(void)
{
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");

        break;
    }
}

static void start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");

        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long)data_buff[i].start;
            buf.length = data_buff[i].length;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");

        break;
    }
}

static void uninit_device(void)
{
    unsigned int i;

    switch (io) {
    case IO_METHOD_READ:
        free(data_buff[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap(data_buff[i].start, data_buff[i].length))
                errno_exit("munmap");
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
            free(data_buff[i].start);
        break;
    }

    free(data_buff);
}

static void init_read(unsigned int buffer_size)
{
    data_buff = calloc(1, sizeof(*data_buff));

    if (!data_buff) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    data_buff[0].length = buffer_size;
    data_buff[0].start = malloc(buffer_size);

    if (!data_buff[0].start) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
}

static void init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                "memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }

    data_buff = calloc(req.count, sizeof(*data_buff));

    if (!data_buff) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        data_buff[n_buffers].length = buf.length;
        data_buff[n_buffers].start = mmap(NULL /* start anywhere */, buf.length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */, fd, buf.m.offset);

        if (MAP_FAILED == data_buff[n_buffers].start)
            errno_exit("mmap");
    }
}

static void init_userp(unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
    unsigned int page_size;

    page_size = getpagesize();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                "user pointer i/o\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    data_buff = calloc(4, sizeof(*data_buff));

    if (!data_buff) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count/*4*/; ++n_buffers) {
        data_buff[n_buffers].length = buffer_size;
        data_buff[n_buffers].start = memalign(/* boundary */page_size, buffer_size);

        if (!data_buff[n_buffers].start) {
            fprintf(stderr, "Out of memory\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void init_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    switch (io) {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf(stderr, "%s does not support read i/o\n", dev_name);
            exit(EXIT_FAILURE);
        }

        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
            exit(EXIT_FAILURE);
        }

        break;
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT");

    /* Note VIDIOC_S_FMT may change width and height. */

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    switch (io) {
    case IO_METHOD_READ:
        init_read(fmt.fmt.pix.sizeimage);
        break;

    case IO_METHOD_MMAP:
        init_mmap();
        break;

    case IO_METHOD_USERPTR:
        init_userp(fmt.fmt.pix.sizeimage);
        break;
    }
}

static void close_device(void)
{
    if (-1 == close(fd))
        errno_exit("close");
    fd = -1;
}

static void open_device(void)
{
    struct stat st;

    if (-1 == stat(dev_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno,
            strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
            strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void routine(Callback callback, void* flag)
{
    dev_name = "/dev/video0";

    open_device();
    init_device();
    start_capturing();

    mainloop(callback, flag);

    end_capturing();
    uninit_device();
    close_device();
}

void save_file(void* fp, const void* buf, int size)
{
    fwrite(buf, size, 1, fp);
}

void send_data(void* fp, const void* buf, int size)
{
    send(*((int*)fp), buf, size, 0);
}

void socket_routine(const char* ip, int port)
{
    const unsigned int maxTimes = 100;
    struct sockaddr_in srvaddr;
    float times = 0;
    const char addrreuse = 0;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(port);
    srvaddr.sin_addr.s_addr = inet_addr(ip);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &addrreuse, sizeof(char));
    while (connect(sock, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) == (-1)) {
        if (times < maxTimes) {
            usleep(100 * (int)pow(2.0f, times));
            times++;
        } else {
            fprintf(stderr, "Retrying to connect finish (times=%d, %s).", (int)times, (errno != 0 ? strerror(errno) : "No error"));
            return;
        }
    }
    routine(send_data, &sock);
    close(sock);
}

void local_routine(char* name)
{
    const char* filename = name;
    FILE* fp = fopen(filename, "wa+");
    if (fp != NULL) {
        routine(save_file, fp);
        fclose(fp);
    }
}

static void usage(FILE* fp, int argc, char** argv)
{
    fprintf(fp, "Usage: %s [options]\n"
        "Options:\n"
        "-d | --device name   Video device name [/dev/video]\n"
        "-f | --file          Save video file to local [filename]\n"
        "-h | --help          Print this message\n"
        "-m | --mmap          Use memory mapped buffers\n"
        "-r | --read          Use read() calls\n"
        "-s | --socket        Run as socket client send data to server [ip:port]\n"
        "-u | --userp         Use application allocated buffers\n"
        "", argv[0]);
}

static const char short_options[] = "d:f:hmrs:u";

static const struct option long_options[] = {
    { "device", required_argument, NULL, 'd' },
    { "file", required_argument, NULL, 'f' },
    { "help", no_argument, NULL, 'h' },
    { "mmap", no_argument, NULL, 'm' },
    { "read", no_argument, NULL, 'r' },
    { "socket", required_argument, NULL, 's' },
    { "userp", no_argument, NULL, 'u' },
    { 0, 0, 0, 0 }
};

int IPv4_verify(char* arg, char* ip, int* port) {
    if (arg == NULL)
        return -1;
    int a, b, c, d, e;
    if (5 == sscanf(arg, "%d.%d.%d.%d:%d", &a, &b, &c, &d, &e)) {
        if (0 <= a && a <= 255 && 0 <= b && b <= 255 && 0 <= c && c <= 255 &&
            0 <= d && d <= 255 && 0 < e && e < 65536) {
            sprintf(ip, "%d.%d.%d.%d", a, b, c, d);
            *port = e;
            return 0;
        }
    }
    return -2;
}

void main_capture(int argc, char* argv[])
{
    char ip[INET_ADDRSTRLEN];
    int port;
    for (;;) {
        int index;
        int c;

        c = getopt_long(argc, argv, short_options, long_options, &index);

        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;

        case 'd':
            dev_name = optarg;
            break;

        case 'f':
            local_routine(optarg);
            break;

        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);

        case 'm':
            io = IO_METHOD_MMAP;
            break;

        case 'r':
            io = IO_METHOD_READ;
            break;

        case 's':
            if (IPv4_verify(optarg, ip, &port) == 0) {
                socket_routine(ip, port);
            }
            break;

        case 'u':
            io = IO_METHOD_USERPTR;
            break;

        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}
