/*
  ESP32 MQTT-Based Smart Home Miniature for 6 Rooms

  Features:
  1. Terrace:
      - RFID scanning: on valid card, servo activation
      - I2C LCD: displays "Selamat Datang" on valid scan
      - PIR motion sensor active between 01:00-03:00 (configurable via MQTT); triggers alarm on pin 27
      - Light control via MQTT
  2. Living Room:
      - Light control via MQTT
      - Fan control via MQTT
  3. Bathroom:
      - PIR sensor: auto-light ON with delay, via relay
  4. Bedroom 1:
      - Light control via MQTT
  5. Bedroom 2:
      - Light control via MQTT
      - DHT22: temperature & humidity publishing via MQTT on separate topics
  6. Kitchen:
      - Light control via MQTT
      - Gas sensor MQ-15: triggers alarm on pin 27

  MQTT Topics:
    // SET TOPICS (untuk menerima perintah dari broker/aplikasi)
    terrace/light/set
    terrace/pir/enable
    livingroom/light/set
    livingroom/fan/set
    bathroom/light/set // Tambahan untuk kontrol manual jika diperlukan
    bedroom1/light/set
    bedroom2/light/set
    kitchen/light/set

    // STATUS/DATA TOPICS (untuk mengirim data ke broker/aplikasi)
    terrace/light/status
    terrace/rfid
    terrace/alarm
    bathroom/light/status // Tambahan untuk status otomatis
    bedroom2/dht/temperature
    bedroom2/dht/humidity
    kitchen/gas/alarm

  Pin Assignments:
    // I2C Bus
    SDA: 21
    SCL: 22

    // Terrace
    RFID RST: 17 // Pin RST RFID
    RFID SDA (SS/CS): 5 // Pin SS RFID
    SERVO: 18
    PIR_TERRACE: 34
    ALARM: 27
    RELAY_TERRACE_LIGHT: 19

    // Living Room
    RELAY_LR_LIGHT: 23
    RELAY_LR_FAN: 14 // Conflict with RFID SS_PIN resolved

    // Bathroom
    PIR_BATHROOM: 32
    RELAY_BATHLIGHT: 4

    // Bedroom 1
    RELAY_BR1_LIGHT: 2

    // Bedroom 2
    RELAY_BR2_LIGHT: 15
    DHT22_PIN: 33

    // Kitchen
    RELAY_KITCHEN_LIGHT: 13
    MQ15_PIN: 35
    // Alarm pin reused: ALARM (27)

  PIN CONFLICT RESOLVED:
    - RELAY_LR_FAN changed from 5 to 14 to resolve conflict with RFID SS_PIN.

  Program Flow:
    1. Setup WiFi & MQTT client, reconnect logic
    2. Initialize peripherals: RFID, Servo, I2C LCD, PIRs, Relays, DHT, MQ15
    3. Subscribe to control topics for lights, fans, PIR enable
    4. Loop:
        - mqttClient.loop()
        - Check RFID: if valid UID, trigger servo, LCD message, publish terrace/rfid
        - Read time; if between 1-3 AM and terrace PIR enabled, check PIR_TERRACE -> if motion, trigger alarm
        - Check PIR_BATHROOM: if motion, switch bathroom light ON with delay off timer
        - Read DHT22 periodically, publish temperature & humidity
        - Read MQ15: if gas threshold, trigger alarm pin and publish kitchen/gas/alarm
        - Maintain relays based on last MQTT command state

  Required Libraries:
    WiFi.h
    PubSubClient.h
    LiquidCrystal_I2C.h
    ESP32Servo.h // Updated: Using ESP32Servo library
    MFRC522.h
    DHT.h
    time.h (for NTP)
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h> // Menggunakan library ESP32Servo
#include <SPI.h>
#include <MFRC522.h>
#include <DHT.h>
#include <time.h> // Untuk fungsi waktu NTP

// WiFi & MQTT settings
const char* ssid = "my_technology";
const char* password = "35k4nu54marina";
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* mqttUser = "ronal";
const char* mqttPassword ="ronaltama12345";

// Client MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// I2C LCD
// Pastikan alamat I2C LCD Anda benar (0x27 atau 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo (Pintu Teras)
Servo terraceServo; // Menggunakan objek Servo dari ESP32Servo

// RFID
#define SS_PIN 5
#define RST_PIN 17
MFRC522 rfid(SS_PIN, RST_PIN);

// DHT22
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Pin defs
const int pinPIRTerrace = 34;
const int pinPIRBathroom = 32;
const int pinMQ15 = 35; // Analog input
const int pinAlarm = 27; // Buzzer

// Relay Pins (active LOW, so HIGH = OFF, LOW = ON)
const int relayTerraceLight = 15;
const int relayLRLight = 26;
const int relayLRFan = 14; // CONFLICT RESOLVED: Changed from 5 to 14
const int relayBathLight = 16;
const int relayBR1Light = 15;
const int relayBR2Light = 25;
const int relayKitchenLight = 13;
const int servoPin = 12; // Pin untuk servo

// RFID access control state
// PENTING: UID kartu master Anda yang sebenarnya (D7 7C 37 03)
byte masterCardUID[] = {0xD7, 0x7C, 0x37, 0x03}; // Contoh UID
int rfidFailedAttempts = 0;
const int MAX_RFID_FAILED_ATTEMPTS = 3;
unsigned long alarmStartTime = 0;
const long ALARM_DURATION = 5000; // Durasi alarm dalam milidetik (5 detik)

// Global state variables for MQTT controlled devices
bool terraceLightState = false;
bool lrLightState = false;
bool lrFanState = false;
bool bathLightState = false; // Initial state for bathroom light (can be auto/manual)
bool br1LightState = false;
bool br2LightState = false;
bool kitchenLightState = false;
bool terracePIREnabled = false; // Default: PIR alarm disabled at startup
bool terracePIRAlarmActive = false; // <<< TAMBAHKAN BARIS INI

// Timing variables for sensors
unsigned long lastDHTReadMillis = 0;
const long DHT_READ_INTERVAL = 15000; // Baca DHT dan kirim data setiap 60 detik (1 menit)

unsigned long lastPirBathDetectMillis = 0;
const long BATHROOM_LIGHT_OFF_DELAY = 15000; // Lampu kamar mandi mati setelah 60 detik tidak ada gerakan

// --- Function Prototypes ---
void setup_wifi();
void reconnect_mqtt();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void activateAlarm(long duration);

void setup() {
  Serial.begin(115200);

  // Initialize pins
  pinMode(pinPIRTerrace, INPUT);
  pinMode(pinPIRBathroom, INPUT);
  pinMode(pinMQ15, INPUT); // Analog input
  pinMode(pinAlarm, OUTPUT); // Buzzer

  // Set all relays OFF at startup (assuming active LOW relays: HIGH = OFF, LOW = ON)
  pinMode(relayTerraceLight, OUTPUT);
  digitalWrite(relayTerraceLight, HIGH);
  pinMode(relayLRLight, OUTPUT);
  digitalWrite(relayLRLight, HIGH);
  pinMode(relayLRFan, OUTPUT);
  digitalWrite(relayLRFan, HIGH);
  pinMode(relayBathLight, OUTPUT);
  digitalWrite(relayBathLight, HIGH);
  pinMode(relayBR1Light, OUTPUT);
  digitalWrite(relayBR1Light, HIGH);
  pinMode(relayBR2Light, OUTPUT);
  digitalWrite(relayBR2Light, HIGH);
  pinMode(relayKitchenLight, OUTPUT);
  digitalWrite(relayKitchenLight, HIGH);
  digitalWrite(pinAlarm, LOW); // Pastikan alarm buzzer mati

  // Inisialisasi Servo
  // Tidak perlu ESP32PWM::attachAll() jika hanya menggunakan satu atau beberapa servo
  // Cukup panggil attach pada objek servo
  terraceServo.attach(servoPin, 500, 2500); // Attach servo ke pin, dengan min/max pulse width (mikrodetik)
  terraceServo.write(0); // Pastikan pintu tertutup saat startup

  // Setup I2C LCD
  Wire.begin(21, 22); // Initialize I2C communication for LCD (SDA, SCL)
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SmartHome Init...");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi...");

  // RFID
  SPI.begin(18, 19, 23, SS_PIN); // SCK, MISO, MOSI, SS
  rfid.PCD_Init();
  Serial.println("RFID initialized.");

  // DHT22
  dht.begin();

  // Connect WiFi
  setup_wifi();

  // Initialize MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  // NTP time (Jakarta time zone)
  // Anda bisa menyesuaikan "Asia/Jakarta" jika lokasi Anda berbeda
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time synchronization...");
  time_t now = 0;
  int retry_count = 0;
  while (now < 1672531200 && retry_count < 10) { // Check if time is after Jan 1, 2023
    delay(500);
    now = time(nullptr);
    retry_count++;
  }
  if (now > 1672531200) {
    Serial.println("NTP time synchronized.");
  } else {
    Serial.println("NTP time synchronization failed.");
  }

  // Initial LCD display after setup
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SmartHome Ready!");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Pastikan koneksi MQTT tetap terjaga
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop(); // Memproses pesan masuk dan mempertahankan koneksi

  // --- RFID handling ---
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.print("UID Tag :");
    String currentCardUID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(rfid.uid.uidByte[i], HEX);
      currentCardUID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      currentCardUID += String(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();
    currentCardUID.toUpperCase();

    // Compare scanned UID with the master card UID
    bool uidMatch = true;
    if (rfid.uid.size != sizeof(masterCardUID)) {
      uidMatch = false;
    } else {
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] != masterCardUID[i]) {
          uidMatch = false;
          break;
        }
      }
    }

    if (uidMatch) {
      Serial.println("Access Granted!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SELAMAT DATANG!");
      lcd.setCursor(0, 1);
      lcd.print("Pintu Terbuka");
      terraceServo.write(90); // Open door
      client.publish("terrace/rfid", "ACCESS_GRANTED"); // Publish success
      rfidFailedAttempts = 0; // Reset failed attempts
      delay(3000); // Keep door open for 3 seconds
      terraceServo.write(0); // Close door
      lcd.clear();
    } else {
      Serial.println("Access Denied!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("COBA LAGI!");
      rfidFailedAttempts++;
      client.publish("terrace/rfid", "ACCESS_DENIED"); // Publish failure
      if (rfidFailedAttempts >= MAX_RFID_FAILED_ATTEMPTS) {
        Serial.println("Too many failed attempts! Activating alarm.");
        lcd.setCursor(0, 1);
        lcd.print("ALARM AKTIF!");
        activateAlarm(ALARM_DURATION);
        rfidFailedAttempts = 0; // Reset after alarm activation
      }
      delay(2000); // Display "COBA LAGI" for 2 seconds
      lcd.clear();
    }
    rfid.PICC_HaltA(); // Halt PICC
    rfid.PCD_StopCrypto1(); // Stop encryption
  }

  // // --- Terrace PIR alarm (time-based) ---
  // struct tm timeinfo;
  // if (getLocalTime(&timeinfo)) {
  //   int currentHour = timeinfo.tm_hour;
  //   // Alarm aktif antara jam 01:00 dan 02:59
  //   if (terracePIREnabled && currentHour >= 1 && currentHour < 3 && digitalRead(pinPIRTerrace) == HIGH) {
  //     Serial.println("PIR Terrace ALARM TRIGGERED!");
  //     lcd.clear();
  //     lcd.setCursor(0, 0);
  //     lcd.print("PIR Teras ALARM!");
  //     activateAlarm(ALARM_DURATION);
  //     client.publish("terrace/alarm", "MOTION_DETECTED");
  //   } else {
  //     // Pastikan alarm mati jika di luar waktu atau tidak ada gerakan
  //     if (alarmStartTime == 0) { // Hanya matikan buzzer jika tidak ada alarm lain yang aktif
  //       digitalWrite(pinAlarm, LOW);
  //     }
  //   }
  // }



// --- Terrace PIR alarm (dengan durasi 1 menit) ---
  // Jika PIR teras diaktifkan dan mendeteksi gerakan, atau jika alarm PIR teras sedang aktif
  if (terracePIREnabled && digitalRead(pinPIRTerrace) == HIGH) {
    if (!terracePIRAlarmActive) { // Hanya picu sekali saat pertama terdeteksi
      Serial.println("PIR Terrace ALARM TRIGGERED! Activating for 1 minute.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PIR Teras ALARM!");
      activateAlarm(ALARM_DURATION); // Aktifkan alarm
      client.publish("terrace/alarm", "1");
      client.publish("esp32cam/trigger", "take_photo");
      terracePIRAlarmActive = true; // Set status alarm PIR teras menjadi aktif
    }
  }

  // Cek apakah alarm PIR teras harus dimatikan setelah durasi
  if (terracePIRAlarmActive && (millis() - alarmStartTime >= ALARM_DURATION)) {
    Serial.println("PIR Terrace ALARM DURATION ENDED. Deactivating.");
    digitalWrite(pinAlarm, LOW); // Matikan buzzer
    alarmStartTime = 0;          // Reset timer alarm global
    terracePIRAlarmActive = false; // Set status alarm PIR teras menjadi non-aktif
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SmartHome Ready!"); // Kembali ke tampilan normal
    client.publish("terrace/alarm", "0"); // Publikasikan status alarm mati
  }
  // --- Bathroom PIR auto-light (with delay off) ---
  if (digitalRead(pinPIRBathroom) == HIGH) {
    digitalWrite(relayBathLight, LOW); // Turn ON (Active LOW)
    bathLightState = true;
    lastPirBathDetectMillis = millis();
    client.publish("bathroom/light/status", "1");
    Serial.println("Motion detected in Bathroom. Light ON.");
  } else {
    // Matikan lampu jika tidak ada gerakan setelah waktu tunda
    if (bathLightState && millis() - lastPirBathDetectMillis > BATHROOM_LIGHT_OFF_DELAY) {
      digitalWrite(relayBathLight, HIGH); // Turn OFF (Active LOW)
      bathLightState = false;
      client.publish("bathroom/light/status", "0");
      Serial.println("No motion in Bathroom. Light OFF.");
    }
  }

  // --- DHT22 publish periodically ---
  if (millis() - lastDHTReadMillis >= DHT_READ_INTERVAL) {
    int t = dht.readTemperature();
    int h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      client.publish("bedroom2/dht/temperature", String(t).c_str());
      client.publish("bedroom2/dht/humidity", String(h).c_str());
      Serial.print("Published Temp: "); Serial.print(t); Serial.print("C, Hum: "); Serial.print(h); Serial.println("%");
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
    lastDHTReadMillis = millis();
  }

  // --- MQ15 gas alarm ---
  int gasValue = analogRead(pinMQ15);
  // Serial.print("Gas Sensor Value: "); Serial.println(gasValue); // Untuk debugging
  if (gasValue > 3000) { // Threshold for gas detection (adjust as per sensor calibration)
    Serial.println("Gas Leak Detected! Activating alarm.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("GAS BOCOR!");
    lcd.setCursor(0, 1);
    lcd.print("ALARM AKTIF!");
    activateAlarm(ALARM_DURATION);
    client.publish("kitchen/gas/alarm", "1");
  }

  // --- Manage global alarm activation and duration ---
  if (alarmStartTime > 0 && millis() - alarmStartTime < ALARM_DURATION) {
    digitalWrite(pinAlarm, HIGH); // Nyalakan buzzer
    lcd.setCursor(0, 0); // Pastikan LCD menampilkan alarm jika masih aktif
    lcd.print("!!! ALARM ON !!!");
  } else if (alarmStartTime > 0 && millis() - alarmStartTime >= ALARM_DURATION) {
    digitalWrite(pinAlarm, LOW); // Matikan buzzer
    alarmStartTime = 0; // Reset timer alarm
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SmartHome Ready!"); // Kembali ke tampilan normal
  }
  delay(10); // Kecilkan delay agar loop lebih responsif
}

// --- WiFi Setup ---
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    lcd.setCursor(0, 1);
    lcd.print("IP: ");
    lcd.print(WiFi.localIP());
    delay(2000); // Tampilkan IP sebentar
  } else {
    Serial.println("");
    Serial.println("WiFi connection FAILED. Retrying...");
    lcd.setCursor(0, 1);
    lcd.print("WiFi FAILED!");
    delay(5000); // Wait before retrying
    ESP.restart(); // Restart ESP32 to attempt reconnection
  }
}

// --- MQTT Reconnect ---
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect with provided username and password
    if (client.connect("ESP32SmartHomeClient", mqttUser, mqttPassword)) { // Beri ID client yang unik
      Serial.println("connected");
      // --- SUBSCRIBE KE SEMUA TOPIC KONTROL DI SINI ---
      client.subscribe("terrace/light/set");
      client.subscribe("terrace/pir/enable");
      client.subscribe("livingroom/light/set");
      client.subscribe("livingroom/fan/set");
      client.subscribe("bathroom/light/set"); // Jika ingin kontrol manual untuk bathroom
      client.subscribe("bedroom1/light/set");
      client.subscribe("bedroom2/light/set");
      client.subscribe("kitchen/light/set");

      Serial.println("Subscribed to control topics.");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

// --- MQTT Callback for incoming messages ---
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msgString;
  for (int i = 0; i < length; i++) {
    msgString += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msgString);

  // Control for Terrace Light
  if (String(topic) == "terrace/light/set") {
    terraceLightState = (msgString == "ON");
    digitalWrite(relayTerraceLight, terraceLightState ? LOW : HIGH); // Active LOW
    client.publish("terrace/light/status", terraceLightState ? "ON" : "OFF");
    Serial.print("Terrace Light set to: "); Serial.println(terraceLightState ? "ON" : "OFF");
  }
else if (String(topic) == "terrace/pir/enable") {
  terracePIREnabled = (msgString == "1");
  Serial.print("Terrace PIR Alarm Enabled: ");
  Serial.println(terracePIREnabled ? "1" : "0");

  // Aktifkan atau nonaktifkan alarm SEGERA, berdasarkan pesan MQTT
  if (terracePIREnabled) {
    activateAlarm(ALARM_DURATION); // Nyalakan alarm
  } else {
    digitalWrite(pinAlarm, LOW);      // Matikan alarm
    alarmStartTime = 0;              // Reset timer alarm
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SmartHome Ready!"); // Kembalikan tampilan LCD
  }
}
  // Control for Living Room Light
  else if (String(topic) == "livingroom/light/set") {
    lrLightState = (msgString == "1");
    digitalWrite(relayLRLight, lrLightState ? LOW : HIGH); // Active LOW
    Serial.print("Living Room Light set to: "); Serial.println(lrLightState ? "1" : "0");
  }
  // Control for Living Room Fan
  else if (String(topic) == "livingroom/fan/set") {
    lrFanState = (msgString == "1");
    digitalWrite(relayLRFan, lrFanState ? LOW : HIGH); // Active LOW
    Serial.print("Living Room Fan set to: "); Serial.println(lrFanState ? "1" : "0");
  }
  // Control for Bathroom Light (manual override if needed)
  else if (String(topic) == "bathroom/light/set") {
    bathLightState = (msgString == "1");
    digitalWrite(relayBathLight, bathLightState ? LOW : HIGH); // Active LOW
    // Reset PIR timer if manual control takes over
    if (bathLightState) lastPirBathDetectMillis = millis(); else lastPirBathDetectMillis = 0;
    Serial.print("Bathroom Light set to: "); Serial.println(bathLightState ? "1" : "0");
  }
  // Control for Bedroom 1 Light
  else if (String(topic) == "bedroom1/light/set") {
    br1LightState = (msgString == "1");
    digitalWrite(relayBR1Light, br1LightState ? LOW : HIGH); // Active LOW
    Serial.print("Bedroom 1 Light set to: "); Serial.println(br1LightState ? "1" : "0");
  }
  // Control for Bedroom 2 Light
  else if (String(topic) == "bedroom2/light/set") {
    br2LightState = (msgString == "1");
    digitalWrite(relayBR2Light, br2LightState ? LOW : HIGH); // Active LOW
    Serial.print("Bedroom 2 Light set to: "); Serial.println(br2LightState ? "1" : "0");
  }
  // Control for Kitchen Light
  else if (String(topic) == "kitchen/light/set") {
    kitchenLightState = (msgString == "1");
    digitalWrite(relayKitchenLight, kitchenLightState ? LOW : HIGH); // Active LOW
    Serial.print("Kitchen Light set to: "); Serial.println(kitchenLightState ? "1" : "0");
  }
}

// --- Activate Alarm ---
void activateAlarm(long duration) {
  if (alarmStartTime == 0) { // Only activate if the alarm is not already active
    alarmStartTime = millis();
    digitalWrite(pinAlarm, HIGH); // Turn on buzzer
    Serial.println("Alarm activated.");
  }
}