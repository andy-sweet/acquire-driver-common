#include "device/kit/driver.h"
#include "device/kit/filter.h"
#include "logger.h"
#include "basic.filter.h"

#include <string.h>
#include <stdlib.h>

#define containerof(P, T, F) ((T*)(((char*)(P)) - offsetof(T, F)))

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define CHECK(e)                                                               \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE("Expression was false:\n\t%s\n", #e);                         \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE(__VA_ARGS__);                                                 \
            goto Error;                                                        \
        }                                                                      \
    } while (0)

#ifdef WIN32
#define export __declspec(dllexport)
#else
#define export
#endif

//
//                  EXTERN
//
// Defined in implementations (e.g. frame_average.cpp).

struct Filter*
frame_average_init();

//
//                  GLOBALS
//

static struct
{
    struct Filter* (**constructors)();
} globals = { 0 };

//
//                  IMPL
//

void
basics_filter_shutdown(struct Driver* driver)
{
    free(globals.constructors);
    globals.constructors = 0;
}

struct Filter*
basics_make_filters(enum BasicDeviceKind kind)
{
    if (!globals.constructors) {
        const size_t nbytes =
          sizeof(globals.constructors[0]) * BasicDeviceKindCount;
        CHECK(globals.constructors = (struct Filter* (**)()) malloc(nbytes));
        struct Filter* (*impls[])() = {
            [BasicDevice_Filter_FrameAverage] = frame_average_init,
        };
        memcpy(
          globals.constructors, impls, nbytes); // cppcheck-suppress uninitvar
    }
    CHECK(kind < BasicDeviceKindCount);
    if (globals.constructors[kind])
        return globals.constructors[kind]();
    else {
        LOGE("No filter device found for %s",
             basic_device_kind_to_string(kind));
    }
Error:
    return NULL;
}
