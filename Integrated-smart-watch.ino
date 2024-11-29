//libraries
#include <Wire.h>
#include <Wireling.h>
#include <TinyScreen.h>
#include <GraphicsBuffer.h>
#include <MAX30101.h>
#include <TimeLib.h>
#include <SPI.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include <RTCZero.h>
#include <BMA250.h>
#include "Chinese.h"
#include <ArduinoJson.h>
#include "pitches.h" // Include for note definitions
#include <STBLE.h>

// define SERIAL, Make Serial Monitor compatible for all TinyCircuits processors
#if defined (ARDUINO_ARCH_AVR)
#define SerialMonitorInterface Serial
#elif defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#endif

// define screen variables
TinyScreen display = TinyScreen(TinyScreenDefault); 
GraphicsBuffer displayBuffer = GraphicsBuffer(96, 64, colorDepth16BPP);
uint8_t menuTextY[8] = {1 * 8 - 1, 2 * 8 - 1, 3 * 8 - 1, 4 * 8 - 1, 5 * 8 - 1, 6 * 8 - 1, 7 * 8 - 3, 8 * 8 - 3};
const uint8_t upButton     = TSButtonUpperRight;
const uint8_t downButton   = TSButtonLowerRight;
const uint8_t viewButton   = TSButtonLowerRight;
const uint8_t clearButton  = TSButtonLowerRight;
const uint8_t selectButton = TSButtonUpperLeft;
const uint8_t menuButton   = TSButtonLowerLeft;
unsigned long sleepTimer = 0;
int sleepTimeout = 0;
volatile uint32_t counter = 0;
uint32_t lastPrint = 0;
uint8_t rewriteTime = true;
uint8_t lastAMPMDisplayed = 0;
uint8_t lastHourDisplayed = -1;
uint8_t lastMinuteDisplayed = -1;
uint8_t lastSecondDisplayed = -1;
uint8_t lastDisplayedDay = -1;
uint8_t displayOn = 0;
uint8_t buttonReleased = 1;
uint8_t rewriteMenu = false;
unsigned long mainDisplayUpdateInterval = 20;
unsigned long lastMainDisplayUpdate = 0;
int brightness = 8;
uint8_t lastSetBrightness = 7;
const uint8_t displayStateHome = 0x01;
const uint8_t displayEmergency = 0x03;
// const uint8_t displayStateCalibration = 0x04;
const uint8_t displayCalendar = 0x05;
const uint8_t displayFall = 0x06;
const uint8_t displayHeartRate = 0x07;
const uint8_t displayChineseHome = 0x08;
const uint8_t displayMalayHome = 0x09;
uint8_t prevDisplayState;
uint8_t currentDisplayState = displayStateHome;
void drawCharacter(uint8_t x, uint8_t y, const uint8_t* bitmap, uint8_t width, uint8_t height);
int lang = 0;

//define pulse variables
MAX30101 pulseSensor = MAX30101();
int pulseSensorPort = 2;
bool slowUpdateRate = false;
const int graphWidth = 96;
const int graphHeight = 32;
const int graphYPos = 40; 
uint8_t menuTextX[8] = {0, 50, 0, 0, 0, 0, 0, 0};
uint8_t menuY[8] = {8, 8, 24, 0, 0, 0, 0, 0};
float samples[graphWidth];
#define BLUE 0xF800   
#define RED 0x001F  
#define BLACK 0x0000 
#define WHITE 0xFFFF

//define pulse function prototypes
void drawGraph(GraphicsBuffer * sbuff, int xDispPos, int yDispPos, int gWidth, int gHeight, float samples[]);
void bpmloop();
void bpmsetup();

// define wifi variables
const char* ssid = "独清";        
const char* pass = "Qiaoyijing1608";
int status = WL_IDLE_STATUS;

// define Real-Time-Clock variables
RTCZero rtc;
const int GMT = 8;

// define wifi function prototypes
void setupWiFi(const char* ssid, const char* password);
void setRTCfromWifi();

// define accelerometer variables
BMA250 accel_sensor;
int x, y, z;
double temp;

// define accelerometer function prototypes
void accel_readings();
void showSerial();
void fallDetection();

// Define buzzer pin
const int buzzerPin = A1;

// define calendar variables
// Server details
const char* serverHost = "192.168.43.35"; // Replace with node.js server (ipconfig)
const int serverPort = 3000; // Node.js server port
WiFiClient client;
// Medication reminders
const int MAX_REMINDERS = 10; // Maximum number of reminders
struct MedicationReminder {
    unsigned long triggerTime; // Time in milliseconds when the alarm should go off
    String timeStr;            // Time as a string for display purposes
    String medication;         // Medication name
};
MedicationReminder reminders[MAX_REMINDERS];
int numReminders = 0;
// Melody for the alarm
int melody[] = { NOTE_C5, NOTE_G4, NOTE_A4, NOTE_G4, NOTE_C5 };
int noteDurations[] = { 8, 8, 4, 4, 8 };
// Variables for timing
bool isAlarmActive = false;
int currentReminderIndex = 0;
unsigned long melodyMillis = 0;
int currentNoteIndex = 0;
unsigned long melodyPauseDuration = 2000; 
time_t previousTime = 0; 
const int interval = 5000; 

