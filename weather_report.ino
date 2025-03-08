#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <esp_task_wdt.h> //TWDT Task watchdog timer
#include <math.h>

#define WDT_TIMEOUT 5

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define MQTT_SERVER "MQTT IP" // Azure
#define MQTT_PORT //MQTT PORT
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_NAME "MQTT_NAME"

#define WIFI_NAME "WIFI"
#define WIFI_PASS "PASSWORD"

#define DHTPIN 23
#define DHTTYPE DHT22
#define LED_BUILTIN 5
#define LED_RED 19
#define LED_GREEN 18
#define SW 17
#define buzzer 16

WiFiClient client;
PubSubClient mqtt(client);

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

unsigned long lasttime1000ms = 0;
unsigned long lasttime5000ms = 0;
unsigned long lasttime9000ms = 0;
char buffString[100];
char tempchar[50];
char buzzerMuteChar[50] = "Unmuted";
char buzzerOnOffChar[50] = "OFF";
char weatherStatus[50] = "Normal";
char OLEDStatus[50] = "Disconnected";
char DHT22Status[50] = "Disconnected";
char BMP280Status[50] = "Disconnected";
bool muteBuzzerStatus = false;
bool buzzerOn = true;
bool LED_RED_STATE = true;
bool LED_GREEN_STATE = true;
int displayMode = 0;

int countBuzzer = 0;

//DHT22
float h = 0;
float t = 0;
float f = 0;
int hint = 0;
float hif = 0;
float hic = 0;

//BMP280
float tempBmp = 0;
float pressureBmp1 = 0;
float pressureBmp2 = 0;
float pressureOnetime = 0;
float altitudeBmp = 0;
float P_change = 0;

//function
float DewPoint = 0;
float rainProbability = 0;

float tempAvg = 0;

int countBewareSend = 0;

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "/buzzerMute") {
    if(messageTemp == "toogle"){
      Serial.println("toogle");
      muteBuzzerStatus = !muteBuzzerStatus;
      if(muteBuzzerStatus){
        Serial.println("Buzzer is Muted!");
        sprintf(buzzerMuteChar, "Muted");
        mqtt.publish("/buzzer/MuteMode", buzzerMuteChar);
      }else{
        Serial.println("Buzzer is Unmuted!");
        sprintf(buzzerMuteChar, "Unmuted");
        mqtt.publish("/buzzer/MuteMode", buzzerMuteChar);
      }
    }
  }

  if (String(topic) == "/buzzerToogle") {
    if(messageTemp == "toogle"){
      Serial.println("toogle");
      buzzerOn = !buzzerOn;
      if(buzzerOn){
        Serial.println("Buzzer is OFF");
        sprintf(buzzerOnOffChar, "OFF");
        mqtt.publish("/buzzer/Status", buzzerOnOffChar);
      }else{
        Serial.println("Buzzer os ON");
        sprintf(buzzerOnOffChar, "ON");
        mqtt.publish("/buzzer/Status", buzzerOnOffChar);
      }
    }
  }

  if (String(topic) == "/display") {
    if(messageTemp == "toogleMode"){
      Serial.println("toogleMode");
      displayMode+=1;
      if(displayMode == 3){
        displayMode = 0;
      }
      sprintf(tempchar,"%d", displayMode);
      mqtt.publish("/display/mode", tempchar);
      Serial.print("Display Showing mode ");
      Serial.println(displayMode);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Wire.begin();

  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL); //add current thread to WDT watch

  Serial.print("Connecting to ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  esp_task_wdt_reset();

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);

  sprintf(tempchar,"Disconnected");
  mqtt.publish("/BMP280/connect_status", tempchar);
  sprintf(tempchar,"Disconnected");
  mqtt.publish("/DHT22/connect_status", tempchar);
  sprintf(tempchar,"Disconnected");
  mqtt.publish("/OLED/connect_status", tempchar);

  // sprintf(tempchar,"Connected");
  // mqtt.publish("/WIFI/connect_status", tempchar);

  // sprintf(tempchar,"Connected");
  // mqtt.publish("/MQTT/connect_status", tempchar);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
    Serial.println(F("SSD1306 allocation failed"));
    sprintf(tempchar,"Disconnected");
    mqtt.publish("/OLED/connect_status", tempchar);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  sprintf(tempchar,"Connected");
  sprintf(OLEDStatus, tempchar);
  mqtt.publish("/OLED/connect_status", tempchar);

  pinMode(SW, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  pinMode(buzzer, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  bmp.begin(0x76);

  if (!bmp.begin(0x76)) {  
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    sprintf(tempchar,"Disconnected");
    sprintf(BMP280Status, tempchar);
    mqtt.publish("/BMP280/connect_status", tempchar);
  }else{
    Serial.println("BMP280 : Connected");
    sprintf(tempchar,"Connected");
    sprintf(BMP280Status, tempchar);
    mqtt.publish("/BMP280/connect_status", tempchar);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  dht.begin();

  digitalWrite(buzzer, true);
  pressureOnetime = (bmp.readPressure() / 100.0F) + 3; //h = hecto 10^2

  mqtt.publish("/buzzer/MuteMode", "Unmuted");
  mqtt.publish("/buzzer/Status", "OFF");
  mqtt.publish("/display/mode", "0");
}

void showDisplay(int mode){
  if(mode == 0){

    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    sprintf(buffString, "%s", weatherStatus);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 20);
    sprintf(buffString, "Rain Chance: %.2f %%", rainProbability);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 30);
    sprintf(buffString, "Temperature: %.2f *C", tempAvg);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 40);
    sprintf(buffString, "Real feel: %.2f *C", hic);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 50);
    sprintf(buffString, "Dew Point: %.2f", DewPoint);
    display.print(buffString);

    display.display();
  }else if(mode == 1){
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    sprintf(buffString, "HUMIDITY: %.2f %%", h);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 20);
    sprintf(buffString, "Temperature");
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 30);
    sprintf(buffString, "%.2f *C, %.2f *F", t, f);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 45);
    sprintf(buffString, "Heat index");
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 55);
    sprintf(buffString,  "%.2f *C %.2f *F", hic, hif);
    display.print(buffString);

    display.display();
  }else if(mode == 2){
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    sprintf(buffString, "Buzzer Mode: %s", buzzerMuteChar);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 12);
    sprintf(buffString, "Buzzer Status: %s", buzzerOnOffChar);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 24);
    sprintf(buffString, "OLED: %s", OLEDStatus);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 36);
    sprintf(buffString, "DHT22: %s", DHT22Status);
    display.print(buffString);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 48);
    sprintf(buffString, "BMP280: %s", BMP280Status);
    display.print(buffString);

    display.display();
  }
}

