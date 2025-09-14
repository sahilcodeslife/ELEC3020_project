#define ST7789_DRIVER

#define TFT_WIDTH  170
#define TFT_HEIGHT 320

#define CGRAM_OFFSET  // For shifted display

// Pins
#define TFT_MOSI 35
#define TFT_SCLK 36
#define TFT_DC   37
#define TFT_RST  38
#define TFT_CS   34

#define TFT_BL   45   // Backlight pin
#define TFT_BACKLIGHT_ON HIGH

// Touch not present
#define TOUCH_CS -1

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY  80000000
