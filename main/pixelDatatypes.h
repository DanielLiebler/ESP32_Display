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