float calculateDewPoint(float T, float RH) {
    float a = 17.27;
    float b = 237.7; // °C
    float alpha = ((a * T) / (b + T)) + log(RH / 100.0);
    float dewPoint = (b * alpha) / (a - alpha);

    DewPoint = dewPoint;

    //Send Average Temperature Data
    sprintf(tempchar,"%.2f", dewPoint);
    mqtt.publish("/Calculate/DewPoint", tempchar);
    Serial.print("DewPoint : ");
    Serial.println(dewPoint);
    return dewPoint;
}

// ฟังก์ชันหลักสำหรับคำนวณโอกาสฝนตก
float calculateRainProbability(float T, float RH, float P_change) {
    // คำนวณจุดน้ำค้าง
    float Td = calculateDewPoint(T, RH);
    // คำนวณความแตกต่างระหว่างอุณหภูมิและจุดน้ำค้าง
    float delta_T = T - Td;

    // ระบบคะแนน
    int score = 0;

    // คะแนนจากความชื้นสัมพัทธ์
    if (RH > 80) {
        score += 30;
    } else if (RH >= 60 && RH <= 80) {
        score += 20;
    } else {
        score += 10;
    }

    // คะแนนจากความแตกต่างระหว่างอุณหภูมิและจุดน้ำค้าง
    if (delta_T < 2) {
        score += 30;
    } else if (delta_T >= 2 && delta_T <= 5) {
        score += 20;
    } else {
        score += 10;
    }

    // คะแนนจากการเปลี่ยนแปลงของความดันบรรยากาศ
    if (P_change < -2) {
        score += 30;
    } else if (P_change >= -2 && P_change <= 0) {
        score += 20;
    } else {
        score += 10;
    }

    // คำนวณโอกาสฝนตกเป็นเปอร์เซ็นต์
    float rainProbability = ((float)score / 90.0) * 100.0;

    return rainProbability;
}

