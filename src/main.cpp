#include <Arduino.h>

#include <ezTime.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "gif.h"
//#include "gif.cpp"

#include "pixelDatatypes.h"
#include <NeoPixelBus.h>

//WIFI Credentials
#define WIFI_SSID "Highway24"
#define WIFI_PASS "grandwind673AC"

//Convert RGB Values to Color
typedef RgbColor pixelColor_t;
pixelColor_t colorFromRGB(uint8_t r, uint8_t g, uint8_t b) {
  return RgbColor(r, g, b);
}

//display Modes
#define MODE_CLOCK 1
#define MODE_FULL_COLOR 2
#define MODE_PICTURE 3
#define MODE_GIF 4
#define MODE_TEST 5

//clock Modes
#define CLOCK_NORMAL 1
#define CLOCK_NORMAL_STEADY 2
#define CLOCK_FULL_SECONDS 3

//HW Pinout
const int INTEGRATED_LED =  2;
const int LED_PIN1 =  4;
const int LED_PIN2 =  16;
const int LED_PIN3 =  17;
const int LEDCOUNTPERROW = 21;

//default values
const uint8_t displayMode_DEF = MODE_CLOCK;
const uint8_t clockMode_DEF = CLOCK_NORMAL_STEADY;
const uint8_t brightness_DEF = 15;
const pixelColor_t color1_DEF = colorFromRGB(255, 70, 10);
const pixelColor_t color2_DEF = colorFromRGB(255, 70, 10);
const pixelColor_t color3_DEF = colorFromRGB(0, 100, 155);
const pixelColor_t color4_DEF = colorFromRGB(0, 0, 255);
const pixelColor_t colorFull_DEF = colorFromRGB(0, 0, 255);

//Rows of Pixels as Strands
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0800KbpsMethod> strip1(LEDCOUNTPERROW*2, LED_PIN1);
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt1800KbpsMethod> strip2(LEDCOUNTPERROW*2, LED_PIN2);
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt2800KbpsMethod> strip3(LEDCOUNTPERROW, LED_PIN3);

//Color variables
byte brightness = brightness_DEF;
pixelColor_t myColor1 = color1_DEF;
pixelColor_t myColor2 = color2_DEF;
pixelColor_t myColor3 = color3_DEF;
pixelColor_t myColor4 = color4_DEF;
pixelColor_t colorFull = colorFromRGB(255, 255, 255);

//display & clock modes
byte displayMode = displayMode_DEF;
byte clockMode = clockMode_DEF;

//hardcoded Font
// (gets initialized in Function)
digit_t digits[10];
digit_t digits_small[10];
cached_animation_t* currentGif;
int currentGifFrame = 0;
long nextFrameTime = 0;

//TZ stuff
Timezone timezone;

//Webserver object
AsyncWebServer server(80);

//Writing to Strands Async
TaskHandle_t updateDisplayTask;
portMUX_TYPE updateStrandMux = portMUX_INITIALIZER_UNLOCKED;

volatile SemaphoreHandle_t updateTimeSemaphore;

/*void heapcheck(String tag) {
  Serial.print("Heapcheck ");
  Serial.print(tag);
  Serial.print(": ");
  if (heap_caps_check_integrity_all(true)) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
    //heap_caps_dump_all();
    Serial.println("\n====================");
  }
}*/

void setStripLed(uint16_t i, pixelColor_t color){
  if (i >= LEDCOUNTPERROW*5) return;
  if (i < LEDCOUNTPERROW*2) {
    strip1.SetPixelColor(i, color);
  } else if (i < LEDCOUNTPERROW*4) {
    strip2.SetPixelColor(i%(LEDCOUNTPERROW*2), color);
  } else {
    strip3.SetPixelColor(i%(LEDCOUNTPERROW*2), color);
  }
}
void setPixel(int x, int y, pixelColor_t color) {
  if (x > 20) return;
  if (x < 0) return;
  switch (y) {
  case 0:
    strip1.SetPixelColor(x, color);
    break;
  case 1:
    strip1.SetPixelColor(41-x, color);
    break;
  case 2:
    strip2.SetPixelColor(x, color);
    break;
  case 3:
    strip2.SetPixelColor(41-x, color);
    break;
  case 4:
    strip3.SetPixelColor(x, color);
    break;
  default:
    break;
  }
}
inline void fillStrip(pixelColor_t color){
  strip1.ClearTo(color);
  strip2.ClearTo(color);
  strip3.ClearTo(color);
}
inline void clearStrip(){
  strip1.ClearTo(RgbColor(0, 0, 0));
  strip2.ClearTo(RgbColor(0, 0, 0));
  strip3.ClearTo(RgbColor(0, 0, 0));
}
inline void showStrip(){
  //TODO currently not used, due to asyncronous updating of pixels
  //strip.Show();
}

