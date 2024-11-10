#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#define EEPROM_SIZE 32

const int ledPin1 = LED_BUILTIN;  // Pin connected to the LED
const int Light = 22;
const int MotorA = 32;
const int MotorB = 33;
const int MotorOn = 36;
const int MotorOff = 39;
const int Float_Valve = 14;  //when 0 valve will colse, when 1 valve will open
const int Float_Motor = 27;  //when 1 water pump will ON other wise Off
const int Valve_Open = 26;   // 0 when valve open
const int Valve_Close = 25;  // 0 when valve closed
const int Valve_M1 = 16;     //valve controlling motor pin
const int Valve_M2 = 17;     //valve controlling motor pin
const int Water_Motor = 18;   // water pump on of pin
long OnOffTime = 1500;
// NTP settings
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 5.5;  // Adjust this to your time zone offset (in hours)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServerName);

WebServer server(80);

bool ledState1 = LOW;
bool ledState2 = LOW;

unsigned long onTime1 = 0;
unsigned long offTime1 = 0;
unsigned long onTime2 = 0;
unsigned long offTime2 = 0;

void writeWordsToEEPROM(int addrOffset, const String& word1, const String& word2) {
  int length1 = word1.length();
  int length2 = word2.length();

  EEPROM.write(addrOffset, length1);
  for (int i = 0; i < length1; ++i) {
    EEPROM.write(addrOffset + 1 + i, word1[i]);
  }

  EEPROM.write(addrOffset + 1 + length1, length2);
  for (int i = 0; i < length2; ++i) {
    EEPROM.write(addrOffset + 2 + length1 + i, word2[i]);
  }

  EEPROM.commit();
}

String readWordFromEEPROM(int addrOffset) {
  int length = EEPROM.read(addrOffset);
  String word = "";
  for (int i = 0; i < length; ++i) {
    word += char(EEPROM.read(addrOffset + 1 + i));
  }
  return word;
}

void setup() {
  pinMode(ledPin1, OUTPUT);
  digitalWrite(ledPin1, ledState1);
  pinMode(Light, OUTPUT);
  digitalWrite(Light, ledState2);
  pinMode(MotorA, OUTPUT);
  digitalWrite(MotorA, LOW);
  pinMode(MotorB, OUTPUT);
  digitalWrite(MotorB, LOW);
  pinMode(MotorOn, INPUT);
  pinMode(MotorOff, INPUT);
  pinMode(Float_Valve, INPUT);
  pinMode(Float_Motor, INPUT);
  pinMode(Valve_Close, INPUT);
  pinMode(Valve_Open, INPUT);
  pinMode(Valve_M1, OUTPUT);
  digitalWrite(Valve_M1, 0);
  pinMode(Valve_M2, OUTPUT);
  digitalWrite(Valve_M2, 0);
  pinMode(Water_Motor, OUTPUT);
  digitalWrite(Water_Motor, 0);
  Serial.begin(115200);
  EEPROM.begin(512);
  if (digitalRead(MotorOff)) {
    digitalWrite(ledPin1, LOW);
    ledState1 = LOW;
    digitalWrite(MotorA, 0);
    digitalWrite(MotorB, 1);
    //delay(OnOffTime);
    while (digitalRead(MotorOff)) {
      delay(10);
    }
    digitalWrite(MotorA, 0);
    digitalWrite(MotorB, 0);
    Serial.print("Motor OFF");
  }
  // Connect to Wi-Fi
  // Read SSID from EEPROM
  String word1 = readWordFromEEPROM(0);
  String word2 = readWordFromEEPROM(1 + word1.length());
  const char* ssid = word1.c_str();
  const char* password = word2.c_str();
  // Check if SSID is stored in EEPROM
  if (strlen(ssid) == 0) {
    configureWiFi();
  } else {
    // Read password from EEPROM
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    int connectionTimeout = 10;  // Timeout in seconds
    while (WiFi.status() != WL_CONNECTED && connectionTimeout > 0) {
      delay(1000);
      Serial.print(".");
      connectionTimeout--;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println();
      Serial.println("WiFi connection failed. Please configure WiFi.");
      configureWiFi();
    }
  }
  // Initialize NTP
  timeClient.begin();
  timeClient.setTimeOffset(19800);  //(timeZone * 3600);
                                    // timeClient.update();

  // Define web server routes
  server.on("/", handleRoot);
  server.on("/on1", handleOn1);
  server.on("/off1", handleOff1);
  server.on("/on2", handleOn2);
  server.on("/off2", handleOff2);
  server.on("/setTimes", handleSetTimes);
  server.on("/status", handleStatus);

  server.begin();
}