void loop() {
  if(millis()-lasttime1000ms >= 1000){
    lasttime1000ms = millis();

    display.clearDisplay();

    //display send connnection status
    Wire.beginTransmission(0x3c);
    if (Wire.endTransmission() == 0) {
      sprintf(tempchar,"Connected");
      sprintf(OLEDStatus, tempchar);
      mqtt.publish("/OLED/connect_status", tempchar);
    } else {
      sprintf(tempchar,"Disconnected");
      sprintf(OLEDStatus, tempchar);
      mqtt.publish("/OLED/connect_status", tempchar);
    }

    //bmp280 send connection status
    if (!bmp.begin(0x76)) {  
      sprintf(tempchar,"Disconnected");
      sprintf(BMP280Status, tempchar);
      mqtt.publish("/BMP280/connect_status", tempchar);
    }else{
      sprintf(tempchar,"Connected");
      sprintf(BMP280Status, tempchar);
      mqtt.publish("/BMP280/connect_status", tempchar);
    }

    //data from DHT22
    h = dht.readHumidity();
    t = dht.readTemperature();
    f = dht.readTemperature(true);

    hint = dht.readHumidity();

    //dht22 send connection status
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      sprintf(tempchar,"Disconnected");
      sprintf(DHT22Status, tempchar);
      mqtt.publish("/DHT22/connect_status", tempchar);
      return;
    }else{
      sprintf(tempchar,"Connected");
      sprintf(DHT22Status, tempchar);
      mqtt.publish("/DHT22/connect_status", tempchar);
    }

    hif = dht.computeHeatIndex(f, h);
    hic = dht.computeHeatIndex(t, h, false);

    tempBmp = bmp.readTemperature();
    pressureBmp1 = bmp.readPressure() / 100.0F;
    //float cal = pressureBmp1/100.0;
    altitudeBmp = bmp.readAltitude(pressureOnetime);

    if(millis()-lasttime5000ms >= 5000){
      lasttime5000ms = millis();
      pressureBmp2 = bmp.readPressure() / 100.0F;
    }

    if(millis()-lasttime9000ms >= 9000){
      lasttime9000ms = millis();
      P_change = pressureBmp1 - pressureBmp2;
    }

    //Send DHT22 Data
    sprintf(tempchar,"%.2f", t);
    mqtt.publish("/DHT22/Temperature", tempchar);
    sprintf(tempchar,"%.2f", h);
    mqtt.publish("/DHT22/Humidity", tempchar);

    //Send BMP280 Data
    sprintf(tempchar,"%.2f", tempBmp);
    mqtt.publish("/BMP280/Temperature", tempchar);
    sprintf(tempchar,"%.2f", pressureBmp1);
    mqtt.publish("/BMP280/Pressure", tempchar);
    sprintf(tempchar,"%.2f", altitudeBmp);
    mqtt.publish("/BMP280/Altitude", tempchar);


    //buzzer
    if ((rainProbability >= 70.0) || (h >= 80.0) || (hic >= 40.0) || (P_change <= -3.0)) {
      if(!muteBuzzerStatus){
        if(countBuzzer < 10){
          buzzerOn = !buzzerOn;
          digitalWrite(buzzer, buzzerOn);
          countBuzzer+=1;
        }else{
          digitalWrite(buzzer, HIGH);
          buzzerOn = true;
        }
      }
      
      digitalWrite(LED_RED, LED_RED_STATE);
      LED_RED_STATE = !LED_RED_STATE;
      digitalWrite(LED_GREEN, LOW);
      sprintf(weatherStatus, "BEWARE!!");
      if(countBewareSend == 0){
        mqtt.publish("/Weather/Status", weatherStatus);
        countBewareSend++;
      }
    } else {
      
      countBewareSend = 0;
      digitalWrite(LED_GREEN, LED_GREEN_STATE);
      LED_GREEN_STATE = !LED_GREEN_STATE;
      digitalWrite(LED_RED, LOW);
      countBuzzer = 0;
      sprintf(weatherStatus, "NORMAL");
      mqtt.publish("/Weather/Status", weatherStatus);
    }

    //buzzer control
    if(!muteBuzzerStatus){
      digitalWrite(buzzer, buzzerOn);
    }else{
      digitalWrite(buzzer, HIGH);
    }

    //Switch Do
    if(digitalRead(SW) == LOW){
      displayMode+=1;
      if(displayMode == 3){
        displayMode = 0;
      }
      sprintf(tempchar,"%d", displayMode);
      mqtt.publish("/display/mode", tempchar);
      Serial.print("Display Showing mode ");
      Serial.println(displayMode);
    }

    //Show Display From MODE
    showDisplay(displayMode);

    //Serial Monitor Show
    Serial.println("DHT22 Sensor");
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" % ");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t");
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print(hif);
    Serial.println(" *F");

    sprintf(tempchar,"%.2f", hic);
    mqtt.publish("/Temperature/HeatIndex", tempchar);

    Serial.println();
    Serial.println("BMP280 Sensor");
    Serial.print(F("Temperature = "));
    Serial.print(tempBmp);
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    Serial.print(pressureBmp1);
    Serial.println(" hPa");

    Serial.print(F("Approx altitude = "));
    Serial.print(altitudeBmp); /* Adjusted to local forecast! */
    Serial.println(" m");

    //Send Average Temperature Data
    tempAvg = (tempBmp + t) / 2;
    sprintf(tempchar,"%.2f", tempAvg);
    Serial.print("Average Temperature : ");
    Serial.println(tempchar);
    mqtt.publish("/Temperature", tempchar);

    Serial.println();

    rainProbability = calculateRainProbability(tempAvg, h, P_change);
    sprintf(tempchar,"%.2f", rainProbability);
    mqtt.publish("/Calculate/rainProbability", tempchar);
    
    Serial.print("Rain Probability : ");
    Serial.print(rainProbability);
    Serial.println(" %");
    Serial.println();

    esp_task_wdt_reset();

  }

  if(mqtt.connected() == false){
    Serial.print("MQTT connection... ");
    if(mqtt.connect(MQTT_NAME, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqtt.subscribe("/buzzerMute");
      mqtt.subscribe("/buzzerToogle");
      mqtt.subscribe("/display");
    } else {
      Serial.println("failed");
      delay(5000);
    }
  }else{
    mqtt.loop();
  }
}