// define calendar function prototypes
void calsetup();
void calloop();
void fetchCalendarEvents();
void parseAndStoreEvents(String jsonResponse);
String formatDateTime(String dateTime);
void startAlarm();
void playAlarmMelody();
void stopAlarm();
bool checkForStop();

// define bluetooth variables
#ifndef BLE_DEBUG
#define BLE_DEBUG true
#endif
uint8_t ble_rx_buffer[21];
uint8_t ble_rx_buffer_len = 0;
uint8_t ble_connection_state = false;
#define PIPE_UART_OVER_BTLE_UART_TX_TX 0

// menu variables
#define menu_debug_print true // Debug messages will print to the Serial Monitor when this is 'true'

// Change the menu font colors for fun! Pre-set color options are TS_16b_:
// Black, Gray, DarkGray, White, Blue, DarkBlue, Red, DarkRed, Green, DarkGreen, Brown, DarkBrown, Yellow
// You can make more colors in the TinyScreen library TinyScreen.h file, there are over 65000+ color combos!
uint16_t defaultFontColor = TS_16b_White;
uint16_t defaultFontBG = TS_16b_Black;
uint16_t inactiveFontColor = TS_16b_Gray;
uint16_t inactiveFontBG = TS_16b_Black;
uint8_t menuHistory[5];
uint8_t menuHistoryIndex = 0;
uint8_t menuSelectionLineHistory[5];
int currentMenu = 0;
int currentMenuLine = 0;
int lastMenuLine = -1;
int currentSelectionLine = 0;
int lastSelectionLine = -1;
void (*menuHandler)(uint8_t) = NULL;
// Use this struct outline to create menus
typedef struct
{
  const uint8_t amtLines;
  const char* const * strings;
  void (*selectionHandler)(uint8_t);
} menu_info;

// emergency menu
void emergencyMenu(uint8_t selection);
static const char PROGMEM emergencyStrings0[] = "Yes";
static const char PROGMEM emergencyStrings1[] = "No";

static const char* const PROGMEM emergencyStrings[] = {
  emergencyStrings0,
  emergencyStrings1,
};

const menu_info emergencyMenuInfo = {
  2,
  emergencyStrings,
  emergencyMenu,
};

const menu_info menuList[] = {
  emergencyMenuInfo, 
  };
#define emergencyMenuIndex 0

void leftArrow(int x, int y);
void rightArrow(int x, int y);

// main setup
void setup() {
  // initialise hardware
  Wire.begin();
  Wireling.begin();
  Serial.begin(200000);
  delay(100);

  // initialise graphicsbuffer
  if (displayBuffer.begin()) {
    while (1) {
      Serial.println("Display buffer memory allocation error!");
      delay(1000);
    }
  }

  // initialise display
  display.begin();
  display.setFlip(true);
  display.setBitDepth(16);
  display.initDMA();
  displayBuffer.setFont(thinPixel7_10ptFontInfo);
  displayBuffer.clear();

  // initialise slow update for pulse
  #if defined (ARDUINO_ARCH_AVR)
  slowUpdateRate = true;
  #endif

  // Connect to WiFi
  setupWiFi(ssid, pass);

  // Set time from RTC
  setRTCfromWifi();
  setTime(rtc.getHours(),rtc.getMinutes(),rtc.getSeconds(),rtc.getDay(), rtc.getMonth(), rtc.getYear());
  previousTime = now();

  // Begin BMA250 acccelerometer sensor
  accel_sensor.begin(BMA250_range_2g, BMA250_update_time_64ms);

  // Fetch reminders from Google Calendar
  fetchCalendarEvents();

  // Initialise bluetooth
  // BLEsetup();
}

