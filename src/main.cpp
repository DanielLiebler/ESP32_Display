#include <Arduino.h>

#include <ezTime.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"

#include "pixelDatatypes.h"
//#include "Adafruit_NeoPixel.h"
#include <NeoPixelBus.h>

#define WIFI_SSID "Highway24"
#define WIFI_PASS "grandwind673AC"

#define MODE_CLOCK_1 1
#define MODE_CLOCK_2 2
#define MODE_CLOCK_3 3

typedef RgbColor pixelColor_t;

const int INTEGRATED_LED =  2;
const int LED_PIN1 =  4;
const int LED_PIN2 =  16;
const int LED_PIN3 =  17;
const int LEDCOUNTPERROW = 21;//105;

uint8_t displayMode_DEF = MODE_CLOCK_2;
uint8_t brightness_DEF = 15;

Timezone timezone;

//Adafruit_NeoPixel strip(LEDCOUNT, LEDS_PIN, NEO_GRB + NEO_KHZ800);
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0800KbpsMethod> strip1(LEDCOUNTPERROW*2, LED_PIN1);
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt1800KbpsMethod> strip2(LEDCOUNTPERROW*2, LED_PIN2);
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt2800KbpsMethod> strip3(LEDCOUNTPERROW, LED_PIN3);
pixelColor_t colorFromRGB(uint8_t r, uint8_t g, uint8_t b) {
  return RgbColor(r, g, b);
}
pixelColor_t color1_DEF = colorFromRGB(255, 70, 10);
pixelColor_t color2_DEF = colorFromRGB(255, 70, 10);
pixelColor_t color3_DEF = colorFromRGB(0, 100, 155);
pixelColor_t color4_DEF = colorFromRGB(0, 0, 255);
pixelColor_t colorFull_DEF = colorFromRGB(0, 0, 255);
byte displayMode = displayMode_DEF;
byte brightness = brightness_DEF;
pixelColor_t myColor1 = color1_DEF;
pixelColor_t myColor2 = color2_DEF;
pixelColor_t myColor3 = color3_DEF;
pixelColor_t myColor4 = color4_DEF;
bool colorFullEn = false;
pixelColor_t colorFull = color1_DEF;




digit_t digits[10];
digit_t digits_small[10];

artPage_t fuckYouList[] = {(artPage_t){0x21327A2Ful, 0xA271A0Aul, 0x1A0A210Eul, 0x327B61ul}, (artPage_t){0x4A442F51ul, 0x28444428ul, 0x4284444ul, 0x476F44ul}};
artBook_t fuckYou = {.numPages=2, .pages=fuckYouList};

AsyncWebServer server(80);

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t updateDisplayTask;
portMUX_TYPE updateMux = portMUX_INITIALIZER_UNLOCKED;

const char* PARAM_MESSAGE = "value";

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
void fillStrip(pixelColor_t color){
  strip1.ClearTo(color);
  strip2.ClearTo(color);
  strip3.ClearTo(color);
}
void clearStrip(){
  strip1.ClearTo(RgbColor(0, 0, 0));
  strip2.ClearTo(RgbColor(0, 0, 0));
  strip3.ClearTo(RgbColor(0, 0, 0));
}
void showStrip(){
  //strip.Show();
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

void displayPage(bool binaryData[], pixelColor_t color) {
  clearStrip();
  for (int i = 0; i < 105; i++) {
    if (binaryData[i]) setStripLed(i, color);
  }
  showStrip();
}

void toBool(artPage_t page, bool* binaryData) {
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 7; k++) {
        if (i%2 == 0) {
          binaryData[i*21 + j * 7 + k] = (page.data[3*i + j] >> k)%2;
        } else {
          binaryData[i*21 + 20 - j * 7 - k] = (page.data[3*i + j] >> k)%2;
        }
      }
    }
  }
}

artPage_t parsePage(bool data[][21]) {
  artPage_t binaryData;
  binaryData.data[15] = 0;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 3; j++) {
      binaryData.data[3*i + j] = 0;
      for (int k = 0; k < 7; k++){
        binaryData.data[3*i + j] += data[i][j*7+k] << k;
      }
    }
  }
  return binaryData;
}

void showArtBook(artBook_t artBook, int dly, pixelColor_t color) {
  bool binaryData[105];
  for (int i = 0; i < artBook.numPages; i++) {
    toBool(artBook.pages[i], binaryData);
    displayPage(binaryData, color);
    //TODO Pages..
  }
}

void flushColor(pixelColor_t color) {
  fillStrip(color);
  showStrip();
}

void updateColorFull() {
  if (colorFullEn) {
    flushColor(colorFull);
  }
}

void setColorFull(pixelColor_t data) {
  //preferences.begin("clock", false);
  //colorFull = data;
  //preferences.putUInt("colorFull", data.num);
  //preferences.end();
  updateColorFull();
}

void setPixel(int x, int y, pixelColor_t color) {
  if (x > 20) return;
  if (y > 4) return;
  if (x < 0) return;
  if (y < 0) return;
  int pos = 0;
  if (y%2 == 0) {
    pos = 21*y + x;  
  } else {
    pos = 21*(y+1) - (x+1);
  }
  setStripLed(pos, color);
}

void printDigit(int digit, int xOffset, pixelColor_t color, digit_t font[]) {
  if (xOffset > 20) return;
  if (xOffset < -3) return;
  for (int i = 0; i < font[digit].pixelCount; i++) {
    setPixel(xOffset + font[digit].pixels[i].x, font[digit].pixels[i].y, color);
  }
}

pixelColor_t calcBrightness(byte bright, pixelColor_t color) {
  return colorFromRGB(color.R*bright/100, color.G*bright/100, color.B*bright/100);
}

