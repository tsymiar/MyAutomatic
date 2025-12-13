#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#ifdef USE_JETSON_MULTIMEDIA_API
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
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
// GBM
#include <sys/mman.h>
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// Jetson Multimedia API helper functions
bool isJetsonPlatform()
{
    struct utsname buf;
    if (uname(&buf)) return false;
    return std::string(buf.nodename).find("jetson") != std::string::npos;
}
#else
#include <SDL2/SDL.h>
// FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#endif

// Global parameters
int video_width = 0;
int video_height = 0;
std::queue<uint8_t*> g_frame_queue{};
std::mutex queue_mutex{};

#ifdef USE_JETSON_MULTIMEDIA_API

struct drm_context {
    int fd = -1;
    drmModeRes* res = nullptr;
    drmModeConnector* conn = nullptr;
    uint32_t crtc_id = 0;
    drmModeModeInfo mode{};
    gbm_device* gbm_dev = nullptr;
    gbm_surface* gbm_surf = nullptr;
} g_drm;

GLuint textureID = 0;
EGLDisplay eglDpy = EGL_NO_DISPLAY;
EGLContext eglCtx = EGL_NO_CONTEXT;
EGLSurface eglSurf = EGL_NO_SURFACE;

void init_drm()
{
    for (int i = 0; i < 2; ++i) {  // card0 card1
        std::string dev_path = "/dev/dri/card" + std::to_string(i);
        g_drm.fd = open(dev_path.c_str(), O_RDWR | O_CLOEXEC);
        if (g_drm.fd >= 0) {
            std::cout << "Opened DRM device: " << dev_path << std::endl;
            break;
        }
    }

    if (g_drm.fd < 0) {
        throw std::runtime_error("Failed to open any DRM device: "
            + std::string(strerror(errno)));
    }
    g_drm.res = drmModeGetResources(g_drm.fd);
    if (!g_drm.res) {
        close(g_drm.fd);
        throw std::runtime_error("drmModeGetResources failed");
    }
    // seek valid connector
    bool connector_found = false;
    for (int i = 0; i < g_drm.res->count_connectors; ++i) {
        g_drm.conn = drmModeGetConnector(g_drm.fd, g_drm.res->connectors[i]);
        if (!g_drm.conn) continue;

        std::cout << "Checking connector " << g_drm.conn->connector_id
            << " (Type: " << g_drm.conn->connector_type
            << ", Status: " << (g_drm.conn->connection == DRM_MODE_CONNECTED
                ? "Connected" : "Disconnected")
            << ")\n";

        // check connect
        if (g_drm.conn->connection == DRM_MODE_CONNECTED
            && g_drm.conn->count_modes > 0) {
            // select best mode
            bool preferred_found = false;
            for (int m = 0; m < g_drm.conn->count_modes; ++m) {
                if (g_drm.conn->modes[m].type & DRM_MODE_TYPE_PREFERRED) {
                    g_drm.mode = g_drm.conn->modes[m];
                    preferred_found = true;
                    std::cout << "Selected preferred mode: "
                        << g_drm.mode.hdisplay << "x" << g_drm.mode.vdisplay
                        << "@" << g_drm.mode.vrefresh << "Hz\n";
                    break;
                }
            }

            // select largest plex if not preferred
            if (!preferred_found) {
                uint32_t max_area = 0;
                for (int m = 0; m < g_drm.conn->count_modes; ++m) {
                    const uint32_t area = g_drm.conn->modes[m].hdisplay
                        * g_drm.conn->modes[m].vdisplay;
                    if (area > max_area) {
                        max_area = area;
                        g_drm.mode = g_drm.conn->modes[m];
                    }
                }
                std::cout << "Selected largest mode: "
                    << g_drm.mode.hdisplay << "x" << g_drm.mode.vdisplay
                    << "@" << g_drm.mode.vrefresh << "Hz\n";
            }

            // get CRTC
            if (g_drm.conn->encoder_id) {
                drmModeEncoder* enc = drmModeGetEncoder(g_drm.fd, g_drm.conn->encoder_id);
                if (enc) {
                    g_drm.crtc_id = enc->crtc_id;
                    drmModeFreeEncoder(enc);
                }
            }

            if (!g_drm.crtc_id) {
                // select first useful CRTC
                g_drm.crtc_id = g_drm.res->crtcs[0];
            }

            connector_found = true;
            break;
        }

        drmModeFreeConnector(g_drm.conn);
        g_drm.conn = nullptr;
    }

    if (!connector_found) {
        close(g_drm.fd);
        throw std::runtime_error("No active connector with valid modes found");
    }
    // create GBM device
    g_drm.gbm_dev = gbm_create_device(g_drm.fd);
    if (!g_drm.gbm_dev) {
        close(g_drm.fd);
        throw std::runtime_error("Failed to create GBM device");
    }

    std::cout << "DRM initialization successful!\n"
        << "Resolution: " << g_drm.mode.hdisplay << "x" << g_drm.mode.vdisplay << "\n"
        << "Refresh Rate: " << g_drm.mode.vrefresh << "Hz\n";
}
void cleanup_drm()
{
    if (g_drm.conn) {
        drmModeFreeConnector(g_drm.conn);
        g_drm.conn = nullptr;
    }
    if (g_drm.res != nullptr) {
        drmModeFreeResources(g_drm.res);
        g_drm.res = nullptr;
    }
    if (g_drm.gbm_dev) {
        gbm_device_destroy(g_drm.gbm_dev);
        g_drm.gbm_dev = nullptr;
    }
    if (g_drm.fd >= 0) {
        close(g_drm.fd);
        g_drm.fd = -1;
    }
}
void init_egl()
{
    // get EGL display
    eglDpy = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, g_drm.gbm_dev, NULL);
    if (eglDpy == EGL_NO_DISPLAY) {
        std::cerr << "EGL Error: Failed to get display ("
            << eglGetError() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    EGLint major, minor;
    if (!eglInitialize(eglDpy, &major, &minor)) {
        std::cerr << "EGL Error: Initialize failed ("
            << eglGetError() << ")" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "EGL Version: " << major << "." << minor << std::endl;

    // config EGL attributes
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig cfg;
    EGLint count;
    eglChooseConfig(eglDpy, config_attribs, &cfg, 1, &count);

    // create GBM surface
    g_drm.gbm_surf = gbm_surface_create(g_drm.gbm_dev,
        g_drm.mode.hdisplay, g_drm.mode.vdisplay,
        GBM_FORMAT_XRGB8888, GBM_BO_USE_RENDERING);

    // create EGL surface
    eglSurf = eglCreatePlatformWindowSurface(eglDpy, cfg, g_drm.gbm_surf, NULL);

    // create EGL context
    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    eglCtx = eglCreateContext(eglDpy, cfg, EGL_NO_CONTEXT, ctx_attribs);

    eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);

    // init GL texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}
void render_frame()
{
    struct gbm_bo* bo = gbm_surface_lock_front_buffer(g_drm.gbm_surf);
    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t pitch = gbm_bo_get_stride(bo);

    // create DRM framebuffer
    uint32_t fb;
    drmModeAddFB(g_drm.fd, g_drm.mode.hdisplay, g_drm.mode.vdisplay,
        24, 32, pitch, handle, &fb);

    // set CRTC
    drmModeSetCrtc(g_drm.fd, g_drm.crtc_id, fb,
        0, 0, &g_drm.conn->connector_id, 1, &g_drm.mode);

    // swap buffer
    eglSwapBuffers(eglDpy, eglSurf);
    gbm_surface_release_buffer(g_drm.gbm_surf, bo);
}

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
        int vi_fd = open(filename, O_RDONLY);
        if (vi_fd < 0) throw std::runtime_error("Open file failed");

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
            if (ptr == MAP_FAILED)
                throw std::runtime_error("mmap failed for buffer");

            off_t file_size = lseek(vi_fd, 0, SEEK_END);
            if (file_size < 0) {
                munmap(ptr, qbuf.m.planes[0].length);
                throw std::runtime_error("Failed to get file size");
            }
            if (lseek(vi_fd, 0, SEEK_SET) < 0) {
                munmap(ptr, qbuf.m.planes[0].length);
                throw std::runtime_error("Failed to seek file");
            }
            // compute how many bytes remain in the file from current offset
            off_t current_pos = lseek(vi_fd, 0, SEEK_CUR);
            if (current_pos < 0) {
                munmap(ptr, qbuf.m.planes[0].length);
                throw std::runtime_error("Failed to get current file position");
            }
            ssize_t buf_len = static_cast<ssize_t>(qbuf.m.planes[0].length);
            ssize_t bytes_available = static_cast<ssize_t>(file_size - current_pos);
            ssize_t bytes_need = std::min(buf_len, bytes_available);
            if (bytes_need <= 0) {
                // nothing to read for this buffer
                munmap(ptr, qbuf.m.planes[0].length);
                break;
            }

            ssize_t total = 0;
            {
                // cache mapped length and use safe signed/unsigned conversions
                size_t map_len = static_cast<size_t>(qbuf.m.planes[0].length);
                ssize_t local_buf_len = static_cast<ssize_t>(map_len);
                while (total < bytes_need) {
                    ssize_t to_read = bytes_need - total;
                    // ensure we never request more than the remaining buffer space
                    if (to_read > (local_buf_len - total)) to_read = local_buf_len - total;
                    if (to_read <= 0) {
                        munmap(ptr, map_len);
                        throw std::runtime_error("No space left in buffer");
                    }
                    // perform read with EINTR handling and allow partial reads
                    ssize_t bytes = 0;
                    while (true) {
                        bytes = ::read(vi_fd, static_cast<uint8_t*>(ptr) + total, static_cast<size_t>(to_read));
                        if (bytes < 0) {
                            if (errno == EINTR) continue; // retry on interruption
                            munmap(ptr, map_len);
                            throw std::runtime_error("Read file failed");
                        }
                        break;
                    }
                    if (bytes == 0) {
                        // EOF reached
                        break;
                    }
                    total += bytes;
                    if (total > local_buf_len) {
                        munmap(ptr, map_len);
                        throw std::runtime_error("Beyond buffer size after read");
                    }
                }
            }

            // set bytesused for the plane before queuing
            qbuf.m.planes[0].bytesused = static_cast<__u32>(total);

            if (ioctl(m_vfd, VIDIOC_QBUF, &qbuf) < 0) {
                munmap(ptr, qbuf.m.planes[0].length);
                throw std::runtime_error("Queue buffer failed");
            }
            // keep mapping for the duration of buffer use (do not munmap here)
        }
        close(vi_fd);
    }