void pasteBGColor(rgb color, cached_animation_t* gif) {
  for (size_t x = 0; x < gif->width; x++){
    for (size_t y = 0; y < gif->height; y++){
      *(gif->colorData + x + (y*gif->width)) = color;
    }
  }
}
cached_animation_t* cacheGif(gif_image_t* image) {
  cached_animation_t* myGif = (cached_animation_t*) calloc(1, sizeof(cached_animation_t));
  myGif->height = 5;
  myGif->width = 21;
  myGif->colorData = (rgb*) malloc((myGif->width * myGif->height)*sizeof(rgb));
  bool gct = image->screen_descriptor.fields & 0x80;
  int bgColInd = image->screen_descriptor.background_color_index;
  rgb bgCol;
  if (gct) {
    if (bgColInd < image->global_color_table_size) {
      bgCol = *(image->global_color_table + bgColInd);
    } else {
      bgCol.r = 0;
      bgCol.g = 0;
      bgCol.b = 0;
    }
  } else {
    bgCol.r = 0;
    bgCol.g = 0;
    bgCol.b = 0;
  }
  pasteBGColor(bgCol, myGif);
  
  cached_animation_t* gifCursor = myGif;
  block_list_t* next = image->blocks;
  while(true) {
    if (next->isExtension) {
      switch(next->extensionHeader.extension_code) {
        case GRAPHIC_CONTROL:
          Serial.println("Graphic Control Extension");
          gifCursor->delay = next->extensionHeader.gce.delay_time;
          gifCursor->next = (cached_animation_t*) calloc(1, sizeof(cached_animation_t));
          gifCursor->next->height = 5;
          gifCursor->next->width = 21;
          gifCursor->next->colorData = (rgb*) malloc((myGif->width * myGif->height)*sizeof(rgb));
          pasteBGColor(bgCol, gifCursor->next);
          gifCursor = gifCursor->next;
        break;
        case APPLICATION_EXTENSION:
          Serial.println("Application Extension. skipping...");
        break;
        case PLAINTEXT_EXTENSION:
        break;
        case COMMENT_EXTENSION:
        default:
        break;
      }
    } else {
      Serial.println("Rect-Frame");
      image_descriptor_t desc = next->block.image_descriptor;
      for (size_t x = 0; x < desc.image_width; x++) {
        for (size_t y = 0; y < desc.image_height; y++) {
          int indice = x + y*desc.image_width;
          if (indice >= next->block.data_length) break;
          int ctIndice = *(next->block.decoded_data + indice);
          rgb colEntry;
          
          if (gct) {
            if (ctIndice >= image->global_color_table_size) return 0;
            colEntry = *(image->global_color_table + ctIndice);
          } else {
            if (ctIndice >= next->block.local_color_table_size) return 0;
            colEntry = *(next->block.local_color_table + ctIndice);
          }
          int globalX = desc.image_left_position + x;
          int globalY = desc.image_top_position + y;
          *(gifCursor->colorData + (globalX + (globalY*gifCursor->width))) = colEntry;
          //pixelColor_t col = colorFromRGB(colEntry.r, colEntry.g, colEntry.b);
          //setPixel(globalX, globalY, col);
        }
      }     
    }
    next = next->next;
    if (next == 0) return myGif;
  }
}

