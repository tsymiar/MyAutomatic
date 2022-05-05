#ifndef _LINUX_SNAP_H_
#define _LINUX_SNAP_H_

#include <stdio.h>
#include <stdlib.h>      // stdio.h and stdlib.h are needed by perror function
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>       // O_RDWR
#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#include <sys/mman.h>    // unistd.h and sys/mman.h are needed by mmap function
#include <stdbool.h>
#include <sys/ioctl.h>
#include <bits/types/struct_timespec.h>
#include <bits/types/struct_timeval.h>
#include <linux/videodev2.h>// v4l2 API

#define DEFAULT_DEVICE "/dev/video0"

typedef struct _snap_struct {
    int v4l_fd;
    __u32 width;
    __u32 height;
    struct v4l2_capability capability;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format format;
    struct v4l2_buffer argp;
    struct v4l2_requestbuffers req;
    struct _st_buf {
        void* start;
        unsigned int size;
    }*buffer;
} snap_device;

typedef struct _st_image_pixel {
    unsigned int img_w;
    unsigned int img_h;
} image_pixel;

// public
int snap_init_dev(snap_device*, image_pixel, char*);
unsigned int snap_mapping_buffers(snap_device*, __u32);
int snap_set_buffer_queue(snap_device*, __u32);
int snap_save_image_frame(snap_device*, const char*);
int snap_close_dev(snap_device*);
// private
int snap_get_capability(snap_device*);
int snap_get_pixel_format(snap_device*);
int snap_set_pixel_format(snap_device*, image_pixel);
int snap_try_format(snap_device*, __u32);

int snap_init_dev(snap_device* snap_dev, image_pixel img_size, char* device)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    if (device == NULL)
        device = (char*)DEFAULT_DEVICE;
    if ((snap_dev->v4l_fd = open(device, O_RDWR /*| O_NONBLOCK*//*Resource temporarily unavailable*/)) < 0) {
        perror("snap_init_dev open " DEFAULT_DEVICE " fail!");
        return -2;
    }
    if (snap_get_capability(snap_dev))
        return -3;
    if (snap_get_pixel_format(snap_dev) <= 0)
        return -4;
    return snap_set_pixel_format(snap_dev, img_size);
}

int snap_get_capability(snap_device* snap_dev)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    if (ioctl(snap_dev->v4l_fd, VIDIOC_QUERYCAP, &snap_dev->capability) < 0) {
        perror("snap_get_capability fail");
        return -1;
    }
    fprintf(stdout, "VideoCaptureDevice: [%s]\nCardName: %s\nBusinfo: %s\nVersion: %u.%u.%u\n",
        snap_dev->capability.driver, snap_dev->capability.card, snap_dev->capability.bus_info,
        (snap_dev->capability.version >> 16) & 0XFF, (snap_dev->capability.version >> 8) & 0xFF,
        snap_dev->capability.version & 0xff);
    return 0;
}

int snap_get_pixel_format(snap_device* snap_dev)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    snap_dev->fmtdesc.index = 0;
    snap_dev->fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(snap_dev->v4l_fd, VIDIOC_ENUM_FMT, &snap_dev->fmtdesc) != -1) {
        fprintf(stdout, "\t%d. %s", snap_dev->fmtdesc.index + 1, snap_dev->fmtdesc.description);
        if (snap_dev->fmtdesc.pixelformat & snap_dev->format.fmt.pix.pixelformat) {
            fprintf(stdout, " (fourcc&pixfmt).");
        }
        snap_dev->fmtdesc.index++;
        fprintf(stdout, "\n");
    }
    return snap_dev->fmtdesc.index + 1;
}

int snap_set_pixel_format(snap_device* snap_dev, image_pixel img_size)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    snap_dev->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    snap_dev->format.fmt.pix.width = img_size.img_w;
    snap_dev->format.fmt.pix.height = img_size.img_h;
    snap_dev->format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    snap_dev->format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    int val = ioctl(snap_dev->v4l_fd, VIDIOC_S_FMT, &snap_dev->format);
    fprintf(stdout,
        "CurrentDataFormat information:\n\twidth = %d\n\theight = %d\n\tbytesperline = %d\n\tsizeimage = %d\n"
        "\tpixelformat = %d\n",
        snap_dev->width = snap_dev->format.fmt.pix.width, snap_dev->height = snap_dev->format.fmt.pix.height,
        snap_dev->format.fmt.pix.bytesperline, snap_dev->format.fmt.pix.sizeimage,
        snap_dev->format.fmt.pix.pixelformat);
    return val;
}

int snap_try_format(snap_device* snap_dev, __u32 format)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    if (snap_dev->v4l_fd < 0) {
        fprintf(stderr, "Error using device(%d)!\n", snap_dev->v4l_fd);
        return -1;
    }
    snap_dev->format.fmt.pix.pixelformat = format;
    if ((ioctl(snap_dev->v4l_fd, VIDIOC_TRY_FMT, &snap_dev->format) == -1) && (errno == EINVAL)) {
        perror("Pixel format not support");
        return -2;
    }
    return 0;
}

