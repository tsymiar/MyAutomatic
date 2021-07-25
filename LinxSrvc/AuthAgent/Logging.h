#include <libgen.h>

#define Message(fmt, ...) fprintf(stdout, "\r[INFO](%s:%d)[%s]: " fmt "\n", basename((char*)__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__)
#define Error(fmt, ...) fprintf(stdout, "\r[ERROR](%s:%d)[%s]: " fmt "\n", basename((char*)__FILE__),__LINE__,__FUNCTION__,##__VA_ARGS__)