private:
    int m_vfd; // V4L2 device file descriptor
};

void convertNV12toRGB(NvBufSurface* src, uint8_t* dst)
{
    // Make sure the current context is released
    if (eglDpy != EGL_NO_DISPLAY && eglCtx != EGL_NO_CONTEXT) {
        if (!eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx)) {
            std::cerr << "Failed to make EGL context current: "
                << eglGetError() << std::endl;
            return;
        }
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

void updateTexture(uint8_t* data)
{
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        video_width, video_height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, data);
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

        for (unsigned i = 0; fmt_ctx != NULL && i < fmt_ctx->nb_streams; i++) {
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

void videoDecodeRender(const char* filename)
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
        g_frame_queue.push(rgb);
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
                g_frame_queue.push(rgb);
            }
            av_frame_free(&frame);
        }
        av_packet_unref(&pkt);
    }
#endif
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <video file path>" << std::endl;
        return -1;
    }
#ifdef USE_JETSON_MULTIMEDIA_API
    try {
        init_drm();
    } catch (const std::exception& e) {
        std::cerr << "DRM setup fail: " << e.what() << std::endl;
        std::cerr << "Perhaps:\n"
            << "1. Monitor disconnect\n"
            << "2. No valid display mode\n"
            << "3. Permission denied(" << getenv("USER") << ")\n";
        cleanup_drm();
        exit(EXIT_FAILURE);
    }
    init_egl();

    // Start the video decoding thread
    std::thread mmediaThread(videoDecodeRender, argv[1]);

    while (true) {
        glClear(GL_COLOR_BUFFER_BIT);

        if (!g_frame_queue.empty()) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            uint8_t* frame = g_frame_queue.front();
            g_frame_queue.pop();
            updateTexture(frame);
            delete[] frame;
            // render to texture
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        // submit to screen
        render_frame();
    }

    mmediaThread.join();

    if (eglCtx != EGL_NO_CONTEXT) {
        eglDestroyContext(eglDpy, eglCtx);
    }
    if (eglDpy != EGL_NO_DISPLAY) {
        eglTerminate(eglDpy);
    }
#else
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init fail: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Video Render",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Create window fail: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Create renderer fail: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Start the video decoding thread
    std::thread ffmpegThread(videoDecodeRender, argv[1]);

    bool quit = false;
    SDL_Event event;
    SDL_Texture* texture = nullptr;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        if (!g_frame_queue.empty()) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            uint8_t* frame = g_frame_queue.front();

            if (!texture) {
                texture = SDL_CreateTexture(renderer,
                    SDL_PIXELFORMAT_RGB24,
                    SDL_TEXTUREACCESS_STREAMING,
                    video_width, video_height);
            }

            SDL_UpdateTexture(texture, nullptr, frame, video_width * 3);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);

            delete[] frame;
            g_frame_queue.pop();
        }
        SDL_Delay(10);
    }

    if (texture) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    ffmpegThread.join();
#endif
    return 0;
}
