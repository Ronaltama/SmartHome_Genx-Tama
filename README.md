# SISTEM RUMAH PINTAR "GENX TAMA" - OLIVIA X 2025

Selamat datang di repositori resmi proyek **GENX TAMA** untuk ajang **Olimpiade Vokasi Indonesia (OLIVIA) X 2025**. Proyek ini adalah implementasi sistem rumah pintar berbasis IoT yang dirancang untuk memberikan solusi keamanan dan kenyamanan hunian secara terintegrasi dan multiplatform.

Tim Penyusun:
* Edwin Ronaltama Mabrur (V3424050)
* Bagas Prabowo (V3424042)
* Muhammad Harits Fahrizal (V3424063)
* Muhammad Iqbal Romadhon (V3424065)

---

### 💡 Penjelasan Proyek

Di era digital saat ini, kebutuhan akan hunian yang tidak hanya nyaman tetapi juga aman menjadi prioritas. Banyak solusi *smart home* yang ada di pasaran seringkali mahal atau hanya berfokus pada satu aspek saja. Proyek **GENX TAMA** hadir untuk menjawab tantangan tersebut dengan menyediakan sistem yang **komprehensif, terjangkau, dan mudah diakses**.

Sistem kami mengintegrasikan berbagai sensor dan aktuator yang dikendalikan oleh mikrokontroler ESP32, memungkinkan pengguna memonitor dan mengontrol rumah mereka dari mana saja melalui tiga platform berbeda: **Antarmuka Web, Perintah Suara (Google Assistant), dan Bot Telegram**.



### ✨ Fitur Unggulan

Sistem kami dilengkapi dengan berbagai fitur canggih untuk keamanan dan kenyamanan:

* 🚨 **Keamanan Proaktif:**
    * **Deteksi Gerakan dengan Notifikasi Foto:** Sensor PIR mendeteksi gerakan mencurigakan, lalu ESP32-CAM secara otomatis mengambil gambar dan mengirimkannya langsung ke Telegram pengguna.
    * **Deteksi Kebocoran Gas:** Sensor MQ-4 akan memicu alarm dan mengirim notifikasi jika terdeteksi ada kebocoran gas berbahaya.
    * **Akses Pintu Cerdas:** Kunci pintu elektronik yang dapat dibuka menggunakan kartu RFID terdaftar, memberikan keamanan akses yang lebih baik daripada kunci konvensional.

* 쾌 **Kenyamanan & Kontrol Fleksibel:**
    * **Kontrol Multiplatform:** Kendalikan lampu, kipas angin, dan perangkat lainnya melalui:
        1.  **Antarmuka Web** yang intuitif.
        2.  **Perintah Suara** via Google Assistant ("Ok Google, nyalakan lampu").
        3.  **Bot Telegram** yang responsif.
    * **Monitoring Suhu & Kelembapan:** Sensor DHT22 memantau kondisi ruangan secara *real-time* yang dapat dilihat pada *dashboard*.
    * **Otomatisasi Cerdas:** Lampu dapat menyala otomatis saat gerakan terdeteksi, meningkatkan efisiensi energi.



### 🛠️ Teknologi yang Digunakan

* **Hardware:** ESP32, ESP32-CAM, Sensor PIR, Sensor DHT22, Sensor Gas MQ4, RFID RC522, Modul Relay, Motor Servo SG90, LCD 16x2 I2C.
* **Software & Platform:** Arduino IDE (C++), Node-RED, MQTT Broker, Google Assistant, Telegram Bot API.



### 🎥 Link Video Demo

Lihat bagaimana sistem kami bekerja dalam skenario nyata melalui video demo berikut:

**https://drive.google.com/drive/folders/1Q_nLw3Bq_vhgBjEZ5blEFUXTn-bGCLsG**



### 📄 Link Dokumentasi Teknis PDF

Untuk penjelasan yang lebih mendalam mengenai arsitektur, skema rangkaian, dan detail teknis lainnya, silakan akses dokumen laporan kami:

**https://drive.google.com/drive/folders/1Qc-_Rl24qDguoRrySqFm5lpViQpaLEE7**


