#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "esp32_digital_led_lib.h"
#include "pixelDatatypes.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

digit_t digits[10];
digit_t digits_small[10];

#define LOG
#ifdef LOG
  #define logLine(x) if(Serial.availableForWrite()>0) Serial.print(x);
#else
  #define logLine(x) ;
#endif

#define NTP_OFFSET  1  * 60 * 60 // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "de.pool.ntp.org"
#define DST_PIN 4

// Replace with your network credentials
const char* ssid     = "Highway24";
const char* password = "grandwind673AC";
const int INTEGRATED_LED =  2;
const int LEDS_PIN =  16;
const int LEDCOUNT = 105;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);


WiFiServer server(80);

// Client variables 
char linebuf[80];
int charcount=0;
strand_t pStrand = {.rmtChannel = 0, .gpioNum = LEDS_PIN, .ledType = LED_WS2812B_V3, .brightLimit = 32, .numPixels = LEDCOUNT, .pixels = nullptr, ._stateVars = nullptr};
myTime_t currentTime = {.hours = 0, .minutes = 0, .seconds = 0};
int timeFormat = 1;
pixelColor_t timeColor = pixelFromRGB(10, 2, 1);
pixelColor_t dotColor = pixelFromRGB(5, 0, 0);

void setupLeds() {
  pinMode (LEDS_PIN, OUTPUT);
  digitalWrite (LEDS_PIN, LOW);

  if (digitalLeds_initStrands(&pStrand, 1)) {
    logLine("Init FAILURE: halting\n");
    while (true) {};
  }
  
  digitalLeds_resetPixels(&pStrand);

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
  digits_small[2] = {.pixelCount =  9, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 0,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[3] = {.pixelCount = 10, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[4] = {.pixelCount =  9, .pixels = {{.x = 0,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[5] = {.pixelCount = 11, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}, }};
  digits_small[6] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[7] = {.pixelCount =  8, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 2,.y = 1}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 2,.y = 4}}};
  digits_small[8] = {.pixelCount = 13, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 0,.y = 3}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
  digits_small[9] = {.pixelCount = 12, .pixels = {{.x = 0,.y = 0}, {.x = 1,.y = 0}, {.x = 2,.y = 0}, {.x = 0,.y = 1}, {.x = 2,.y = 1}, {.x = 0,.y = 2}, {.x = 1,.y = 2}, {.x = 2,.y = 2}, {.x = 2,.y = 3}, {.x = 0,.y = 4}, {.x = 1,.y = 4}, {.x = 2,.y = 4}}};
}

void setupWifiServer() {
  // We start by connecting to a WiFi network
  logLine("\n");
  logLine("\n");
  logLine("Connecting to ");
  logLine(ssid);
  logLine("\n");

  WiFi.begin(ssid, password);
  
  //digitalWrite(INTEGRATED_LED, HIGH);
  
  // attempt to connect to Wifi network:
  while(WiFi.status() != WL_CONNECTED) {
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(500);
    logLine(".");
  }
  logLine("\n");
  logLine("WiFi connected\n");
  logLine("IP address: \n");
  logLine(WiFi.localIP());
  logLine("\n");

  if (!MDNS.begin("esp32")) {
    logLine("Error setting up MDNS responder!\n");
    while(1) {
      delay(1000);
    }
  }
  logLine("mDNS responder started\n");
  
  server.begin();
  
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}

void buildWebpage(WiFiClient client) {
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");  // the connection will be closed after completion of the response
  client.println();
  client.println("<!DOCTYPE HTML><html><head>");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>");
  client.println("<h1>ESP32 - Web Server</h1>");
  client.println("<p>LED #1 <a href=\"on1\"><button>ON</button></a>&nbsp;<a href=\"off1\"><button>OFF</button></a></p>");
  client.println("<p>LED #2 <a href=\"on2\"><button>ON</button></a>&nbsp;<a href=\"off2\"><button>OFF</button></a></p>");
  client.println("<p>LED #2 <a href=\"set0\"><button>SET 0</button></a>&nbsp;<a href=\"reset\"><button>RESET</button></a></p>");
  client.println("</html>");
}

