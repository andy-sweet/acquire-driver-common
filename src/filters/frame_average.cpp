#include "device/props/components.h"
#include "device/props/filter.h"
#include "device/kit/filter.h"
#include "logger.h"
#include "platform.h"

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <algorithm>

using namespace std;

#define LOG(...) aq_logger(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) aq_logger(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE(__VA_ARGS__);                                                 \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false:\n\t%s", #e)

#define countof(e) (sizeof(e) / sizeof(*(e)))
#define containerof(ptr, T, V) ((T*)(((char*)(ptr)) - offsetof(T, V)))

namespace {

struct FrameAverage final : public Filter
{
    FrameAverage() noexcept;

    int set(const struct FilterProperties* settings) noexcept;
    void get(struct FilterProperties* settings) const noexcept;
    int accumulate(struct VideoFrame* accumulator, const struct VideoFrame* in) noexcept;

  private:
    uint32_t window_size_;
    uint32_t frame_count_;
};

enum DeviceState
frame_average_set(struct Filter*, const struct FilterProperties* settings);

enum DeviceState
frame_average_get(const struct Filter*, struct FilterProperties* settings);

enum DeviceState
frame_average_accumulate(const struct Filter*, struct VideoFrame* accumulator, const struct VideoFrame* in);

// TODO
//enum DeviceState
//frame_average_destroy(struct Filter*);

FrameAverage::FrameAverage() noexcept
  : Filter{
    .state = DeviceState_AwaitingConfiguration,
    .set = ::frame_average_set,
    .get = ::frame_average_get,
    .accumulate = ::frame_average_accumulate,
  }
  , window_size_(0)
  , frame_count_(0)
{
}

int
FrameAverage::set(const struct FilterProperties* settings) noexcept
{
    window_size_ = settings->window_size;
    return 1;
}

void
FrameAverage::get(struct FilterProperties* settings) const noexcept
{
    settings->window_size = window_size_;
}

int
FrameAverage::accumulate(struct VideoFrame* accumulator, const struct VideoFrame* in) noexcept
{
    ++frame_count_;

    size_t npx = accumulator->shape.strides.planes; // assumes planes is outer dim
    float* x = (float*)accumulator->data;
    switch (in->shape.type) {
        case SampleType_u8: {
            const uint8_t* y = in->data;
            for (size_t i = 0; i < npx; ++i)
                x[i] += y[i];
            break;
        }
        case SampleType_u10:
        case SampleType_u12:
        case SampleType_u14:
        case SampleType_u16: {
            const uint16_t* y = (uint16_t*)in->data;
            for (size_t i = 0; i < npx; ++i)
                x[i] += y[i];
            break;
        }
        case SampleType_i8: {
            const int8_t* y = (int8_t*)in->data;
            for (size_t i = 0; i < npx; ++i)
                x[i] += y[i];
            break;
        }
        case SampleType_i16: {
            const int16_t* y = (int16_t*)in->data;
            for (size_t i = 0; i < npx; ++i)
                x[i] += y[i];
            break;
        }
        default:
            LOGE("Unsupported pixel type");
    }

    if (frame_count_ == window_size_) {
        const float inverse_norm = frame_count_ ? 1.0f / (frame_count_) : 1.0f;
        for (size_t i = 0; i < npx; ++i)
            x[i] *= inverse_norm;
        frame_count_ = 0;
        return 1;
    }

    return 0;
}

enum DeviceState
frame_average_set(struct Filter* self_, const struct FilterProperties* settings)
{
    struct FrameAverage* self = (struct FrameAverage*)self_;
    if (self->set(settings))
        return DeviceState_Armed;
    else
        return DeviceState_AwaitingConfiguration;
}

enum DeviceState
frame_average_get(const struct Filter* self_, struct FilterProperties* settings)
{
    struct FrameAverage* self = (struct FrameAverage*)self_;
    self->get(settings);
    return DeviceState_Armed;
}

enum DeviceState
frame_average_accumulate(const struct Filter* self_, struct VideoFrame* accumulator, const struct VideoFrame* in)
{
    struct FrameAverage* self = (struct FrameAverage*)self_;
    if (self->accumulate(accumulator, in))
        return DeviceState_Armed;
    else
        return DeviceState_Running;
}

// TODO
//void
//frame_average_destroy(struct Filter* self_)
//{
//    struct FrameAverage* self = (struct FrameAverage*)self_;
//    delete self;
//}

} // end namespace ::{anonymous}

extern "C" struct Filter*
frame_average_init()
{
    return new FrameAverage();
}
