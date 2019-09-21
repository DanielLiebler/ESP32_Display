typedef struct {
  int x;
  int y;
} pixel_t;

typedef struct {
  int pixelCount;
  pixel_t pixels[20];
} digit_t;

typedef struct {
  int hours;
  int minutes;
  int seconds;
} myTime_t;

typedef union {
  uint32_t num[4];
  uint8_t data[16];
} artPage_t;

typedef struct {
  int numPages;
  artPage_t * pages;
} artBook_t;
