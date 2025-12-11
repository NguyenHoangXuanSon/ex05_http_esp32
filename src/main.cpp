#include <Arduino.h> 
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// Cấu hình các chân
#define DHTPIN 15        
#define DHTTYPE DHT22
#define PIR_PIN 12       
#define LED_PIN 32       

// Khai báo đối tượng
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Setup wifi 
const char* ssid = "Wokwi-GUEST"; 
const char* password = "";

//Server endpoints
const char* server_GET = "https://postman-echo.com/get";
const char* server_POST = "https://postman-echo.com/post";

//Khai báo các hàm gửi dữ liệu
void gui_GET(float t, float h);
void gui_POST_URL(float t, float h);
void gui_POST_JSON(float t, float h, int motion);

void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  dht.begin();
  lcd.init();
  lcd.backlight();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
  
  int tryDelay = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tryDelay++;
    if(tryDelay > 20) { 
        Serial.println("\nWifi Connect Failed!");
        break;
    }
  }
  if(WiFi.status() == WL_CONNECTED){
      Serial.println("\nWiFi Connected!");
      lcd.clear();
  }
}

void loop() {
  // 1. Đọc dữ liệu
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int motion = digitalRead(PIR_PIN);

  if (isnan(h) || isnan(t)) {
    Serial.println("Loi cam bien DHT!");
    delay(2000);
    return;
  }

  // 2. Điều khiển LED & LCD
  if (motion == HIGH) digitalWrite(LED_PIN, HIGH);
  else digitalWrite(LED_PIN, LOW);

  lcd.setCursor(0, 0);
  lcd.print("T:" + String(t, 1) + " H:" + String(h, 0));
  lcd.setCursor(0, 1);
  lcd.print("Mode: Sending...");

  // 3. GỬI DỮ LIỆU
  if (WiFi.status() == WL_CONNECTED) {
    gui_GET(t, h);            
    delay(1000);
    gui_POST_URL(t, h);       
    delay(1000);
    gui_POST_JSON(t, h, motion); 
  } else {
      Serial.println("Mat ket noi WiFi!");

  }

  Serial.println("\n--- Doi 10 giay ---");
  delay(10000); 
}

// Các hàm gửi dữ liệu

void gui_GET(float t, float h) {
  HTTPClient http;
  String path = String(server_GET) + "?temp=" + String(t) + "&humid=" + String(h);
  
  Serial.println("\n[A] GET Request...");
  http.begin(path.c_str());
  
  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[A] OK! Code: %d\n", httpCode);
  } else {
    Serial.printf("[A] Error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void gui_POST_URL(float t, float h) {
  HTTPClient http;
  http.begin(server_POST);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String body = "temp=" + String(t) + "&humid=" + String(h);
  
  Serial.println("\n[B] POST URL-Encoded...");
  int httpCode = http.POST(body);
  
  if (httpCode > 0) {
    Serial.printf("[B] OK! Code: %d\n", httpCode);
  } else {
    Serial.printf("[B] Error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void gui_POST_JSON(float t, float h, int motion) {
  HTTPClient http;
  http.begin(server_POST);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(1024);
  doc["temperature"] = t;
  doc["humidity"] = h;
  doc["motion_detected"] = (motion == HIGH) ? true : false;
  
  String jsonOutput;
  serializeJson(doc, jsonOutput);

  Serial.println("\n[C] POST JSON...");
  int httpCode = http.POST(jsonOutput); 
  
  if (httpCode > 0) {
    Serial.printf("[C] OK! Code: %d\n", httpCode);
    Serial.println("Response: " + http.getString());
  } else {
    Serial.printf("[C] Error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}