void loop() {
  // bpmloop();

  displayOn = 1;
  if (displayOn && (millisOffset() > mainDisplayUpdateInterval + lastMainDisplayUpdate)) {
    lastMainDisplayUpdate = millis();
    display.endTransfer();
    display.goTo(0, 0);
    display.startData();
    display.writeBufferDMA(displayBuffer.getBuffer(), displayBuffer.getBufferSize());
  }

  if (currentDisplayState == displayHeartRate) {
    bpmloop();
  } else if (currentDisplayState == displayFall) {
    displayBuffer.clear();
    fallDetection();
    delay(10000);
    displayBuffer.clear();
    currentDisplayState = prevDisplayState;
  } 
  else if (currentDisplayState == displayStateHome || currentDisplayState == displayChineseHome || currentDisplayState == displayMalayHome) {
    if ( display.getButtons(TSButtonLowerRight) || display.getButtons(TSButtonUpperRight) || display.getButtons(TSButtonLowerLeft) || display.getButtons(TSButtonUpperLeft)){
      displayBuffer.clear();  // if a button is pressed, clear the screen
    }

    updateMainDisplay(); 
    liveDisplay(); // This is the main home screen display
    display.writeBufferDMA(displayBuffer.getBuffer(), displayBuffer.getBufferSize());
    delay(500); // Making this smaller will make the screen more scattered, making it longer will mean you need to hold in buttons longer
  } 
  else {
    drawMenu();
  } 

  checkButtons();
  calloop();
  // accel_readings();

  // bluetooth loop
  // aci_loop(); // Process ACI commands/events, must run frequently
  // // Check if data is received via Bluetooth
  // if (ble_rx_buffer_len) {
  //   SerialMonitorInterface.print(ble_rx_buffer_len);
  //   SerialMonitorInterface.print(" : ");
  //   SerialMonitorInterface.println((char *)ble_rx_buffer);
  //   // Send the same message as a notification
  //   // sendNotification((char*)ble_rx_buffer);
  //   ble_rx_buffer_len = 0; // Clear buffer after reading
  // }
  // // Check if serial input is available to send via Bluetooth
  // if (SerialMonitorInterface.available()) {
  //   delay(10); // Small delay to catch input
  //   uint8_t sendBuffer[21];
  //   uint8_t sendLength = 0;
  //   while (SerialMonitorInterface.available() && sendLength < 19) {
  //     sendBuffer[sendLength] = SerialMonitorInterface.read();
  //     sendLength++;
  //   }
  //   if (SerialMonitorInterface.available()) {
  //     SerialMonitorInterface.print(F("Input truncated, dropped: "));
  //     if (SerialMonitorInterface.available()) {
  //       SerialMonitorInterface.write(SerialMonitorInterface.read());
  //     }
  //   }
  //   sendBuffer[sendLength] = '\0'; // Terminate string
  //   sendLength++;
  //   if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t *)sendBuffer, sendLength)) {
  //     SerialMonitorInterface.println(F("TX dropped!"));
  //   }
  // }

}

// functions for menu
uint32_t millisOffset() {
  return millis();
}

uint32_t getSecondsCounter() {
  return millis()/1000;
}

int requestScreenOn() {
  sleepTimer = millisOffset();
  if (!displayOn) {
    setTime(counter);
    displayOn = 1;
    updateMainDisplay();
    return 1;
  }
  return 0;
}

void checkButtons() {
  byte buttons = display.getButtons();
  if (buttonReleased && buttons) {
    if (displayOn) {
      buttonPress(buttons);
    }
    requestScreenOn();
    buttonReleased = 0;
  }
  if (!buttonReleased && !(buttons & 0x1F)) {
    buttonReleased = 1;
  }
}

void buttonPress(uint8_t buttons) {
  if (currentDisplayState == displayStateHome) {
    if (buttons == viewButton) {
      currentDisplayState = displayHeartRate;
      bpmsetup();
    } else if (buttons == selectButton) {
      currentDisplayState = displayChineseHome;
      lang = 1;
    } else if (buttons == menuButton) {
      menuHandler = viewMenu;
      newMenu(emergencyMenuIndex);
      menuHandler(0);
      currentDisplayState = displayEmergency;
    }
  } else if (currentDisplayState == displayHeartRate) {
    if (buttons == viewButton) {
      currentDisplayState = displayCalendar;
      calsetup();
      slowUpdateRate = false;
    } else if (buttons == selectButton) {
      currentDisplayState = displayStateHome;
      slowUpdateRate = false;
    }
  } else if (currentDisplayState == displayFall) {
    if (buttons == viewButton) {
      currentDisplayState = displayStateHome;
      
    } else if (buttons == selectButton) {
      currentDisplayState = displayStateHome;
    }
  } else if (currentDisplayState == displayChineseHome) {
    if (buttons == viewButton) {
      currentDisplayState = displayHeartRate;
      bpmsetup();
    } else if (buttons == selectButton) {
      currentDisplayState = displayMalayHome;
      lang = 2;
    }
  } else if (currentDisplayState == displayMalayHome) {
    if (buttons == viewButton) {
      currentDisplayState = displayHeartRate;
      bpmsetup();
    } else if (buttons == selectButton) {
      currentDisplayState = displayStateHome;
      lang = 0;
    }
  } else if (currentDisplayState == displayEmergency) {
    if (buttons == selectButton) {
      currentDisplayState = displayStateHome;
      lang = 0;
    }
  }
}

void initHomeScreen() {
  displayBuffer.clearWindow(0, 1, 96, 5);
  rewriteTime = true;
  rewriteMenu = true;
  updateMainDisplay();
}

