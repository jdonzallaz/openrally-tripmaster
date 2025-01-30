#include "freertos/FreeRTOS.h"

// ===== Main =====
#define MAIN_LOOP_DELAY_MS 20  // TODO: remove - not used

// ===== GPS =====
#define GPS_LOOP_DELAY_MS 500
#define GPS_PROCESS_CORE 1
#define GPS_PROCESS_PRIORITY 3
#define GPS_PROCESS_STACK_DEPTH 1024 * 8
// Define the UART port and pins
#define GPS_UART_PORT_NUM UART_NUM_2
#define GPS_UART_RX_PIN 13
#define GPS_UART_TX_PIN 14
#define GPS_UART_BUFFER_SIZE 1024
#define GPS_UART_BAUD_RATE 9600  // Standard for ATGM336H

// ===== Buttons =====
#define BUTTONS_LOOP_DELAY_MS 1000
#define BUTTONS_PROCESS_CORE 1
#define BUTTONS_PROCESS_PRIORITY 3
#define BUTTONS_PROCESS_STACK_DEPTH 1024 * 8

#define BUTTON_INCREMENT_DISTANCE_LONG_PRESS_TIME_MS 800
#define BUTTON_INCREMENT_DISTANCE_SHORT_PRESS_TIME_MS CONFIG_BUTTON_SHORT_PRESS_TIME_MS  // Keep default value
#define BUTTON_INCREMENT_DISTANCE_GPIO 0
#define BUTTON_INCREMENT_DISTANCE_M 10.0f

// ===== Display =====
#define DISPLAY_LOOP_DELAY_MS 41
#define DISPLAY_PROCESS_CORE 1
#define DISPLAY_PROCESS_PRIORITY 5
#define DISPLAY_PROCESS_STACK_DEPTH 1024 * 8
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

// ===== Storage =====
#define STORAGE_NAMESPACE "storage"

// ===== State =====
#define STATE_DISTANCE_EPSILON 0.5f  // Comparison with epsilon good enough for our use case
#define STATE_SEMAPHORE_TIMEOUT pdMS_TO_TICKS(50)
#define STATE_MAX_VALID_SPEED 150.0f
#define STATE_MIN_SAVE_DELAY_US 10'000'000
#define STATE_DEFAULT_WHEEL_SIZE 2000
#define STATE_DEFAULT_BRIGHTNESS 100

// ===== Touch =====
#define SWIPE_VERTICAL_THRESHOLD 70
#define SWIPE_HORIZONTAL_THRESHOLD 80
#define SWIPE_MAX_OTHER_DIRECTION_THRESHOLD 40

// ===== UI =====
#define COLOR_PRIMARY 0x1bba  // #1975D6
#define COLOR_LIGHTGREY 0xbdf7  // #BDBEBD
#define COLOR_GREY 0x9cf3       // #9C9C9C
#define COLOR_ERROR 0xc800      // #CE0000

#define UI_BUTTON_TEXT_COLOR BLACK
#define UI_BUTTON_BACKGROUND_COLOR WHITE
#define UI_BUTTON_BORDER_COLOR COLOR_LIGHTGREY

#define UI_SLIDER_THUMB_COLOR COLOR_PRIMARY
#define UI_SLIDER_BACKGROUND_COLOR COLOR_LIGHTGREY
#define UI_SLIDER_BORDER_COLOR WHITE

#define UI_ALERT_WIDTH 280
#define UI_ALERT_TOP_MARGIN 50
#define UI_ALERT_HEIGHT_LINE_HEIGHT 40
