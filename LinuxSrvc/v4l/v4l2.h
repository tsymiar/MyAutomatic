#ifndef _V4L2_H_
#define _V4L2_H_

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
#include <linux/videodev2.h>// v4l2 API

typedef struct _v4l2_struct
{
    int v4l_fd;
    __u32 width;
    __u32 height;
    struct v4l2_capability capability;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format format;
    struct v4l2_buffer argp;
    struct v4l2_requestbuffers req;
    struct _st_buf {
        void *start;
        unsigned int length;
    }*buffer;
} v4l2_device;

#define DEFAULT_DEVICE "/dev/video0"

const int imgw = 640;
const int imgh = 320;

extern int v4l2_init_dev(v4l2_device*, char*);
extern int v4l2_mapping_buffers(v4l2_device*, __u32);
extern unsigned int v4l2_set_buffer_queue(v4l2_device*, __u32);
extern int v4l2_save_image_frame(v4l2_device*, char*);
extern int v4l2_close_dev(v4l2_device*);

int v4l2_get_capability(v4l2_device*);
int v4l2_get_pixel_format(v4l2_device*);
int v4l2_set_pixel_format(v4l2_device*);
int v4l2_try_format(v4l2_device*, __u32);

int v4l2_init_dev(v4l2_device *v4l2_obj, char *device)
{
    if (!device)
        device = DEFAULT_DEVICE;
    if ((v4l2_obj->v4l_fd = open(device, O_RDWR /*| O_NONBLOCK*//*Resource temporarily unavailable*/)) < 0)
    {
        perror("v4l2_open fail");
        return -1;
    }
    if (v4l2_get_capability(v4l2_obj))
        return -2;
    if (v4l2_get_pixel_format(v4l2_obj) <= 0)
        return -3;
    return v4l2_set_pixel_format(v4l2_obj);
}

int v4l2_close_dev(v4l2_device *v4l2_obj)
{
    return close(v4l2_obj->v4l_fd);
}

int v4l2_get_capability(v4l2_device *v4l2_obj)
{
    if (ioctl(v4l2_obj->v4l_fd, VIDIOC_QUERYCAP, &v4l2_obj->capability) < 0)
    {
        perror("v4l2_get_capability fail");
        return -1;
    }
    fprintf(stdout, "VideoCaptureDevice: [%s]\nCardName: %s\nBusinfo: %s\nVersion: %u.%u.%u\n",
        v4l2_obj->capability.driver, v4l2_obj->capability.card, v4l2_obj->capability.bus_info, (v4l2_obj->capability.version >> 16) & 0XFF, (v4l2_obj->capability.version >> 8) & 0xFF, v4l2_obj->capability.version & 0xff);
    return 0;
}

int v4l2_get_pixel_format(v4l2_device *v4l2_obj)
{    
    v4l2_obj->fmtdesc.index = 0;
    v4l2_obj->fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(v4l2_obj->v4l_fd, VIDIOC_ENUM_FMT, &v4l2_obj->fmtdesc) != -1)
    {
        fprintf(stdout, "\t%d. %s", v4l2_obj->fmtdesc.index + 1, v4l2_obj->fmtdesc.description);
        if (v4l2_obj->fmtdesc.pixelformat & v4l2_obj->format.fmt.pix.pixelformat)
        {
            fprintf(stdout, " (desc=fmt).");
        }
        v4l2_obj->fmtdesc.index++;
        fprintf(stdout, "\n");
    }
    return v4l2_obj->fmtdesc.index + 1;
}

int v4l2_set_pixel_format(v4l2_device *v4l2_obj) 
{
    v4l2_obj->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_obj->format.fmt.pix.width = imgw;
    v4l2_obj->format.fmt.pix.height = imgh;
    v4l2_obj->format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    v4l2_obj->format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    int val = ioctl(v4l2_obj->v4l_fd, VIDIOC_S_FMT, &v4l2_obj->format);
    fprintf(stdout, "CurrentDataFormat information:\n\twidth = %d\n\theight = %d\n\tbytesperline = %d\n\tsizeimage = %d\n\tpixelformat = %d\n",
        v4l2_obj->width = v4l2_obj->format.fmt.pix.width, v4l2_obj->height = v4l2_obj->format.fmt.pix.height,
        v4l2_obj->format.fmt.pix.bytesperline, v4l2_obj->format.fmt.pix.sizeimage,
        v4l2_obj->format.fmt.pix.pixelformat);
    return val;
}