void setupLeds() {
  pinMode (LED_PIN1, OUTPUT);
  pinMode (LED_PIN2, OUTPUT);
  pinMode (LED_PIN3, OUTPUT);
  digitalWrite (LED_PIN1, LOW);
  digitalWrite (LED_PIN2, LOW);
  digitalWrite (LED_PIN3, LOW);
  strip1.Begin();
  strip2.Begin();
  strip3.Begin();
  clearStrip();
  showStrip();

  digits[0] = {.pixelCount = 14, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 3,.y = 1}, {.x = 0,.y = 2}, {.x = 3,.y = 2}, {.x = 0,.y = 3}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[1] = {.pixelCount = 6 , .pixels = {{.x = 3,.y = 0}, {.x = 2,.y = 1}, {.x = 3,.y = 1}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 3,.y = 4}}};
  digits[2] = {.pixelCount = 14, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 3,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 0,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[3] = {.pixelCount = 13, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 3,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[4] = {.pixelCount = 9 , .pixels = {{.x = 0,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 1,.y = 3}, {.x = 2,.y = 3}, {.x = 3,.y = 3}, {.x = 2,.y = 4}}};
  digits[5] = {.pixelCount = 14, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[6] = {.pixelCount = 15, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 0,.y = 3}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[7] = {.pixelCount = 9 , .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 3,.y = 1}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 3,.y = 4}}};
  digits[8] = {.pixelCount = 16, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 3,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 0,.y = 3}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};
  digits[9] = {.pixelCount = 15, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 3,.y = 0}, {.x = 0,.y = 1}, {.x = 3,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 3,.y = 2}, {.x = 3,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, {.x = 3,.y = 4}}};

  digits_small[0] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[1] = {.pixelCount =  5, .pixels = {{.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[2] = {.pixelCount = 11, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[3] = {.pixelCount = 10, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[4] = {.pixelCount =  9, .pixels = {{.x = 0,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[5] = {.pixelCount = 11, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, }};
  digits_small[6] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[7] = {.pixelCount =  8, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[8] = {.pixelCount = 13, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[9] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
}

void printDigit(int digit, int xOffset, pixelColor_t color, digit_t font[]) {
  if (xOffset > 20) return;
  if (xOffset < -3) return;
  for (int i = 0; i < font[digit].pixelCount; i++) {
    setPixel(xOffset + font[digit].pixels[i].x, font[digit].pixels[i].y, color);
  }
}

//uses percentage brightness
pixelColor_t calcBrightness(byte brightPercent, pixelColor_t color) {
  return colorFromRGB(color.R*brightPercent/100, color.G*brightPercent/100, color.B*brightPercent/100);
}

void printTime() {
  portENTER_CRITICAL(&updateStrandMux);
  switch (displayMode) {
  case MODE_CLOCK:
    switch (clockMode) {
    case CLOCK_FULL_SECONDS:
      //Serial.println("Clock_Sec");
      clearStrip();
      printDigit(timezone.hour() / 10, 0, calcBrightness(brightness,myColor1), digits_small);
      printDigit(timezone.hour() % 10, 4, calcBrightness(brightness,myColor1), digits_small);
      printDigit(timezone.minute() / 10, 7, calcBrightness(brightness,myColor2), digits_small);
      printDigit(timezone.minute() % 10, 11, calcBrightness(brightness,myColor2), digits_small);
      printDigit(timezone.second() / 10, 14, calcBrightness(brightness,myColor3), digits_small);
      printDigit(timezone.second() % 10, 18, calcBrightness(brightness,myColor3), digits_small);
      showStrip();
      break;
    case CLOCK_NORMAL:
      //Serial.println("Clock_Normal");
      clearStrip();
      printDigit(timezone.hour() / 10, 0, calcBrightness(brightness,myColor1), digits);
      printDigit(timezone.hour() % 10, 5, calcBrightness(brightness,myColor1), digits);
      if (timezone.second()%2 == 0) {
        setPixel(10, 1, calcBrightness(brightness,myColor2));
        setPixel(10, 3, calcBrightness(brightness,myColor2));
      }
      printDigit(timezone.minute() / 10, 12, calcBrightness(brightness,myColor1), digits);
      printDigit(timezone.minute() % 10, 17, calcBrightness(brightness,myColor1), digits);
      showStrip();
      break;
    case CLOCK_NORMAL_STEADY:
    default:
      //Serial.println("Clock_Steady");
      clearStrip();
      printDigit(timezone.hour() / 10, 0, calcBrightness(brightness,myColor1), digits);
      printDigit(timezone.hour() % 10, 5, calcBrightness(brightness,myColor1), digits);
      setPixel(10, 1, calcBrightness(brightness,myColor2));
      setPixel(10, 3, calcBrightness(brightness,myColor2));
      printDigit(timezone.minute() / 10, 12, calcBrightness(brightness,myColor1), digits);
      printDigit(timezone.minute() % 10, 17, calcBrightness(brightness,myColor1), digits);
      showStrip();
      break;
    }
    break;
  case MODE_PICTURE:
      Serial.println("Picture");
    //TODO Picture Mode
    break;
  case MODE_GIF:
    //TODO GIF Mode
      if (currentGif != 0 && millis() >= nextFrameTime) {
        cached_animation_t* frame = currentGif;
        for (size_t i = 0; i < currentGifFrame; i++) {
          frame = frame->next;
          if (frame == 0) {
            frame = currentGif;
            currentGifFrame = 0;
            break;
          }
        }
        Serial.print("Frame ");
        Serial.println(currentGifFrame);
        clearStrip();
        for (size_t x = 0; x < frame->width; x++) {
          for (size_t y = 0; y < frame->height; y++) {
            rgb col = *(frame->colorData + (x + y*frame->width));
            setPixel(x, y, colorFromRGB(col.r, col.g, col.b));
          }
        }
        currentGifFrame++;
        nextFrameTime = millis() + frame->delay*10;
      }
    break;
  case MODE_TEST:
    Serial.print(timezone.second());
    Serial.print(" ");
    clearStrip();
    if (false) {
      setStripLed(timezone.second(), colorFromRGB(255, 255, 255));
      setStripLed(timezone.second()+42, colorFromRGB(255, 255, 255));
      setStripLed(timezone.second()+84, colorFromRGB(255, 255, 255));
    } else {
      setPixel((timezone.second())%21, 0, colorFromRGB(250, 50, 0));
      setPixel((timezone.second()-1)%21, 1, colorFromRGB(200, 100, 0));
      setPixel((timezone.second()-2)%21, 2, colorFromRGB(150, 150, 0));
      setPixel((timezone.second()-3)%21, 3, colorFromRGB(100, 200, 0));
      setPixel((timezone.second()-4)%21, 4, colorFromRGB(50, 250, 0));
    }
    break;
  case MODE_FULL_COLOR:
  default:
    Serial.println("FullColor");
    fillStrip(colorFull);
    showStrip();
    break;
  }
  portEXIT_CRITICAL(&updateStrandMux);
}

bool processGif(String fname, gif_image_t* image) {
  Serial.println("==============================");
  Serial.println("Current Filesystem:");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print(" - ");
      Serial.println(file.available());
      file.close();
      file = root.openNextFile();
  }
  root.close();
  Serial.println("==============================\n");


  File gif_file = SPIFFS.open( fname, FILE_READ );
  Serial.print("Trying to open ");
  Serial.println(fname);

  Serial.print("File readable?: ");
  Serial.println(gif_file.available());

  if ( !gif_file || !gif_file.available()) {
    Serial.print("Unable to open file ");
    Serial.println( fname );
    perror( ": " );
    return false;
  }

  bool state = process_gif_stream( gif_file, (gif_image_t*)image);

  //close( gif_file );
  gif_file.close();
  return state;
}

//Webserver functions
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String processor(const String& var) {
  //Serial.println(var);
  if(var == "PLACEHOLDER") {
    return String(random(1,20));
  } else if(var == "BRIGHTNESS_VALUE")
    return String(brightness);
  if(var == "CLOCK_NORMAL" && displayMode == CLOCK_NORMAL)
    return F(" active");
  if(var == "CLOCK_NORMAL_STEADY" && displayMode == CLOCK_NORMAL_STEADY)
    return F(" active");
  if(var == "CLOCK_FULL_SECONDS" && displayMode == CLOCK_FULL_SECONDS)
    return F(" active");
  return String();
}
inline void setDisplayMode(int p){
  displayMode = p;
}
inline void setClockMode(int p){
  clockMode = p;
}
inline void set_Brightness(int p){
  brightness = p;
  printTime();
}
inline void setColor1(pixelColor_t p){
  myColor1 = p;
  printTime();
}
inline void setColor2(pixelColor_t p){
  myColor2 = p;
  printTime();
}
inline void setColor3(pixelColor_t p){
  myColor3 = p;
  printTime();
}
inline void setColor4(pixelColor_t p){
  myColor4 = p;
  printTime();
}
inline void setColorFull(pixelColor_t p){
  colorFull = p;
  printTime();
}
void checkParams(AsyncWebServerRequest *request) {
  if(request->hasParam("displayMode")) {
    AsyncWebParameter* p = request->getParam("displayMode");
    setDisplayMode(p->value().toInt());
    printTime();
  }
  if(request->hasParam("clockMode")) {
    AsyncWebParameter* p = request->getParam("clockMode");
    setClockMode(p->value().toInt());
    printTime();
  }
  if(request->hasParam("brightness")) {
    AsyncWebParameter* p = request->getParam("brightness");
    set_Brightness(p->value().toInt());
    printTime();
  }
  if(request->hasParam("color1R") && request->hasParam("color1G") && request->hasParam("color1B")) {
    AsyncWebParameter* r = request->getParam("color1R");
    AsyncWebParameter* g = request->getParam("color1G");
    AsyncWebParameter* b = request->getParam("color1B");
    setColor1(colorFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime();
  }
  if(request->hasParam("color2R") && request->hasParam("color2G") && request->hasParam("color2B")) {
    AsyncWebParameter* r = request->getParam("color2R");
    AsyncWebParameter* g = request->getParam("color2G");
    AsyncWebParameter* b = request->getParam("color2B");
    setColor2(colorFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime();
  }
  if(request->hasParam("color3R") && request->hasParam("color3G") && request->hasParam("color3B")) {
    AsyncWebParameter* r = request->getParam("color3R");
    AsyncWebParameter* g = request->getParam("color3G");
    AsyncWebParameter* b = request->getParam("color3B");
    setColor3(colorFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime();
  }
  if(request->hasParam("color4R") && request->hasParam("color4G") && request->hasParam("color4B")) {
    AsyncWebParameter* r = request->getParam("color4R");
    AsyncWebParameter* g = request->getParam("color4G");
    AsyncWebParameter* b = request->getParam("color4B");
    setColor4(colorFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime();
  }
  if(request->hasParam("intergratedLed")) {
    AsyncWebParameter* p = request->getParam("intergratedLed");
    if(p->value().equalsIgnoreCase("true")) {
      digitalWrite(INTEGRATED_LED, HIGH);
      printTime();
    } else {
      digitalWrite(INTEGRATED_LED, LOW);  
    }
  }
  if(request->hasParam("fullColorR") && request->hasParam("fullColorG") && request->hasParam("fullColorB")) {
    AsyncWebParameter* r = request->getParam("fullColorR");
    AsyncWebParameter* g = request->getParam("fullColorG");
    AsyncWebParameter* b = request->getParam("fullColorB");
    setColorFull(colorFromRGB(r->value().toInt(), g->value().toInt(), b->value().toInt()));
    printTime();
  }
  if(request->hasParam("gif")) {
    AsyncWebParameter* fname = request->getParam("gif");
    gif_image_t* image = (gif_image_t*) malloc(sizeof(gif_image_t));
    Serial.println("Processing...");
    //heapcheck("1");
    try {
      if (processGif(fname->value(), image)) {
        Serial.println("Showing...");
        Serial.println(image->screen_descriptor.width);
        Serial.println(image->screen_descriptor.height);
        currentGif = cacheGif(image);
        currentGifFrame = 0;
        nextFrameTime = 0;
      } else {
        Serial.println("Failed '" + fname->value() + "'");
      }
    } catch(const std::exception& e) {
      Serial.println(e.what());
    }
    //heapcheck("end");
  }
}

//Timer for Async printing to strands
TickType_t xLastWakeTime;
const TickType_t xFrequency = 500 / portTICK_PERIOD_MS;
void updateDisplayTaskFunction(void* parameter) {

  xLastWakeTime = xTaskGetTickCount();
  int i = 1;
  long lastTime = millis();
  int myDivider = 100;
  for( ;; )
  {
    printTime();
    //set_Brightness((brightness+2)%100);
    portENTER_CRITICAL(&updateStrandMux);
    strip1.Show();
    strip2.Show();
    strip3.Show();
    portEXIT_CRITICAL(&updateStrandMux);
    //xSemaphoreGiveFromISR(updateTimeSemaphore, NULL);

    if (i%myDivider == 0) {
      Serial.print("Average Time: ");
      Serial.print((millis()-lastTime)/myDivider);
      Serial.println("ms");
      lastTime = millis();
    }
    i++;
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
  vTaskDelete(NULL);
}

void setup() {

  pinMode(INTEGRATED_LED, OUTPUT);
  digitalWrite(INTEGRATED_LED, HIGH);
  updateTimeSemaphore = xSemaphoreCreateBinary();

	Serial.begin(115200);
  
  if(!SPIFFS.begin()){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
  }

  setupLeds();

  Serial.println("Connecting...");
  WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASS);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    ESP.restart();
    return;
  }
  digitalWrite(INTEGRATED_LED, LOW);
  Serial.println("READY\n======\n======");

	// Uncomment the line below to see what it does behind the scenes
	// setDebug(INFO);
	
	waitForSync();


	// Provide official timezone names
	// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
	timezone.setLocation(F("de"));
	Serial.print(F("Germany:         "));
	Serial.println(timezone.dateTime());

	// wait for time to cool down
	delay(1000);
  
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    checkParams(request);
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });

  server.onNotFound(notFound);

  server.begin();
  xTaskCreatePinnedToCore(updateDisplayTaskFunction, "updateDisplayTask", 1000, NULL, 1, &updateDisplayTask, 1);
}

void loop() {
	//events();
  
  if (xSemaphoreTake(updateTimeSemaphore, 0) == pdTRUE){
    //printTime();
    /*portENTER_CRITICAL(&updateStrandMux);
    strip.Show();
    portEXIT_CRITICAL(&updateStrandMux);*/
  }
}