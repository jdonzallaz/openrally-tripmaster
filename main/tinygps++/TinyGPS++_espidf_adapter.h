/*
 * Adapter for the TinyGPS++ library to work with ESP-IDF.
 */

#if defined(ESP_PLATFORM)
// Define millis function
#include "esp_timer.h"
unsigned long millis()
{
    return esp_timer_get_time() / 1000;
}

// Add missing byte type
typedef uint8_t byte;

// Add missing math functions
#include <math.h>
#define TWO_PI M_TWOPI
#define degrees(x) ((x) * 180 / M_PI)
#define radians(x) ((x) * M_PI / 180)
#define sq(x) ((x) * (x))

#endif
