#include <Arduino.h>

#include <ezTime.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"

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

//TZ stuff
Timezone timezone;

//Webserver object
AsyncWebServer server(80);

//Writing to Strands Async
TaskHandle_t updateDisplayTask;
portMUX_TYPE updateStrandMux = portMUX_INITIALIZER_UNLOCKED;



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
    strip1.SetPixelColor(x+21, color);
    break;
  case 2:
    strip2.SetPixelColor(x, color);
    break;
  case 3:
    strip2.SetPixelColor(x+21, color);
    break;
  case 4:
    strip3.SetPixelColor(x, color);
    break;
  default:
    break;
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
  //TODO currently not used, due to asyncronous updating of pixels
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
    //TODO Picture Mode
    break;
  case MODE_GIF:
    //TODO GIF Mode
    break;
  case MODE_FULL_COLOR:
  default:
    fillStrip(colorFull);
    showStrip();
    break;
  }
  portEXIT_CRITICAL(&updateStrandMux);
}

//Webserver functions
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String processor(const String& var) {
  Serial.println(var);
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
void setDisplayMode(int p){
  displayMode = p;
}
void setClockMode(int p){
  clockMode = p;
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
void setColorFull(pixelColor_t p){
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
    //set_Brightness((brightness+2)%100);
    portENTER_CRITICAL(&updateStrandMux);
    strip1.Show();
    strip2.Show();
    strip3.Show();
    portEXIT_CRITICAL(&updateStrandMux);
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
  if (false) {
    Serial.print(ESP.getFreeHeap()/1024);
    Serial.print("kB - ");
    Serial.print(ESP.getMinFreeHeap()/1024);
    Serial.print("kB - ");
    Serial.print("NumTasks: ");
    Serial.print(uxTaskGetNumberOfTasks());
    Serial.println(")");
  }
  //strip.Show();
}