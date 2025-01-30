#pragma once

#include <stdint.h>

#include <cstdarg>
#include <cstdio>
#include <string>

/** Check if a point (px,py) is contained in the given area. The margin is added around the area to allow a larger area,
 * e.g. for finger/touch interactions. */
bool isContained(int16_t x, int16_t y, int16_t w, int16_t h, int16_t px, int16_t py, int16_t margin = 0) {
    return x - margin <= px && px <= x + w + margin && y - margin <= py && py <= y + h + margin;
}

/** Format a string using printf-style formatting. */
std::string formatString(const char* format, ...) {
    va_list args;

    // Compute required buffer size
    va_start(args, format);
    int size = vsnprintf(nullptr, 0, format, args);  // Get required size (excluding null terminator)
    va_end(args);

    if (size < 0) {
        return "";
    }

    // Allocate buffer and format the string
    std::string result(size, '\0');  // Create a string with the required size
    va_start(args, format);
    vsnprintf(result.data(), size + 1, format, args);  // Format the string into the buffer
    va_end(args);

    return result;
}