void updateMainDisplay() {
  if (y > 350){
    prevDisplayState = currentDisplayState;
    currentDisplayState = displayFall;
  }
  if (lastSetBrightness != brightness) {
    display.setBrightness(brightness);
    lastSetBrightness = brightness;
  }
  updateDateTimeDisplay();
  if (currentDisplayState == displayStateHome || currentDisplayState == displayChineseHome || currentDisplayState == displayMalayHome) {
    displayBuffer.setCursor(9, menuTextY[6]);
    if (lang == 0) {
      displayBuffer.print("Emergency");
    } else if (lang == 1) {
      drawCharacter(9, menuTextY[6]-3, jin, 12, 11.5);
      drawCharacter(22, menuTextY[6]-3, ji, 12, 11.5);
    } else if (lang == 2) {
      displayBuffer.print("Kecemasan");
    }
    
    displayBuffer.setCursor(67, menuTextY[6]);
    if (lang == 0 || lang == 2) {
      displayBuffer.print("BPM");
    } else {
      drawCharacter(58, menuTextY[6]-4, xin, 12, 11.5);
      drawCharacter(71, menuTextY[6]-4, lv, 12, 11.5);
    }
    
    leftArrow(0, 57);
    rightArrow(90, 20);
    rightArrow(90, 57);
    rewriteMenu = false;
  }
}


void updateDateTimeDisplay() {

  displayBuffer.clearWindow(0, 0, 96, 8);
  displayBuffer.setCursor(0, -1); 
  int currentDay = day();
  lastDisplayedDay = currentDay;
  if (rtc.getHours() + GMT >= 24) {
    if (weekday() == 7) {
      displayBuffer.print(dayShortStr(1));
    } else {
      displayBuffer.print(dayShortStr(weekday()+1));
    }
    displayBuffer.print(' ');
    displayBuffer.print(rtc.getDay() + 1);
    displayBuffer.print("/");
    displayBuffer.print(rtc.getMonth());
    lastHourDisplayed = rtc.getHours() + GMT - 24;

  } else {
    displayBuffer.print(dayShortStr(weekday()));
    displayBuffer.print(' ');
    displayBuffer.print(rtc.getDay());
    displayBuffer.print("/");
    displayBuffer.print(rtc.getMonth());
    lastHourDisplayed = rtc.getHours() + GMT;

  }
  displayBuffer.print("  ");
  lastMinuteDisplayed = rtc.getMinutes();
  lastSecondDisplayed = rtc.getSeconds();
  int hour12 = lastHourDisplayed;
  int AMPM = 1;
  if (hour12 > 12) {
    AMPM = 2;
    hour12 -= 12;
  }
  lastHourDisplayed = hour12;
  displayBuffer.setCursor(58, -1);
  if (lastHourDisplayed < 10)displayBuffer.print('0');
  displayBuffer.print(lastHourDisplayed);
  displayBuffer.write(':');
  if (lastMinuteDisplayed < 10)displayBuffer.print('0');
  displayBuffer.print(lastMinuteDisplayed);
  displayBuffer.setCursor(80, -1);
  if (AMPM == 2) {
    displayBuffer.print(" PM");
  } else {
    displayBuffer.print(" AM");
  }
  rewriteTime = false;
}

void liveDisplay() {

  displayBuffer.setCursor(0, menuTextY[1]);
  if (lang == 0) {
    displayBuffer.print("Reminder:");
  } else if (lang == 1) {
    drawCharacter(0, menuTextY[1]-1, ti, 12, 11.5);
    drawCharacter(13, menuTextY[1]-1, xing, 12, 11.5);
    displayBuffer.setCursor(26, menuTextY[1]);
    displayBuffer.print(":");
  } else if (lang == 2) {
    displayBuffer.print("Peringatan:");
  }
  displayBuffer.setCursor(65, menuTextY[1]);
  displayBuffer.print("Stop");

  displayBuffer.setCursor(0, menuTextY[3]);
  displayBuffer.print(reminders[0].medication);
  displayBuffer.setCursor(0, menuTextY[4]);
  displayBuffer.print(reminders[0].timeStr);
  
}

// languange function
void drawCharacter(uint8_t x, uint8_t y, const uint8_t* bitmap, uint8_t width, uint8_t height) {
  // Loop through each row of the character
  for (uint8_t i = 0; i < height; i++) {
    uint16_t row = (bitmap[i * 2] << 8) | bitmap[i * 2 + 1];  // Combine two bytes to form a 16-bit row

    // Loop through each pixel in the row (16 pixels)
    for (uint8_t j = 0; j < width; j++) {
      if (row & (1 << (15 - j))) {  // Check if the j-th bit is set (pixel is on)
        displayBuffer.drawPixel(x + j, y + i, 0xFFFF);  // Draw the pixel at position (x+j, y+i) in white
      }
    }
  }
}

// bpm functions
void bpmsetup() {
  // initialise pulse sensor
  if (pulseSensorPort) {
    Wireling.selectPort(pulseSensorPort);
    pulseSensor.begin(); 
  }
  for (int i = 0; i < graphWidth; i++) {
    samples[i] = 0.0;
  }
}

