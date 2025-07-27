#define NULL 0
struct SwsContext { };
enum AVPixelFormat { A };
typedef int uint8_t;
typedef int AVCodecID;
typedef void SwsFilter;
typedef void  AVCodecContext;
typedef void AVPacket;
typedef void AVFrame;
typedef void AVFormatContext;
typedef void AVInputFormat;
typedef void AVDictionary;
typedef void AVCodec;
typedef void AVCodecParameters;
struct SwsContext* sws_getContext(
    int,
    int,
    enum AVPixelFormat,
    int,
    int,
    enum AVPixelFormat,
    int,
    SwsFilter*,
    SwsFilter*,
    const double*
)
{
    return NULL;
}
int sws_scale(
    struct SwsContext*,
    const uint8_t* const [],
    const int[],
    int,
    int,
    uint8_t* const [],
    const int[]
)
{
    return 0;
}
int avcodec_send_packet(AVCodecContext*, const AVPacket*) { return 0; }
AVFrame* av_frame_alloc(void) { return NULL; }
void av_frame_free(AVFrame**) { }
void av_packet_unref(AVPacket*) { }
int avcodec_receive_frame(AVCodecContext*, AVFrame*) { return 0; }
int av_read_frame(AVFormatContext*, AVPacket*) { return 0; }
int avformat_open_input(AVFormatContext**, const char*, AVInputFormat*, AVDictionary**) { return 0; }
int avformat_find_stream_info(AVFormatContext*, AVDictionary**) { return 0; }
const AVCodec* avcodec_find_decoder(AVCodecID) { return 0; }
AVCodecContext* avcodec_alloc_context3(const AVCodec*) { return 0; }
int avcodec_parameters_to_context(AVCodecContext*, const AVCodecParameters*) { return 0; }
int avcodec_open2(AVCodecContext*, const AVCodec*, AVDictionary**) { return 0; }
