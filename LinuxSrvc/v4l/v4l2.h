#ifndef _V4L2_H_
#define _V4L2_H_

#include <stdio.h>
#include <stdlib.h>      //stdio.h and stdlib.h are needed by perror function
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>       //O_RDWR
#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#include <sys/mman.h>    //unistd.h and sys/mman.h are needed by mmap function
#include <stdbool.h>     //false and true
#include <sys/ioctl.h>
#include <linux/videodev2.h>//v4l2 API

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
    }*buffers;
} v4l2_device;


extern int v4l2_open_dev(char*, v4l2_device*);
extern int v4l2_close_dev(v4l2_device*);
extern int v4l2_get_capability(v4l2_device*);
extern int v4l2_get_pixel_format(v4l2_device*);
int v4l2_try_format(v4l2_device*, __u32);

#define DEFAULT_DEVICE "/dev/video0"
#define ToString(x) #x

int v4l2_open_dev(char *device, v4l2_device *v4l2_obj)
{
    if (!device)
        device = DEFAULT_DEVICE;
    if ((v4l2_obj->v4l_fd = open(device, O_RDWR)) < 0)
    {
        perror("v4l2_open fail");
        return -1;
    }
    if (v4l2_get_capability(v4l2_obj))
        return -2;
    if (v4l2_get_pixel_format(v4l2_obj))
        return -3;
    return 0;
}

int v4l2_close_dev(v4l2_device *v4l2_obj)
{
    close(v4l2_obj->v4l_fd);
    return 0;
}

int v4l2_get_capability(v4l2_device *v4l2_obj)
{
    if (ioctl(v4l2_obj->v4l_fd, VIDIOC_QUERYCAP, &v4l2_obj->capability) < 0)
    {
        perror("v4l2_get_capability fail");
        return -1;
    }
    printf("VideoCaptureDevice: [%s]\nCardName: %s\nBusinfo: %s\nVersion: %u.%u.%u\n",
        v4l2_obj->capability.driver, v4l2_obj->capability.card, v4l2_obj->capability.bus_info, (v4l2_obj->capability.version >> 16) & 0XFF, (v4l2_obj->capability.version >> 8) & 0xFF, v4l2_obj->capability.version & 0xff);
    return 0;
}

int v4l2_get_pixel_format(v4l2_device *v4l2_obj)
{
    v4l2_obj->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(v4l2_obj->v4l_fd, VIDIOC_G_FMT, &v4l2_obj->format);
    printf("CurrentDataFormat information:\n\twidth = %d\n\theight = %d\n\tbytesperline = %d\n\tsizeimage = %d\n\tpixelformat = %d\n",
        v4l2_obj->width = v4l2_obj->format.fmt.pix.width, v4l2_obj->height = v4l2_obj->format.fmt.pix.height, v4l2_obj->format.fmt.pix.bytesperline, v4l2_obj->format.fmt.pix.sizeimage, v4l2_obj->format.fmt.pix.pixelformat);
    v4l2_obj->fmtdesc.index = 0;
    v4l2_obj->fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(v4l2_obj->v4l_fd, VIDIOC_ENUM_FMT, &v4l2_obj->fmtdesc) != -1)
    {
        printf("\t%d. %s", v4l2_obj->fmtdesc.index + 1, v4l2_obj->fmtdesc.description);
        if (v4l2_obj->fmtdesc.pixelformat & v4l2_obj->format.fmt.pix.pixelformat)
        {
            printf(" (desc=fmt).");
        }
        v4l2_obj->fmtdesc.index++;
        printf("\n");
    }
}

int v4l2_try_format(v4l2_device *v4l2_obj, __u32 format)
{
    if (v4l2_obj->v4l_fd < 0)
    {
        printf("Error using device(%d)!\n", v4l2_obj->v4l_fd);
        return -1;
    }
    v4l2_obj->format.fmt.pix.pixelformat = format;
    if ((ioctl(v4l2_obj->v4l_fd, VIDIOC_TRY_FMT, &v4l2_obj->format) == -1) && (errno == EINVAL))
    {
        printf("Not support format!");
        return -2;
    }
    return 0;
}

int v4l2_mapping_buffers(v4l2_device *v4l2_obj, __u32 count)
{
    v4l2_obj->req.count = count;
    v4l2_obj->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_obj->req.memory = V4L2_MEMORY_MMAP;
    ioctl(v4l2_obj->v4l_fd, VIDIOC_REQBUFS, &v4l2_obj->req);
    v4l2_obj->buffers = (struct _st_buf*)calloc(v4l2_obj->req.count, sizeof(*v4l2_obj->buffers));
    for (unsigned int i = 0; i < v4l2_obj->req.count; ++i) {
        memset(&v4l2_obj->argp, 0, sizeof(v4l2_obj->v4l_fd));
        v4l2_obj->argp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_obj->argp.memory = V4L2_MEMORY_MMAP;
        v4l2_obj->argp.index = i;
        // 查询缓冲区得到起始物理地址和大小  
        if (-1 == ioctl(v4l2_obj->v4l_fd, VIDIOC_QUERYBUF, &v4l2_obj->argp))
            exit(-1);
        v4l2_obj->buffers[i].length = v4l2_obj->argp.length;
        // 映射内存  
        v4l2_obj->buffers[i].start = mmap(NULL, v4l2_obj->argp.length, PROT_READ | PROT_WRITE, MAP_SHARED,
            v4l2_obj->v4l_fd, v4l2_obj->argp.m.offset);
        if (MAP_FAILED == v4l2_obj->buffers[i].start)
            exit(-1);
    }
    return count;
}

unsigned int v4l2_set_queue(v4l2_device *v4l2_obj, __u32 count)
{
    unsigned int i;
    enum v4l2_buf_type type;
    for (i = 0; i < count; i++) {
        v4l2_obj->argp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_obj->argp.memory = V4L2_MEMORY_MMAP;
        v4l2_obj->argp.index = i;
        ioctl(v4l2_obj->v4l_fd, VIDIOC_QBUF, &v4l2_obj->argp);
    }
    v4l2_obj->argp.type = type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_obj->argp.memory = V4L2_MEMORY_MMAP;
    ioctl(v4l2_obj->v4l_fd, VIDIOC_STREAMON, &v4l2_obj->argp.type);
    return i;
}

int v4l2_save_got_frame(v4l2_device *v4l2_obj, char* flag)
{
    char file[126];
    snprintf(file, strlen(flag) + 1, "%s%d", flag, v4l2_obj->argp.index);
    int yuv = open(file, O_WRONLY | O_CREAT, 00700);
    int result = write(yuv, v4l2_obj->buffers[v4l2_obj->argp.index].start,
        (v4l2_obj->width ? : 640) * (v4l2_obj->height ? : 480));
    close(yuv);
    printf("Save picture as [ %s ](%x).\n", file, v4l2_obj->buffers);
    return result;
}

#endif