void checkParameters() {
  if (strstr(linebuf,"GET /on1") > 0) {
    logLine("LED 1 ON\n");
    digitalWrite(INTEGRATED_LED, HIGH);
  } else if (strstr(linebuf,"GET /off1") > 0) {
    logLine("LED 1 OFF\n");
    digitalWrite(INTEGRATED_LED, LOW);
  } else if (strstr(linebuf,"GET /on2") > 0) {
    logLine("LEDs GREEN\n");
    flushColor(pixelFromRGB(0, 255, 0));
  } else if (strstr(linebuf,"GET /off2") > 0) {
    logLine("LEDs RED\n");
    flushColor(pixelFromRGB(255, 0, 0));
  } else if (strstr(linebuf,"GET /reset") > 0) {
    logLine("reset LEDs\n");
    digitalLeds_resetPixels(&pStrand);
  }else if (strstr(linebuf,"GET /set") > 0) {
    int num = ((int)*(strstr(linebuf,"GET /set") + 8)+2)%10;
    logLine("set LED-DIGIT to ");
    logLine(num);
    printDigit(num, 0, pixelFromRGB(150, 150, 0));
    digitalLeds_updatePixels(&pStrand);
  }
}

void listenforClients() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    logLine("New client\n");
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        logLine(c);
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          buildWebpage(client);
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          checkParameters();
          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
      updateTime();
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    logLine("client disconnected\n");
  }
}

void flushColor(pixelColor_t color) {
  for (int i = 0; i < LEDCOUNT; i++) {
    pStrand.pixels[i] = color;
  }  
  digitalLeds_updatePixels(&pStrand);
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
  pStrand.pixels[pos] = color;
}

void printDigit(int digit, int xOffset, pixelColor_t color) {
  if (xOffset > 20) return;
  if (xOffset < -3) return;
  for (int i = 0; i < digits[digit].pixelCount; i++) {
    setPixel(xOffset + digits[digit].pixels[i].x, digits[digit].pixels[i].y, color);
  }
}

void printTime(int change) {
  if (timeFormat == 0) {
    digitalLeds_resetPixels(&pStrand);
    printDigit(currentTime.hours / 10, 0, timeColor);
    printDigit(currentTime.hours % 10, 5, timeColor);
    if (currentTime.seconds%2 == 0) {
      setPixel(10, 1, dotColor);
      setPixel(10, 3, dotColor);
    }
    printDigit(currentTime.minutes / 10, 12, timeColor);
    printDigit(currentTime.minutes % 10, 17, timeColor);
    digitalLeds_updatePixels(&pStrand);
  }else if (timeFormat == 1 && change > 2) {
    digitalLeds_resetPixels(&pStrand);
    printDigit(currentTime.hours / 10, 0, timeColor);
    printDigit(currentTime.hours % 10, 5, timeColor);
    setPixel(10, 1, dotColor);
    setPixel(10, 3, dotColor);
    printDigit(currentTime.minutes / 10, 12, timeColor);
    printDigit(currentTime.minutes % 10, 17, timeColor);
    digitalLeds_updatePixels(&pStrand);
  }
}

void updateTime(){
  timeClient.update();
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();
  int change = 0;
  if (hours != currentTime.hours){
    change = 4;
  } else if (minutes != currentTime.minutes){
    change = 3;
  } else if (seconds != currentTime.seconds){
    change = 2;
  }
  
  if(change > 0) {
    currentTime.hours = hours;
    currentTime.minutes = minutes;
    currentTime.seconds = seconds;
    String formattedTime = timeClient.getFormattedTime();
    logLine(currentTime.hours);
    logLine(":");
    logLine(currentTime.minutes);
    logLine(":");
    logLine(currentTime.seconds);
    logLine(" -- ");
    logLine(formattedTime);
    printTime(change);
  }
}

void setup() {
  timeClient.begin();
  // initialize the LEDs pins as an output:
  pinMode(INTEGRATED_LED, OUTPUT);

  #ifdef LOG
    //Initialize serial and wait for port to open:
    Serial.begin(115200);
    //while(!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    //}
  #endif

  //-------
  pinMode(DST_PIN, INPUT_PULLUP);
  if(digitalRead(DST_PIN) != HIGH) {
    logLine("Sommerzeit, GMT+2 \n");
    timeClient.setTimeOffset(2*NTP_OFFSET);
  }else{
    logLine("Winterzeit, GMT+1\n");
  }
  
  setupLeds();
  setupWifiServer();
  digitalWrite(INTEGRATED_LED, LOW);
}

void loop() {
  listenforClients();
  updateTime();  
}
