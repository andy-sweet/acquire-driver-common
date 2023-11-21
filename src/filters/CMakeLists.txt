set(tgt filters)
add_library(${tgt} STATIC
        basic.filter.c
        basic.filter.h
        frame_average.cpp
)
target_enable_simd(${tgt})
target_link_libraries(${tgt} PRIVATE
        acquire-core-platform
        acquire-core-logger
        acquire-device-kit
)