void drawGraph(GraphicsBuffer * sbuff, int xDispPos, int yDispPos, int gWidth, int gHeight, float samples[]) {
  for (uint8_t x = xDispPos; x < xDispPos + gWidth; x++) {
    sbuff->drawLine( x, yDispPos, x, yDispPos + gHeight, 0);
  }
  int midPoint = yDispPos + gHeight / 2;
  for (int x = 1; x < gWidth; x++) {
    int sample0 = (float)(samples[x - 1] * (float)gHeight / 2.0);
    int sample1 = (float)(samples[x    ] * (float)gHeight / 2.0);
    sbuff->drawLine(xDispPos + x - 1, midPoint - sample0, xDispPos + x, midPoint - sample1, 20);
  }
}

void bpmloop() {
  Wireling.selectPort(pulseSensorPort);
  if (pulseSensor.update()) {
    if (slowUpdateRate) { // double each result to increase perceived speed
      for (int i = 2; i < graphWidth / 2; i++) {
        samples[i * 2 - 1] = samples[i * 2];
        samples[i * 2 - 2] = samples[i * 2];
      }
      samples[graphWidth - 1] = pulseSensor.cardiogram();
      samples[graphWidth - 2] = pulseSensor.cardiogram();
    } else {
      for (int i = 1; i < graphWidth; i++) {
        samples[i - 1] = samples[i];
      }
      samples[graphWidth - 1] = pulseSensor.cardiogram();
    }

    displayBuffer.clear(); 
    drawGraph(&displayBuffer, displayBuffer.width - graphWidth, graphYPos, graphWidth, graphHeight, samples);

    if (pulseSensor.pulseValid()) {

      // Get BPM and oxygen values
      int bpm = round(pulseSensor.BPM());
      int oxygen = round(pulseSensor.oxygen());
      int temp = round(pulseSensor.temperature());

      // Define thresholds
      int bpmLow = 60; // Low BPM threshold
      int bpmHigh = 70; // High BPM threshold (Actual: 100) (Set to 70 for testing)
      int oxygenLow = 95; // Low oxygen threshold (Actual: 90) (Set to 95 for testing)

      displayBuffer.setCursor(menuTextX[2], menuTextY[2]);
      displayBuffer.print("BPM: ");
      displayBuffer.print(bpm);
      
      displayBuffer.setCursor(menuTextX[1] - 5, menuTextY[1]);
      displayBuffer.print("Temp:");
      displayBuffer.print(temp);
      displayBuffer.print("C");

      displayBuffer.setCursor(menuTextX[0], menuTextY[0]); 
      displayBuffer.print("O2: ");
      displayBuffer.print(oxygen);

      // Check thresholds and display notifications 
      displayBuffer.fontColor(RED, BLACK); // 

      if (bpm < bpmLow) {
        displayBuffer.setCursor(menuTextX[2] + 40, menuTextY[2]); 
        displayBuffer.print("Low!");
      } else if (bpm > bpmHigh) {
        displayBuffer.setCursor(menuTextX[2] + 40, menuTextY[2]);
        displayBuffer.print("High!");
      }

      if (oxygen < oxygenLow) {
        displayBuffer.setCursor(menuTextX[0] + 35, menuTextY[0]); 
        displayBuffer.print("Low!");
      }
      displayBuffer.fontColor(WHITE, BLACK); 

    } else {
      displayBuffer.setCursor(menuTextX[2], menuTextY[2]);
      displayBuffer.print("BPM: -");

      displayBuffer.setCursor(menuTextX[1] - 5, menuTextY[1]);
      displayBuffer.print("Temp: -");

      displayBuffer.setCursor(menuTextX[0], menuTextY[0]);
      displayBuffer.print("O2: -");
    }

    display.endTransfer();
    display.goTo(0, 0);
    display.startData();
    display.writeBufferDMA(displayBuffer.getBuffer(), displayBuffer.getBufferSize());
  }

  Wire.setClock(500000);
  display.writeBuffer(displayBuffer.getBuffer(), displayBuffer.getBufferSize());
  Wire.setClock(100000);
}

// accelerometer functions
void accel_readings(){
  accel_sensor.read();
  x = accel_sensor.X;
  y = accel_sensor.Y;
  z = accel_sensor.Z;
  temp = ((accel_sensor.rawTemp * 0.5) + 24.0);
  if (x == -1 && y == -1 && z == -1) {
    Serial.print("ERROR! NO BMA250 DETECTED!");
  }
  else { 
    showSerial();
    delay(250);
  }
}

// calendar functions
void calsetup() {

    display.clearScreen();
    pinMode(buzzerPin, OUTPUT);

}