int snap_request_buffers(snap_device* snap_dev, __u32 count)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    snap_dev->req.count = count;
    snap_dev->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    snap_dev->req.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(snap_dev->v4l_fd, VIDIOC_REQBUFS, &snap_dev->req)) {
        perror("Alloc buffer memory issue");
        return 0;
    };
    snap_dev->buffer = (struct _st_buf*)calloc(snap_dev->req.count, sizeof(*snap_dev->buffer));
    return snap_dev->req.count;
}

unsigned int snap_mapping_buffers(snap_device* snap_dev, __u32 count)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    unsigned int i;
    int reqcount = snap_request_buffers(snap_dev, count);
    for (i = 0; i < reqcount; ++i) {
        memset(&snap_dev->argp, 0, sizeof(snap_dev->v4l_fd));
        snap_dev->argp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        snap_dev->argp.memory = V4L2_MEMORY_MMAP;
        snap_dev->argp.index = i;
        // query buffer to get start of hardware address
        if (-1 == ioctl(snap_dev->v4l_fd, VIDIOC_QUERYBUF, &snap_dev->argp)) {
            perror("Trans VIDIOC_REQBUFS buffer to point fail");
            exit(-1);
        }
        snap_dev->buffer[i].size = snap_dev->argp.length;
        snap_dev->buffer[i].start = mmap(NULL,
            snap_dev->argp.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            snap_dev->v4l_fd,
            snap_dev->argp.m.offset);
        if (MAP_FAILED == snap_dev->buffer[i].start)
            exit(-1);
    }
    return i;
}

int snap_set_buffer_queue(snap_device* snap_dev, __u32 count)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    int set;
    for (set = 0; set < count; set++) {
        snap_dev->argp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        snap_dev->argp.memory = V4L2_MEMORY_MMAP;
        snap_dev->argp.index = (unsigned)set;
        if (-1 == ioctl(snap_dev->v4l_fd, VIDIOC_QBUF, &snap_dev->argp)) {
            perror("Get data from buffer issue");
            return -1;
        };
    }
    return set;
}

int snap_save_image_frame(snap_device* snap_dev, const char* prefix)
{
    if (snap_dev == NULL) {
        perror("snap_device is null!");
        return -1;
    }
    enum v4l2_buf_type type;
    snap_dev->argp.type = type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    snap_dev->argp.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(snap_dev->v4l_fd, VIDIOC_STREAMON, &snap_dev->argp.type)) {
        perror("Start streaming I/O issue");
        return -1;
    }
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(snap_dev->v4l_fd, &fds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    select(FD_SETSIZE, (fd_set*)(&snap_dev->v4l_fd + 1), &fds, (fd_set*)0, &timeout);
    snap_dev->argp.type = type;
    snap_dev->argp.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(snap_dev->v4l_fd, VIDIOC_DQBUF, &snap_dev->argp)) {
        perror("Put data back to buffer issue");
        return -2;
    }
    char file[128];
    sprintf(file, "%s_%d", prefix, snap_dev->argp.index);
    int yuv = open(file, O_WRONLY | O_CREAT, 0666);
    ssize_t result = write(yuv, snap_dev->buffer[snap_dev->argp.index].start,
        snap_dev->buffer[snap_dev->argp.index].size
    );
    if (-1 == ioctl(snap_dev->v4l_fd, VIDIOC_QBUF, &snap_dev->argp)) {
        perror("Re-get buffer data issue");
        return -3;
    }
    int i = 0;
    for (; i < snap_dev->req.count; ++i)
        munmap(snap_dev->buffer[i].start, snap_dev->buffer[i].size);
    close(yuv);
    fprintf(stdout, "Save picture as [ %s ](0x%p).\n", file, snap_dev->buffer->start);
    return result;
}

int snap_close_dev(snap_device* snap_dev)
{
    if (snap_dev == NULL || snap_dev->v4l_fd < 0) {
        fprintf(stdout, "Not avalid to close device!\n");
        return -1;
    }
    return close(snap_dev->v4l_fd);
}

int snap_image_test(const char* filename)
{
    snap_device snap_dev;
    image_pixel img_size = {
        .img_w = 640,
        .img_h = 480
    };
    int val = snap_init_dev(&snap_dev, img_size, NULL);
    if (val == 0) {
        if (snap_set_buffer_queue(&snap_dev, snap_mapping_buffers(&snap_dev, 4)) <= 0)
            return -1;
        if (snap_save_image_frame(&snap_dev, filename) <= 0)
            return -2;
        snap_close_dev(&snap_dev);
    }
    return 0;
}

#endif // _LINUX_SNAP_H_
