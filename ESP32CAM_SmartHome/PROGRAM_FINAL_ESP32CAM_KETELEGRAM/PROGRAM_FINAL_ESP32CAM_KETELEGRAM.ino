#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h> // Tidak perlu WiFiClientSecure dan UniversalTelegramBot di sini lagi

// =======================================================
// ====== KONFIGURASI UMUM (GANTI DENGAN DATA ANDA) ======
// =======================================================

// ====== Konfigurasi WiFi ======
const char* ssid = "my_technology";
const char* password = "35k4nu54marina";

// ====== Konfigurasi MQTT ======
const char* mqtt_broker             = "broker.hivemq.com";
const int   mqtt_port               = 1883;
const char* mqtt_user               = "ronal";
const char* mqtt_password           = "ronaltama12345";
const char* mqtt_client_id          = "ESP32CAM_Client";
const char* mqtt_subscribe_topic    = "esp32cam/trigger";
const char* mqtt_publish_topic_status = "esp32cam/status";

WiFiClient espClient;
PubSubClient client(espClient);

// ====== Konfigurasi Server HTTP untuk Upload (Ganti dengan IP XAMPP Anda) ======
// PASTIKAN IP INI ADALAH IP LOKAL KOMPUTER ANDA YANG MENJALANKAN XAMPP
// Dan ESP32-CAM terhubung ke WiFi yang sama
const char* server_host = "192.168.0.105"; // **PENTING: Ganti dengan Alamat IP komputer XAMPP Anda**
const int server_port = 80;               // Port default Apache di XAMPP
const char* server_path = "/Olivia/upload_telegram.php"; // **PENTING: Ganti dengan path folder Anda di htdocs XAMPP**

// ====== Konfigurasi Kamera (ESP32-CAM AI-Thinker) ======
#define PWDN_GPIO_NUM      32
#define RESET_GPIO_NUM     -1
#define XCLK_GPIO_NUM       0
#define SIOD_GPIO_NUM      26
#define SIOC_GPIO_NUM      27
#define Y9_GPIO_NUM        35
#define Y8_GPIO_NUM        34
#define Y7_GPIO_NUM        39
#define Y6_GPIO_NUM        36
#define Y5_GPIO_NUM        21
#define Y4_GPIO_NUM        19
#define Y3_GPIO_NUM        18
#define Y2_GPIO_NUM         5
#define VSYNC_GPIO_NUM     25
#define HREF_GPIO_NUM      23
#define PCLK_GPIO_NUM      22

// =======================================================
// ====== PROTOTIPE FUNGSI ======
// =======================================================
void connectToWiFi();
void connectToMqtt();
void callback(char* topic, byte* payload, unsigned int length);
void setupCamera();
void takeAndUploadPhotoToWebServer(); // Fungsi baru untuk upload ke server

// =======================================================
// ====== SETUP() ======
// =======================================================
void setup() {
  Serial.begin(115200);
  Serial.println("Booting ESP32-CAM...");

  setupCamera();
  connectToWiFi();

  // MQTT
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  Serial.println("Setup Complete.");
}

// =======================================================
// ====== LOOP() ======
// =======================================================
void loop() {
  if (!client.connected()) {
    connectToMqtt();
  }
  client.loop();
  delay(10);
}

// =======================================================
// ====== CONNECT WIFI ======
// =======================================================
void connectToWiFi() {
  Serial.printf("Connecting to WiFi %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// =======================================================
// ====== CONNECT MQTT ======
// =======================================================
void connectToMqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.publish(mqtt_publish_topic_status, "ESP32-CAM is online!");
      client.subscribe(mqtt_subscribe_topic);
      Serial.printf("Subscribed to: %s\n", mqtt_subscribe_topic);
    } else {
      Serial.printf("failed, rc=%d. Retrying in 5 seconds...\n", client.state());
      delay(5000);
    }
  }
}

// =======================================================
// ====== CALLBACK MQTT ======
// =======================================================
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s]: ", topic);
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);
  if (String(topic) == mqtt_subscribe_topic && msg == "take_photo") {
    Serial.println("Trigger received: taking photo...");
    takeAndUploadPhotoToWebServer(); // Panggil fungsi upload ke server
  }
}

// =======================================================
// ====== SETUP KAMERA ======
// =======================================================
void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Anda bisa kembali ke resolusi yang lebih tinggi jika ingin,
  // karena beban pengiriman ke Telegram ditangani oleh PHP
  config.frame_size   = FRAMESIZE_SVGA; // Atau bahkan FRAMESIZE_XGA jika memori cukup untuk pengambilan
  config.jpeg_quality = 10;             // Kualitas JPEG: 0 (terbaik) hingga 63 (terburuk)
  config.fb_count     = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    Serial.println("Restarting in 5 seconds due to camera init failure...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Camera initialized successfully.");
}

// =======================================================
// ====== AMBIL & UPLOAD FOTO KE WEB SERVER ======
// =======================================================
void takeAndUploadPhotoToWebServer() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    client.publish(mqtt_publish_topic_status, "Capture Failed!");
    return;
  }

  Serial.printf("Picture taken! Size: %lu bytes\n", fb->len);
  client.publish(mqtt_publish_topic_status, "Photo Captured, Uploading to Web Server...");

  WiFiClient client_http;
  if (!client_http.connect(server_host, server_port)) {
    Serial.println("Connection to server failed");
    client.publish(mqtt_publish_topic_status, "Upload Server Conn Failed!");
    esp_camera_fb_return(fb);
    return;
  }

  String head = "--ESP32_CAM_BOUNDARY\r\n";
  head += "Content-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\n";
  head += "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--ESP32_CAM_BOUNDARY--\r\n";

  uint32_t imageLen = fb->len;
  uint32_t totalLen = head.length() + imageLen + tail.length();

  client_http.printf("POST %s HTTP/1.1\r\n", server_path);
  client_http.printf("Host: %s\r\n", server_host);
  client_http.printf("Content-Length: %d\r\n", totalLen);
  client_http.print("Content-Type: multipart/form-data; boundary=ESP32_CAM_BOUNDARY\r\n\r\n");
  client_http.print(head);

  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  for (size_t n = 0; n < fbLen; n = client_http.write(fbBuf, fbLen - n)) {
    // Loop ini memastikan semua data terkirim
  }
  client_http.print(tail);

  esp_camera_fb_return(fb);

  Serial.println("Photo uploaded to web server.");
  client.publish(mqtt_publish_topic_status, "Photo Uploaded to Web Server!");

  // Baca respons dari server PHP (misalnya "OK" atau "Failed")
  String serverResponse = "";
  while (client_http.connected()) {
    String line = client_http.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String body = client_http.readString();
  Serial.print("Server Response: ");
  Serial.println(body);

  if (body.indexOf("SUCCESS") != -1) { // Periksa jika server PHP mengembalikan "SUCCESS"
    Serial.println("Photo successfully sent to Telegram by web server!");
    client.publish(mqtt_publish_topic_status, "Photo Sent to Telegram by Server!");
  } else {
    Serial.println("Web server failed to send photo to Telegram.");
    client.publish(mqtt_publish_topic_status, "Telegram Send Failed by Server!");
  }
  
  client_http.stop();
}