void loop() {
  server.handleClient();

  // Get the current time from the NTP server
  timeClient.update();
  unsigned long currentTime = timeClient.getEpochTime();

  int hours = (currentTime % 86400L) / 3600;  // 86400 seconds in a day, 3600 seconds in an hour
  int minutes = (currentTime % 3600) / 60;    // 3600 seconds in an hour, 60 seconds in a minute
  int seconds = currentTime % 60;
  currentTime = hours * 3600 + minutes * 60 + seconds;

  // Serial.print("currentTime");
  // Serial.println(currentTime);
  // Serial.print("On Time");
  // Serial.println(onTime);
  delay(500);
  // Check if it's time to turn the LED on or off
  if (ledState1 == LOW && currentTime >= onTime1 && currentTime < offTime1) {
    digitalWrite(ledPin1, HIGH);
    ledState1 = HIGH;
    digitalWrite(MotorA, HIGH);
    digitalWrite(MotorB, 0);
    //delay(OnOffTime);
    while (int i = digitalRead(MotorOn) != 0) {
      delay(10);
    }
    digitalWrite(MotorA, 0);
    digitalWrite(MotorB, 0);
    Serial.print(" Motor ON");
  } else if (ledState1 == HIGH && (currentTime >= offTime1 || currentTime < onTime1)) {
    digitalWrite(ledPin1, LOW);
    ledState1 = LOW;
    digitalWrite(MotorA, 0);
    digitalWrite(MotorB, 1);
    //delay(OnOffTime);
    while (int i = digitalRead(MotorOff) != 0) {
      delay(10);
    }
    digitalWrite(MotorA, 0);
    digitalWrite(MotorB, 0);
    Serial.print("Motor OFF");
  }
  if (ledState2 == LOW && currentTime >= onTime2 && currentTime < offTime2) {
    digitalWrite(Light, HIGH);
    ledState2 = HIGH;
  } else if (ledState2 == HIGH && (currentTime >= offTime2 || currentTime < onTime2)) {
    digitalWrite(Light, LOW);
    ledState2 = LOW;
  }

  if (digitalRead(Float_Valve) == 0 && digitalRead(Valve_Close) == 1) {
    digitalWrite(Valve_M1, 0);
    digitalWrite(Valve_M2, HIGH);
    while (digitalRead(Valve_Close) == 1) {
      delay(50);
    }
    digitalWrite(Valve_M1, 0);
    digitalWrite(Valve_M2, 0);
  }

  if (digitalRead(Float_Valve) == 1 && digitalRead(Valve_Open) == 1) {
    digitalWrite(Valve_M1, HIGH);
    digitalWrite(Valve_M2, 0);
    while (digitalRead(Valve_Open) == 1) {
      delay(50);
    }
    digitalWrite(Valve_M1, 0);
    digitalWrite(Valve_M2, 0);
  }

  if (digitalRead(Float_Motor) == 1) {
    digitalWrite(Water_Motor, HIGH);
  } else {
    digitalWrite(Water_Motor, LOW);
  }
}