void calloop() {
    // Check if it's time to start the next alarm
    if (!isAlarmActive && currentReminderIndex < numReminders) {
        if (getHourAsInt(reminders[0].timeStr) == rtc.getHours()+GMT) {
            calsetup();
            isAlarmActive = true;
            startAlarm(); // Display reminder and start alarm
        }// Play alarm melody and display reminder if the alarm is active
    }
    if (isAlarmActive) {
        playAlarmMelody();
        delay(10);
        // Check for "Stop Alarm" button press
        if (checkForStop()) {
            stopAlarm(); // Stop the alarm when button is pressed
        }
    }
    time_t currentTime = now(); // Get current time
    if (currentTime - previousTime >= interval) {
        fetchCalendarEvents();
        previousTime = currentTime; 
    }
}

// Function to fetch events from Google Calendar
void fetchCalendarEvents() {
  if (client.connect(serverHost, serverPort)) {
    SerialUSB.println("Fetching events from Node.js server...");

    // Send GET request
    client.println("GET / HTTP/1.1");
    client.print("Host: ");
    client.println(serverHost);
    client.println("Connection: close");
    client.println(); // End of headers

    // Read the server response
    String rawResponse = "";
    while (client.connected() || client.available()) {
      if (client.available()) {
        rawResponse += client.readString();
      }
    }
    client.stop();
    
    SerialUSB.println("Raw server response:");
    SerialUSB.println(rawResponse);

    // Strip HTTP headers
    int jsonStart = rawResponse.indexOf("\r\n\r\n");
    if (jsonStart != -1) {
      rawResponse = rawResponse.substring(jsonStart + 4); // Skip past the headers
    }

    SerialUSB.println("Trimmed JSON response:");
    SerialUSB.println(rawResponse);

    // Parse the JSON response
    parseAndStoreEvents(rawResponse);
  } else {
    SerialUSB.println("Failed to connect to Node.js server.");
  }
}


// Function to parse and store events
void parseAndStoreEvents(String jsonResponse) {
    const size_t capacity = 2048; // Adjust this based on JSON size
    DynamicJsonDocument doc(capacity);

    // Parse JSON
    DeserializationError error = deserializeJson(doc, jsonResponse);
    if (error) {
        SerialUSB.print("JSON parse failed: ");
        SerialUSB.println(error.f_str());
        return;
    }

    // Iterate over events and store them in reminders array
    JsonArray events = doc.as<JsonArray>();
    numReminders = 0;

    for (JsonObject event : events) {
        if (numReminders >= MAX_REMINDERS) break;

        String summary = event["summary"].as<String>();
        String startTime = event["start"]["dateTime"].as<String>();

        reminders[numReminders].medication = summary;
        reminders[numReminders].timeStr = formatDateTime(startTime);

        // Set the trigger time relative to millis() (for simulation purposes)
        reminders[numReminders].triggerTime = millis() + (numReminders + 1) * 20000; // Each alarm 10 seconds apart
        numReminders++;
    }

    SerialUSB.println("Reminders updated:");
    for (int i = 0; i < numReminders; i++) {
        SerialUSB.print("Reminder ");
        SerialUSB.print(i + 1);
        SerialUSB.print(": ");
        SerialUSB.println(reminders[i].medication);
    }
}

// Function to format dateTime
String formatDateTime(String dateTime) {
    // Example input: "2024-11-22T02:30:00+08:00"
    String time = dateTime.substring(11, 16); // Extract "HH:MM"
    return time;
}

int getHourAsInt(String dateTime) {
    String hourString = dateTime.substring(0, 2); 
    int hour = hourString.toInt(); 
    return hour;
}

// Function to start the alarm
void startAlarm() {
  display.clearScreen();
  display.fontColor(TS_8b_White, TS_8b_Black);

  // Display all reminders for the current time
  display.setCursor(0, 0);
  display.print("Time: ");
  display.print(reminders[currentReminderIndex].timeStr);

  int yPos = 16; // Start displaying medication at y = 16
  for (int i = currentReminderIndex; i < numReminders; i++) {
    // Check if the reminder matches the current time
    if (reminders[i].timeStr == reminders[currentReminderIndex].timeStr) {
      display.setCursor(0, yPos);
      display.print(reminders[i].medication);
      yPos += 16; // Move to the next line for the next medication
    } else {
      break; // Stop checking once times no longer match
    }
  }

  // No need to call display.display(); as the screen updates automatically
}


// Function to play the alarm melody
void playAlarmMelody() {
    unsigned long currentMillis = millis();
    if (currentNoteIndex < sizeof(melody) / sizeof(melody[0])) {
        if (currentMillis - melodyMillis >= (1000 / noteDurations[currentNoteIndex])) {
            melodyMillis = currentMillis;
            tone(buzzerPin, melody[currentNoteIndex], 500);
            currentNoteIndex++;
        }
    } else {
        if (currentMillis - melodyMillis >= melodyPauseDuration) {
            currentNoteIndex = 0;
            melodyMillis = currentMillis;
        }
    }
}

