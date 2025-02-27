#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifdef USE_JETSON_MULTIMEDIA_API
#include <sys/utsname.h>
// Jetson Multimedia API headers
#include <sys/ioctl.h>
#include <fcntl.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <nvbufsurface.h> // NvBufSurface API
#include <nvbufsurftransform.h> // buffer transform
#include <NvVideoDecoder.h> // video decoder
#include <NvBuffer.h>
#include <linux/videodev2.h> // V4L2
#include <unistd.h>
#include <sys/mman.h>
// Jetson Multimedia API helper functions
bool isJetsonPlatform()
{
    struct utsname buf;
    if (uname(&buf)) return false;
    return std::string(buf.nodename).find("jetson") != std::string::npos;
}

#else
// FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#endif

// Global parameters
GLuint textureID = 0;
int video_width = 0;
int video_height = 0;
std::queue<uint8_t*> frame_queue{};
std::mutex queue_mutex{};
EGLDisplay eglDpy = EGL_NO_DISPLAY;
EGLContext eglCtx = EGL_NO_CONTEXT;

#ifdef USE_JETSON_MULTIMEDIA_API
class JetsonDecoder {
public:
    explicit JetsonDecoder(const char* filename) : m_vfd(-1)
    {
        // Open V4L2 video device
        m_vfd = open("/dev/video0", O_RDWR);
        if (m_vfd < 0) throw std::runtime_error("Open video device failed");

        // Set input format (H.264)
        struct v4l2_format in_fmt = {};
        in_fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        in_fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
        in_fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 1920 * 1080 * 1.5;
        if (ioctl(m_vfd, VIDIOC_S_FMT, &in_fmt) < 0)
            throw std::runtime_error("Set input format failed");

        // Set output format (NV12)
        struct v4l2_format out_fmt = {};
        out_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        out_fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
        out_fmt.fmt.pix_mp.width = 1920;
        out_fmt.fmt.pix_mp.height = 1080;
        if (ioctl(m_vfd, VIDIOC_S_FMT, &out_fmt) < 0)
            throw std::runtime_error("Set output format failed");

        // Request input buffers
        struct v4l2_requestbuffers reqbuf = {};
        reqbuf.count = 4;
        reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        reqbuf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(m_vfd, VIDIOC_REQBUFS, &reqbuf) < 0)
            throw std::runtime_error("Request input buffers failed");

        // Start input stream
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        if (ioctl(m_vfd, VIDIOC_STREAMON, &type) < 0)
            throw std::runtime_error("Start input stream failed");

        // Start output stream
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        if (ioctl(m_vfd, VIDIOC_STREAMON, &type) < 0)
            throw std::runtime_error("Start output stream failed");

        // Load video file
        loadVideoFile(filename);
    }

    NvBufSurface* getFrame()
    {
        // Get captured frame
        struct v4l2_buffer buf = {};
        struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_DMABUF;
        buf.m.planes = planes;
        buf.length = VIDEO_MAX_PLANES;

        if (ioctl(m_vfd, VIDIOC_DQBUF, &buf) < 0)
            return nullptr;

        // Import DMA-BUF
        NvBufSurface* surface = nullptr;
        if (NvBufSurfaceFromFd(planes[0].m.fd, (void**)&surface) != 0)
            return nullptr;

        return surface;
    }