void handleStatus() {
  String json = "{";
  json += "\"valveState\":\"" + String(ledState1 == HIGH ? "On" : "Off") + "\",";
  json += "\"lightState\":\"" + String(ledState2 == HIGH ? "On" : "Off") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  String formattedOnTime1 = formatTime(onTime1);
  String formattedOffTime1 = formatTime(offTime1);
  String formattedOnTime2 = formatTime(onTime2);
  String formattedOffTime2 = formatTime(offTime2);

  String html = "<html><head>";
  html += "<title>Table Top Grower</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f4f4f4; color: #333; }";
  html += ".container { width: 80%; margin: auto; overflow: hidden; padding: 20px; background: #fff; border-radius: 8px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }";
  html += "h1 { font-family: 'Times New Roman', Times, serif; text-align: center; color: #007BFF; }";
  html += "p, label { font-size: 1.2em; }";
  html += "a { display: inline-block; text-decoration: none; color: #fff; background-color: #007BFF; padding: 10px 20px; border-radius: 5px; margin: 5px 0; }";
  html += "form { margin-top: 20px; }";
  html += "input[type='text'], input[type='time'], input[type='number'], input[type='submit'] { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; box-sizing: border-box; font-size: 1em; }";
  html += "input[type='submit'] { background-color: #28a745; color: #fff; cursor: pointer; }";
  html += "input[type='submit']:hover { background-color: #218838; }";
  html += "</style>";
  html += "<script>";
  html += "function updateTime() {";
  html += "  var now = new Date();";
  html += "  var hours = now.getHours().toString().padStart(2, '0');";
  html += "  var minutes = now.getMinutes().toString().padStart(2, '0');";
  html += "  var seconds = now.getSeconds().toString().padStart(2, '0');";
  html += "  var currentTime = hours + ':' + minutes + ':' + seconds;";
  html += "  document.getElementById('currentTime').innerText = currentTime;";
  html += "}";
  html += "function updateStatus() {";
  html += "  fetch('/status')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('valveStatus').innerText = 'Valve: ' + data.valveState;";
  html += "      document.getElementById('lightStatus').innerText = 'Light: ' + data.lightState;";
  html += "    })";
  html += "    .catch(error => console.error('Error fetching status:', error));";
  html += "}";
  html += "setInterval(updateTime, 1000);";
  html += "setInterval(updateStatus, 1000);";
  html += "</script>";
  html += "</head><body onload='updateTime();updateStatus()'>";
  html += "<div class='container'>";
  html += "<h1>Table Top Grower M1 Control</h1>";
  // Display current on and off times
  html += "<div><h2>Current Times:</h2>";
  html += "<p>Valve On Time: " + formattedOnTime1 + "</p>";
  html += "<p>Valve Off Time: " + formattedOffTime1 + "</p>";
  html += "<p>Light On Time: " + formattedOnTime2 + "</p>";
  html += "<p>Light Off Time: " + formattedOffTime2 + "</p>";
  html += "</div>";
  // Set new times and angle
  html += "<div><h2>Set New Times and Angle:</h2>";
  html += "<form action='/setTimes' method='POST'>";
  html += "<label for='onTime1'>Valve On Time (HH:MM:SS):</label>";
  html += "<input type='time' id='onTime1' name='onTime1' step='1' value='" + formattedOnTime1 + "' required><br>";
  html += "<label for='offTime1'>Valve Off Time (HH:MM:SS):</label>";
  html += "<input type='time' id='offTime1' name='offTime1' step='1' value='" + formattedOffTime1 + "' required><br>";
  html += "<label for='onTime2'>Light On Time (HH:MM:SS):</label>";
  html += "<input type='time' id='onTime2' name='onTime2' step='1' value='" + formattedOnTime2 + "' required><br>";
  html += "<label for='offTime2'>Light Off Time (HH:MM:SS):</label>";
  html += "<input type='time' id='offTime2' name='offTime2' step='1' value='" + formattedOffTime2 + "' required><br>";
  html += "<label for='angle'>Angle (in degrees):</label>";
  html += "<input type='number' id='angle' name='angle' min='0' max='90' step='1' value='90' required><br>";
  html += "<input type='submit' value='Set Times and Angle'>";
  html += "</form>";
  html += "</div>";
  html += "<p style='float: right;'>Current Time: <span id='currentTime'></span></p>";
  html += "<p id='valveStatus'>Valve: Loading...</p>";
  html += "<p id='lightStatus'>Light: Loading...</p>";
  html += "</div>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}


String formatTime(unsigned long timeInSeconds) {
  int hours = timeInSeconds / 3600;
  int minutes = (timeInSeconds % 3600) / 60;
  int seconds = timeInSeconds % 60;

  return (hours < 10 ? "0" : "") + String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
}


void handleOn1() {
  digitalWrite(ledPin1, HIGH);
  ledState1 = HIGH;
  server.send(200, "text/plain", "LED 1 turned on");
}

void handleOff1() {
  digitalWrite(ledPin1, LOW);
  ledState1 = LOW;
  server.send(200, "text/plain", "LED 1 turned off");
}

void handleOn2() {
  digitalWrite(Light, HIGH);
  ledState2 = HIGH;
  server.send(200, "text/plain", "LED 2 turned on");
}

void handleOff2() {
  digitalWrite(Light, LOW);
  ledState2 = LOW;
  server.send(200, "text/plain", "LED 2 turned off");
}

void handleSetTimes() {
  if (server.hasArg("onTime1") && server.hasArg("offTime1") && server.hasArg("onTime2") && server.hasArg("offTime2") && server.arg("angle")) {
    String onTimeStr1 = server.arg("onTime1");
    String offTimeStr1 = server.arg("offTime1");
    String onTimeStr2 = server.arg("onTime2");
    String offTimeStr2 = server.arg("offTime2");
    String angleStr = server.arg("angle");
    int angle = angleStr.toInt();
    Serial.println(onTimeStr1);
    Serial.println(offTimeStr1);
    Serial.println(onTimeStr2);
    Serial.println(offTimeStr2);
    Serial.println(angle);

    OnOffTime = map(angle, 0, 90, 0, 1600);
    // Parse on and off times for LED 1 (HH:MM:SS format)
    int onHour1 = onTimeStr1.substring(0, 2).toInt();
    int onMinute1 = onTimeStr1.substring(3, 5).toInt();
    int onSecond1 = onTimeStr1.substring(6, 8).toInt();
    int offHour1 = offTimeStr1.substring(0, 2).toInt();
    int offMinute1 = offTimeStr1.substring(3, 5).toInt();
    int offSecond1 = offTimeStr1.substring(6, 8).toInt();

    // Parse on and off times for LED 2 (HH:MM:SS format)
    int onHour2 = onTimeStr2.substring(0, 2).toInt();
    int onMinute2 = onTimeStr2.substring(3, 5).toInt();
    int onSecond2 = onTimeStr2.substring(6, 8).toInt();
    int offHour2 = offTimeStr2.substring(0, 2).toInt();
    int offMinute2 = offTimeStr2.substring(3, 5).toInt();
    int offSecond2 = offTimeStr2.substring(6, 8).toInt();

    // Convert on and off times to seconds since midnight for LED 1
    onTime1 = onHour1 * 3600 + onMinute1 * 60 + onSecond1;
    offTime1 = offHour1 * 3600 + offMinute1 * 60 + offSecond1;

    // Convert on and off times to seconds since midnight for LED 2
    onTime2 = onHour2 * 3600 + onMinute2 * 60 + onSecond2;
    offTime2 = offHour2 * 3600 + offMinute2 * 60 + offSecond2;

    // Print parsed times for debugging
    Serial.print("LED 1 On Time: ");
    Serial.println(onTime1);
    Serial.print("LED 1 Off Time: ");
    Serial.println(offTime1);
    Serial.print("LED 2 On Time: ");
    Serial.println(onTime2);
    Serial.print("LED 2 Off Time: ");
    Serial.println(offTime2);

    String response = "Times set successfully.<script>setTimeout(function(){ window.location='/'; }, 2000);</script>";
    server.send(200, "text/html", response);
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}


void configureWiFi() {
  Serial.println();
  Serial.println("WiFi not configured. Please enter the SSID and password.");

  char ssid1[EEPROM_SIZE];
  char password1[EEPROM_SIZE];

  Serial.print("Please Enter Your SSID: ");
  while (!Serial.available()) {
    // Wait for user input
  }
  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toCharArray(ssid1, EEPROM_SIZE);

  Serial.print("Please Enter Your Password: ");
  while (!Serial.available()) {
    // Wait for user input
  }
  input = Serial.readStringUntil('\n');
  input.trim();
  input.toCharArray(password1, EEPROM_SIZE);

  Serial.println();
  Serial.print("Saving SSID and password to EEPROM...");
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  writeWordsToEEPROM(0, ssid1, password1);
  Serial.println("done");

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid1);

  WiFi.begin(ssid1, password1);
  int connectionTimeout = 10;  // Timeout in seconds
  while (WiFi.status() != WL_CONNECTED && connectionTimeout > 0) {
    delay(1000);
    Serial.print(".");
    connectionTimeout--;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed. Please retry configuration.");
    configureWiFi();
  }
}