// Function to stop the alarm
void stopAlarm() {
  noTone(buzzerPin);
  isAlarmActive = false;
  display.clearScreen();
  display.fontColor(TS_8b_Blue, TS_8b_Black);
  display.setCursor(10, 20);
  display.print("Alarm Stopped!");
  delay(2000);
  display.clearScreen();

  // Skip reminders with the same time
  String currentAlarmTime = reminders[currentReminderIndex].timeStr;
  while (currentReminderIndex < numReminders &&
         reminders[currentReminderIndex].timeStr == currentAlarmTime) {
    currentReminderIndex++;
  }
}

// Function to detect "Stop Alarm" button press
bool checkForStop() {
    uint8_t buttons = display.getButtons();
    return buttons & TSButtonUpperRight;
}

//bluetooth functions
#if BLE_DEBUG
#include <stdio.h>
char sprintbuff[100];
#define PRINTF(...) {sprintf(sprintbuff,__VA_ARGS__);SerialMonitorInterface.print(sprintbuff);}
#else
#define PRINTF(...)
#endif

volatile uint8_t set_connectable = 1;
uint16_t connection_handle = 0;

#define  ADV_INTERVAL_MIN_MS  50
#define  ADV_INTERVAL_MAX_MS  100

int connected = FALSE;

int BLEsetup() {
  int ret;

  HCI_Init();
  /* Init SPI interface */
  BNRG_SPI_Init();
  /* Reset BlueNRG/BlueNRG-MS SPI interface */
  BlueNRG_RST();

  uint8_t bdaddr[] = {0x12, 0x34, 0x00, 0xE1, 0x80, 0x02};

  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bdaddr);

  if (ret) {
    PRINTF("Setting BD_ADDR failed.\n");
  }

  ret = aci_gatt_init();

  if (ret) {
    PRINTF("GATT_Init failed.\n");
  }

  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  ret = aci_gap_init_IDB05A1(GAP_PERIPHERAL_ROLE_IDB05A1, 0, 0x07, &service_handle, &dev_name_char_handle, &appearance_char_handle);

  if (ret) {
    PRINTF("GAP_Init failed.\n");
  }

  const char *name = "BlueNRG";

  ret = aci_gatt_update_char_value(service_handle, dev_name_char_handle, 0, strlen(name), (uint8_t *)name);

  if (ret) {
    PRINTF("aci_gatt_update_char_value failed.\n");
  } else {
    PRINTF("BLE Stack Initialized.\n");
  }

  ret = Add_UART_Service();

  if (ret == BLE_STATUS_SUCCESS) {
    PRINTF("UART service added successfully.\n");
  } else {
    PRINTF("Error while adding UART service.\n");
  }

  /* +4 dBm output power */
  ret = aci_hal_set_tx_power_level(1, 3);
}

void aci_loop() {
  HCI_Process();
  ble_connection_state = connected;
  if (set_connectable) {
    setConnectable();
    set_connectable = 0;
  }
  if (HCI_Queue_Empty()) {
    //Enter_LP_Sleep_Mode();
  }
}

#define COPY_UUID_128(uuid_struct, uuid_15, uuid_14, uuid_13, uuid_12, uuid_11, uuid_10, uuid_9, uuid_8, uuid_7, uuid_6, uuid_5, uuid_4, uuid_3, uuid_2, uuid_1, uuid_0) \
  do {\
    uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; uuid_struct[2] = uuid_2; uuid_struct[3] = uuid_3; \
    uuid_struct[4] = uuid_4; uuid_struct[5] = uuid_5; uuid_struct[6] = uuid_6; uuid_struct[7] = uuid_7; \
    uuid_struct[8] = uuid_8; uuid_struct[9] = uuid_9; uuid_struct[10] = uuid_10; uuid_struct[11] = uuid_11; \
    uuid_struct[12] = uuid_12; uuid_struct[13] = uuid_13; uuid_struct[14] = uuid_14; uuid_struct[15] = uuid_15; \
  }while(0)

#define COPY_UART_SERVICE_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x6E, 0x40, 0x00, 0x01, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E)
#define COPY_UART_TX_CHAR_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x6E, 0x40, 0x00, 0x02, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E)
#define COPY_UART_RX_CHAR_UUID(uuid_struct)  COPY_UUID_128(uuid_struct,0x6E, 0x40, 0x00, 0x03, 0xB5, 0xA3, 0xF3, 0x93, 0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E)

uint16_t UARTServHandle, UARTTXCharHandle, UARTRXCharHandle;