private:
    void loadVideoFile(const char* filename)
    {
        // Load data using standard file IO
        int file_fd = open(filename, O_RDONLY);
        if (file_fd < 0) throw std::runtime_error("Open file failed");

        // Map input buffers
        struct v4l2_buffer qbuf = {};
        struct v4l2_plane qplanes[VIDEO_MAX_PLANES] = {};
        qbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        qbuf.memory = V4L2_MEMORY_MMAP;
        qbuf.m.planes = qplanes;
        qbuf.length = VIDEO_MAX_PLANES;

        for (unsigned int i = 0; i < 4; ++i) {
            qbuf.index = i;
            if (ioctl(m_vfd, VIDIOC_QUERYBUF, &qbuf) < 0)
                throw std::runtime_error("Query buffer failed");

            void* ptr = mmap(NULL, qbuf.m.planes[0].length,
                PROT_READ | PROT_WRITE, MAP_SHARED,
                m_vfd, qbuf.m.planes[0].m.mem_offset);

            ssize_t bytes_read = read(file_fd, ptr, qbuf.m.planes[0].length);
            if (bytes_read < 0) throw std::runtime_error("Read file failed");

            if (ioctl(m_vfd, VIDIOC_QBUF, &qbuf) < 0)
                throw std::runtime_error("Queue buffer failed");
        }
        close(file_fd);
    }

    int m_vfd; // V4L2 device file descriptor
};

void convertNV12toRGB(NvBufSurface* src, uint8_t* dst)
{
    // Make sure the current context is released
    if (eglDpy != EGL_NO_DISPLAY && eglCtx != EGL_NO_CONTEXT) {
        eglMakeCurrent(eglDpy, EGL_NO_SURFACE, EGL_NO_SURFACE, eglCtx);
    }
    // Configure transformation parameters
    NvBufSurfTransformRect src_rect = { 0, 0, src->surfaceList[0].width, src->surfaceList[0].height };
    NvBufSurfTransformRect dst_rect = { 0, 0, src->surfaceList[0].width, src->surfaceList[0].height };
    NvBufSurfTransformParams transformParams = {
        .transform_flag = NVBUFSURF_TRANSFORM_CROP_SRC | NVBUFSURF_TRANSFORM_CROP_DST,
        .transform_flip = NvBufSurfTransform_None,
        .transform_filter = NvBufSurfTransformInter_Default,
        .src_rect = &src_rect,
        .dst_rect = &dst_rect
    };
    // Create destination surface
    NvBufSurfaceCreateParams createParams = {
        .gpuId = 0,
        .width = src->surfaceList[0].width,
        .height = src->surfaceList[0].height,
        .size = 0,
        .isContiguous = 0,
        .colorFormat = NVBUF_COLOR_FORMAT_RGB,
        .layout = NVBUF_LAYOUT_PITCH,
        .memType = NVBUF_MEM_DEFAULT
    };

    NvBufSurface* dstSurface;
    if (NvBufSurfaceCreate(&dstSurface, 1, &createParams) != 0)
        throw std::runtime_error("Create surface failed");

    // Perform transformation
    if (NvBufSurfTransform(src, dstSurface, &transformParams) != 0) {
        NvBufSurfaceDestroy(dstSurface);
        throw std::runtime_error("Transform failed");
    }

    // Map memory
    NvBufSurfaceMap(dstSurface, 0, 0, NVBUF_MAP_READ);
    memcpy(dst, dstSurface->surfaceList[0].dataPtr,
        dstSurface->surfaceList[0].dataSize);

    // Clean up resources
    NvBufSurfaceUnMap(dstSurface, 0, 0);
    NvBufSurfaceDestroy(dstSurface);
}

#else
struct FFmpegDecoder {
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    int video_stream = -1;

    explicit FFmpegDecoder(const char* filename)
    {
        avformat_open_input(&fmt_ctx, filename, nullptr, nullptr);
        avformat_find_stream_info(fmt_ctx, nullptr);

        for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream = i;
                const AVCodec* codec = avcodec_find_decoder(
                    fmt_ctx->streams[i]->codecpar->codec_id);
                codec_ctx = avcodec_alloc_context3(codec);
                avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[i]->codecpar);
                avcodec_open2(codec_ctx, codec, nullptr);

                video_width = codec_ctx->width;
                video_height = codec_ctx->height;
                break;
            }
        }
    }
};