void printTime() {
  if (colorFullEn) return;
  if (displayMode == MODE_CLOCK_1) {
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
  }else if (displayMode == MODE_CLOCK_2) {
    clearStrip();
    printDigit(timezone.hour() / 10, 0, calcBrightness(brightness,myColor1), digits);
    printDigit(timezone.hour() % 10, 5, calcBrightness(brightness,myColor1), digits);
    setPixel(10, 1, calcBrightness(brightness,myColor2));
    setPixel(10, 3, calcBrightness(brightness,myColor2));
    printDigit(timezone.minute() / 10, 12, calcBrightness(brightness,myColor1), digits);
    printDigit(timezone.minute() % 10, 17, calcBrightness(brightness,myColor1), digits);
    showStrip();
  }else if (displayMode == MODE_CLOCK_3) {
    clearStrip();
    printDigit(timezone.hour() / 10, 0, calcBrightness(brightness,myColor1), digits_small);
    printDigit(timezone.hour() % 10, 4, calcBrightness(brightness,myColor1), digits_small);
    printDigit(timezone.minute() / 10, 7, calcBrightness(brightness,myColor2), digits_small);
    printDigit(timezone.minute() % 10, 11, calcBrightness(brightness,myColor2), digits_small);
    printDigit(timezone.second() / 10, 14, calcBrightness(brightness,myColor3), digits_small);
    printDigit(timezone.second() % 10, 18, calcBrightness(brightness,myColor3), digits_small);
    showStrip();
  }
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String processor(const String& var) {
  Serial.println(var);
  if(var == "PLACEHOLDER") {
    return String(random(1,20));
  } else if(var == "BRIGHTNESS_VALUE")
    return String(brightness); //F("20");
  if(var == "DISPLAYMODE_1" && displayMode == MODE_CLOCK_1)
    return F(" active");
  if(var == "DISPLAYMODE_2" && displayMode == MODE_CLOCK_2)
    return F(" active");
  if(var == "DISPLAYMODE_3" && displayMode == MODE_CLOCK_3)
    return F(" active");
  return String();

}
void setDisplayMode(int p){
  displayMode = p;
}
void set_Brightness(int p){
  brightness = p;
  printTime();
}
void setColor1(pixelColor_t p){
  myColor1 = p;
  printTime();
}
void setColor2(pixelColor_t p){
  myColor2 = p;
  printTime();
}
void setColor3(pixelColor_t p){
  myColor3 = p;
  printTime();
}
void setColor4(pixelColor_t p){
  myColor4 = p;
  printTime();
}
void checkParams(AsyncWebServerRequest *request) {
  if(request->hasParam("dst")) {
    //AsyncWebParameter* p = request->getParam("dst");
    //setDst(p->value().equalsIgnoreCase("true"));
    //printTime(8);
  }
  if(request->hasParam("displayMode")) {
    AsyncWebParameter* p = request->getParam("displayMode");
    setDisplayMode(p->value().toInt());
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
      showArtBook(fuckYou, 800, myColor4);
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
  if(request->hasParam("fullColor")) {
    AsyncWebParameter* p = request->getParam("fullColor");
    colorFullEn = (p->value().equalsIgnoreCase("true"));
    updateColorFull();
  }
}

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}

TickType_t xLastWakeTime;
const TickType_t xFrequency = 500 / portTICK_PERIOD_MS;
void updateDisplayTaskFunction(void* parameter) {

  xLastWakeTime = xTaskGetTickCount();
  int i = 1;
  long lastTime = millis();
  int myDivider = 100;
  for( ;; )
  {
    //set_Brightness((brightness+2)%100);
    portENTER_CRITICAL(&updateMux);
    strip1.Show();
    strip2.Show();
    strip3.Show();
    portEXIT_CRITICAL(&updateMux);
    if (i%myDivider == 0) {
      Serial.print("Average Time: ");
      Serial.print((millis()-lastTime)/myDivider);
      Serial.println("ms");
      lastTime = millis();
    }
    i++;
    //Serial.println("newFrame");
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
  vTaskDelete(NULL);
}

void setup() {

  pinMode(INTEGRATED_LED, OUTPUT);
  digitalWrite(INTEGRATED_LED, HIGH);
  timerSemaphore = xSemaphoreCreateBinary();
  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();

  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 1000000, true);

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
    //TODO Reboot
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
	//delay(1000);
  
  // Start an alarm
  timerAlarmEnable(timer);
  
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

int myCount = 0;
void loop() {
	//events();
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){

    portENTER_CRITICAL(&updateMux);
    printTime();
    portEXIT_CRITICAL(&updateMux);
    
    uint32_t isrCount = 0, isrTime = 0;
    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime = lastIsrAt;
    portEXIT_CRITICAL(&timerMux);
    int divider = 10;
    if (isrCount%divider == 0) {
      Serial.print(ESP.getFreeHeap()/1024);
      Serial.print("kB - ");
      Serial.print(ESP.getMinFreeHeap()/1024);
      Serial.print("kB - ");
      //Serial.print(myCount/divider);
      //Serial.println(" Loops/s");
      Serial.print("NumTasks: ");
      Serial.print(uxTaskGetNumberOfTasks());
      Serial.print(" - ");
      Serial.print(isrTime);
      Serial.print("ms(");
      Serial.print(isrCount);
      Serial.println(")");

      //Serial.println("----------------------------");
      myCount = 0;
    }
  }
  // If button is pressed
  if (false) {
    // If timer is still running
    if (timer) {
      // Stop and free timer
      timerEnd(timer);
      timer = NULL;
    }
  }
  myCount++;
  //strip.Show();
}