uint8_t Add_UART_Service(void)
{
  tBleStatus ret;
  uint8_t uuid[16];
  COPY_UART_SERVICE_UUID(uuid);
  ret = aci_gatt_add_serv(UUID_TYPE_128,  uuid, PRIMARY_SERVICE, 7, &UARTServHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;
  COPY_UART_TX_CHAR_UUID(uuid);
  ret =  aci_gatt_add_char(UARTServHandle, UUID_TYPE_128, uuid, 20, CHAR_PROP_WRITE_WITHOUT_RESP, ATTR_PERMISSION_NONE, GATT_NOTIFY_ATTRIBUTE_WRITE,
                           16, 1, &UARTTXCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;
  COPY_UART_RX_CHAR_UUID(uuid);
  ret =  aci_gatt_add_char(UARTServHandle, UUID_TYPE_128, uuid, 20, CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE, 0,
                           16, 1, &UARTRXCharHandle);
  if (ret != BLE_STATUS_SUCCESS) goto fail;
  return BLE_STATUS_SUCCESS;
fail:
  PRINTF("Error while adding UART service.\n");
  return BLE_STATUS_ERROR ;
}



uint8_t lib_aci_send_data(uint8_t ignore, uint8_t* sendBuffer, uint8_t sendLength) {
  return !Write_UART_TX((char*)sendBuffer, sendLength);
}

uint8_t Write_UART_TX(char* TXdata, uint8_t datasize)
{
  tBleStatus ret;
  ret = aci_gatt_update_char_value(UARTServHandle, UARTRXCharHandle, 0, datasize, (uint8_t *)TXdata);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Error while updating UART characteristic.\n") ;
    return BLE_STATUS_ERROR ;
  }
  return BLE_STATUS_SUCCESS;
}

void Read_Request_CB(uint16_t handle)
{
  if (connection_handle != 0)
    aci_gatt_allow_read(connection_handle);
}

void setConnectable(void)
{
  tBleStatus ret;
  const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME, 'B', 'l', 'u', 'e', 'N', 'R', 'G'};
  hci_le_set_scan_resp_data(0, NULL);
  PRINTF("General Discoverable Mode.\n");
  ret = aci_gap_set_discoverable(ADV_IND,
                                 (ADV_INTERVAL_MIN_MS * 1000) / 625, (ADV_INTERVAL_MAX_MS * 1000) / 625,
                                 STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
                                 sizeof(local_name), local_name, 0, NULL, 0, 0);
  if (ret != BLE_STATUS_SUCCESS)
    PRINTF("%d\n", (uint8_t)ret);

}

void Attribute_Modified_CB(uint16_t handle, uint8_t data_length, uint8_t *att_data)
{
  if (handle == UARTTXCharHandle + 1) {
    int i;
    for (i = 0; i < data_length; i++) {
      ble_rx_buffer[i] = att_data[i];
    }
    ble_rx_buffer[i] = '\0';
    ble_rx_buffer_len = data_length;
  }
}

void GAP_ConnectionComplete_CB(uint8_t addr[6], uint16_t handle) {
  connected = TRUE;
  connection_handle = handle;
  PRINTF("Connected to device:");
  for (int i = 5; i > 0; i--) {
    PRINTF("%02X-", addr[i]);
  }
  PRINTF("%02X\r\n", addr[0]);
}

void GAP_DisconnectionComplete_CB(void) {
  connected = FALSE;
  PRINTF("Disconnected\n");
  /* Make the device connectable again. */
  set_connectable = TRUE;
}

void sendNotification(const char* message) {
    uint8_t messageLength = strlen(message);
    if (Write_UART_TX((char*)message, messageLength) == BLE_STATUS_SUCCESS) {
        SerialMonitorInterface.println("Notification sent to phone!");
    } else {
        SerialMonitorInterface.println("Failed to send notification.");
    }
}

void HCI_Event_CB(void *pckt)
{
  hci_uart_pckt *hci_pckt = (hci_uart_pckt *)pckt;
  hci_event_pckt *event_pckt = (hci_event_pckt*)hci_pckt->data;
  if (hci_pckt->type != HCI_EVENT_PKT)
    return;
  switch (event_pckt->evt) {
    case EVT_DISCONN_COMPLETE:
      {
        //evt_disconn_complete *evt = (void *)event_pckt->data;
        GAP_DisconnectionComplete_CB();
      }
      break;
    case EVT_LE_META_EVENT:
      {
        evt_le_meta_event *evt = (evt_le_meta_event *)event_pckt->data;

        switch (evt->subevent) {
          case EVT_LE_CONN_COMPLETE:
            {
              evt_le_connection_complete *cc = (evt_le_connection_complete *)evt->data;
              GAP_ConnectionComplete_CB(cc->peer_bdaddr, cc->handle);
            }
            break;
        }
      }
      break;
    case EVT_VENDOR:
      {
        evt_blue_aci *blue_evt = (evt_blue_aci *)event_pckt->data;
        switch (blue_evt->ecode) {
          case EVT_BLUE_GATT_READ_PERMIT_REQ:
            {
              evt_gatt_read_permit_req *pr = (evt_gatt_read_permit_req *)blue_evt->data;
              Read_Request_CB(pr->attr_handle);
            }
            break;
          case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
            {
              evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)blue_evt->data;
              Attribute_Modified_CB(evt->attr_handle, evt->data_length, evt->att_data);
            }
            break;
        }
      }
      break;
  }
}