void convertYUVtoRGB(AVFrame* frame, uint8_t* dst)
{
    static SwsContext* sws_ctx = sws_getContext(
        video_width, video_height, AV_PIX_FMT_YUV420P,
        video_width, video_height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    uint8_t* dest[4] = { dst, nullptr, nullptr, nullptr };
    int dest_linesize[4] = { video_width * 3, 0, 0, 0 };
    sws_scale(sws_ctx, frame->data, frame->linesize, 0,
        video_height, dest, dest_linesize);
}
#endif

void videoDecodeThread(const char* filename)
{
#ifdef USE_JETSON_MULTIMEDIA_API
    if (!isJetsonPlatform()) {
        throw std::runtime_error("Jetson platform required");
    }
    JetsonDecoder decoder(filename);
    while (true) {
        NvBufSurface* buffer = decoder.getFrame();
        if (!buffer) break;

        uint8_t* rgb = new uint8_t[video_width * video_height * 3];
        convertNV12toRGB(buffer, rgb);

        std::lock_guard<std::mutex> lock(queue_mutex);
        frame_queue.push(rgb);
    }
#else
    FFmpegDecoder decoder(filename);
    AVPacket pkt;
    while (av_read_frame(decoder.fmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index == decoder.video_stream) {
            avcodec_send_packet(decoder.codec_ctx, &pkt);
            AVFrame* frame = av_frame_alloc();
            if (avcodec_receive_frame(decoder.codec_ctx, frame) == 0) {
                uint8_t* rgb = new uint8_t[video_width * video_height * 3];
                convertYUVtoRGB(frame, rgb);

                std::lock_guard<std::mutex> lock(queue_mutex);
                frame_queue.push(rgb);
            }
            av_frame_free(&frame);
        }
        av_packet_unref(&pkt);
    }
#endif
}

void updateTexture(uint8_t* data)
{
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        video_width, video_height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, data);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video file path>" << std::endl;
        return -1;
    }
    putenv((char*)"DISPLAY=:0");

    // EGL setup
    eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDpy == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        return -1;
    }
    EGLint major, minor;
    if (!eglInitialize(eglDpy, &major, &minor)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        return -1;
    }
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE
    };
    EGLConfig eglConfig;
    EGLint numConfigs;
    if (!eglChooseConfig(eglDpy, configAttribs, &eglConfig, 1, &numConfigs)) {
        std::cerr << "Failed to choose EGL config" << std::endl;
        return -1;
    }
    // EGL context
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    eglCtx = eglCreateContext(eglDpy, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (eglCtx == EGL_NO_CONTEXT) {
        std::cerr << "Failed to create EGL context" << std::endl;
        return -1;
    }
    EGLint pbufferAttribs[] = {
        EGL_WIDTH, 1280,
        EGL_HEIGHT, 720,
        EGL_NONE
    };
    EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy, eglConfig, pbufferAttribs);
    if (eglSurf == EGL_NO_SURFACE) {
        std::cerr << "Failed to create EGL surface" << std::endl;
        return -1;
    }
    if (!eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx)) {
        std::cerr << "Failed to make context current" << std::endl;
        return -1;
    }

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Video Player", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Start the video decoding thread
    std::thread decoderThread(videoDecodeThread, argv[1]);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        if (!frame_queue.empty()) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            uint8_t* frame = frame_queue.front();
            frame_queue.pop();

            updateTexture(frame);
            delete[] frame;

            glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex2f(-1, -1);
            glTexCoord2f(1, 1); glVertex2f(1, -1);
            glTexCoord2f(1, 0); glVertex2f(1, 1);
            glTexCoord2f(0, 0); glVertex2f(-1, 1);
            glEnd();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    decoderThread.join();
    glfwTerminate();

    if (eglCtx != EGL_NO_CONTEXT) {
        eglDestroyContext(eglDpy, eglCtx);
    }
    if (eglDpy != EGL_NO_DISPLAY) {
        eglTerminate(eglDpy);
    }
    return 0;
}
