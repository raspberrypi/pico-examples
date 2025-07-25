// Display Configuration - adjust as needed
#define PIN_MOSI 11   // MOSI (SPI data to display)
#define PIN_SCK  10   // CLK (SPI clock)
#define PIN_CS   9    // LCD_CS
#define PIN_DC   8    // LCD_DC
#define PIN_RST  15   // LCD_RST

// Display Dimensions - adjust as needed
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240

// Backlight control
#define PIN_BL   13   // LCD_BL (Backlight, if controllable)

// SPI Configuration
#define SPI_PORT spi1
#define SPI_BAUDRATE 10000000  // 10MHz

// Display Colors (RGB565)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
