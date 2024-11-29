void setupWiFi(const char* ssid, const char* password) {
    WiFi.setPins(8, 2, A3, -1); // Configure WiFi pins
    delay(2000);
    while (WiFi.status() != WL_CONNECTED) {
        SerialMonitorInterface.print("Attempting to connect to SSID: ");
        SerialMonitorInterface.println(ssid);
        WiFi.begin(ssid, password);
        delay(2000);
    }
    SerialMonitorInterface.println("Connected to WiFi!");
}

void setRTCfromWifi() {
  rtc.begin();
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime();
    numberOfTries++;
  } while ((epoch == 0) && (numberOfTries < maxTries));
  if (numberOfTries == maxTries) {
    Serial.print("NTP unreachable!!");
    while (1);
  } else {
    Serial.print("Epoch received: ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);
    Serial.println();
  }
}

void showSerial() {
  SerialMonitorInterface.print("X = ");
  SerialMonitorInterface.print(x);
  
  SerialMonitorInterface.print("  Y = ");
  SerialMonitorInterface.print(y);
  
  SerialMonitorInterface.print("  Z = ");
  SerialMonitorInterface.print(z);
  
  SerialMonitorInterface.print("  Temperature(C) = ");
  SerialMonitorInterface.println(temp);
}

void fallDetection(){
  char *detection[] = {
        "Fall Detected",
        "Are you OK?",
        "Yes         No"
    };

  for(int i = 0; i < 3; i++){
    char buffer[20];
    strcpy(buffer, &(*detection[i]));
    Serial.println("buffer: ");
    Serial.println(buffer);
    int width = displayBuffer.getPrintWidth(buffer);
    displayBuffer.setCursor(96 / 2 - width / 2, 16*(i+1));
    displayBuffer.print(buffer);
  }

  leftArrow(90, 15 + 2);
  rightArrow(90, 45 + 4); 

}