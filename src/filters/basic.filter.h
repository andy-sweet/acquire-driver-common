#ifndef ACQUIRE_DRIVER_BASICS_BASIC_FILTER_H
#define ACQUIRE_DRIVER_BASICS_BASIC_FILTER_H

#include "../identifiers.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct Filter* basics_make_filters(enum BasicDeviceKind kind);
    void basics_filter_shutdown(struct Driver* driver);

#ifdef __cplusplus
};
#endif

#endif // ACQUIRE_DRIVER_BASICS_BASIC_FILTER_H