int v4l2_try_format(v4l2_device *v4l2_obj, __u32 format)
{
    if (v4l2_obj->v4l_fd < 0)
    {
        fprintf(stderr, "Error using device(%d)!\n", v4l2_obj->v4l_fd);
        return -1;
    }
    v4l2_obj->format.fmt.pix.pixelformat = format;
    if ((ioctl(v4l2_obj->v4l_fd, VIDIOC_TRY_FMT, &v4l2_obj->format) == -1) && (errno == EINVAL))
    {
        perror("Not support format");
        return -2;
    }
    return 0;
}

int v4l2_request_buffers(v4l2_device *v4l2_obj, __u32 count)
{
    v4l2_obj->req.count = count;
    v4l2_obj->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_obj->req.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(v4l2_obj->v4l_fd, VIDIOC_REQBUFS, &v4l2_obj->req)) {
        perror("Request buffer error");
        return 0;
    };
    v4l2_obj->buffer = (struct _st_buf*)calloc(v4l2_obj->req.count, sizeof(*v4l2_obj->buffer));
    return v4l2_obj->req.count;
}

int v4l2_mapping_buffers(v4l2_device *v4l2_obj, __u32 count)
{
    unsigned int i;
    int req_count = v4l2_request_buffers(v4l2_obj, count);
    for (i = 0; i < req_count; ++i) {
        memset(&v4l2_obj->argp, 0, sizeof(v4l2_obj->v4l_fd));
        v4l2_obj->argp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_obj->argp.memory = V4L2_MEMORY_MMAP;
        v4l2_obj->argp.index = i;
        // 查询缓冲区得到起始物理地址和大小  
        if (-1 == ioctl(v4l2_obj->v4l_fd, VIDIOC_QUERYBUF, &v4l2_obj->argp)) {
            perror("Query buffer fail");
            exit(-1);
        }
        v4l2_obj->buffer[i].length = v4l2_obj->argp.length;
        // 映射内存  
        v4l2_obj->buffer[i].start = mmap(NULL, 
            v4l2_obj->argp.length, 
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            v4l2_obj->v4l_fd, 
            v4l2_obj->argp.m.offset);
        if (MAP_FAILED == v4l2_obj->buffer[i].start)
            exit(-1);
    }
    return i;
}

unsigned int v4l2_set_buffer_queue(v4l2_device *v4l2_obj, __u32 count)
{
    unsigned int i;
    for (i = 0; i < count; i++) {
        v4l2_obj->argp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_obj->argp.memory = V4L2_MEMORY_MMAP;
        v4l2_obj->argp.index = i;
        if (-1 == ioctl(v4l2_obj->v4l_fd, VIDIOC_QBUF, &v4l2_obj->argp)) {
            perror("Enqueue data issue");
            return -1;
        };
    }
    return i;
}

int v4l2_save_image_frame(v4l2_device *v4l2_obj, char* flag)
{
    enum v4l2_buf_type type;
    v4l2_obj->argp.type = type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_obj->argp.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(v4l2_obj->v4l_fd, VIDIOC_STREAMON, &v4l2_obj->argp.type)) {
        perror("Start streaming I/O issue");
        return -1;
    }
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(v4l2_obj->v4l_fd, &fds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    select(FD_SETSIZE, (fd_set*)(&v4l2_obj->v4l_fd + 1), &fds, (fd_set *)0, &timeout);
    v4l2_obj->argp.type = type;   
    v4l2_obj->argp.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(v4l2_obj->v4l_fd, VIDIOC_DQBUF, &v4l2_obj->argp)) {
        perror("Dequeue data issue");
        return -2;
    }
    char file[126];
    snprintf(file, strlen(flag) + 1, "%s%d", flag, v4l2_obj->argp.index);
    int yuv = open(file, O_WRONLY | O_CREAT, 0777);
    int result = write(yuv, v4l2_obj->buffer[v4l2_obj->argp.index].start,
        v4l2_obj->buffer[v4l2_obj->argp.index].length
    //    (v4l2_obj->width ? : imgw) * (v4l2_obj->height ? : imgh)
    );
    if (-1 == ioctl(v4l2_obj->v4l_fd, VIDIOC_QBUF, &v4l2_obj->argp)) {
        perror("Re-enqueue data issue");
        return -3;
    }
    int i = 0;
    for (; i < v4l2_obj->req.count; ++i)
        munmap(v4l2_obj->buffer[i].start, v4l2_obj->buffer[i].length);
    close(yuv);
    fprintf(stdout, "Save picture as [ %s ](0x%p).\n", file, v4l2_obj->buffer->start);
    return result;
}

#endif
