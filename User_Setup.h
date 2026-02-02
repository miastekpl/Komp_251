// TFT_eSPI User Setup for ILI9341 with ESP32-S3
// Road Stripe Painter Controller - Trassar251

#ifndef USER_SETUP_H
#define USER_SETUP_H

// Driver
#define ILI9341_DRIVER

// ESP32-S3 SPI pins for ILI9341
// UWAGA: Piny 11, 12, 13 są zarezerwowane dla Flash na ESP32-S3!
// UWAGA: Piny 26-32 NIE ISTNIEJĄ na ESP32-S3!
// USB CDC wyłączone - GPIO 19, 20 dostępne
#define TFT_MISO 44
#define TFT_MOSI 43
#define TFT_SCLK 19
#define TFT_CS   10  // Chip select
#define TFT_DC   9   // Data Command
#define TFT_RST  8   // Reset

// Backlight control
#define TFT_BL   7
#define TFT_BACKLIGHT_ON HIGH

// Fonts
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2  // Font 2. Small 16 pixel font
#define LOAD_FONT4  // Font 4. Medium 26 pixel font
#define LOAD_FONT6  // Font 6. Large 48 pixel font
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font
#define LOAD_FONT8  // Font 8. Large 75 pixel font
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT

// SPI frequency
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

#endif // USER_SETUP_H
