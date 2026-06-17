<div align="center">
  <h1>🤖 KRSBI IoT - Robot Mecanum 3-Wheel</h1>
  <p><i>Sistem Kontrol Robot Omnidirectional Berbasis ESP32 & Komunikasi MQTT</i></p>
  
  ![C++](https://img.shields.io/badge/Language-C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
  ![ESP32](https://img.shields.io/badge/Platform-ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
  ![MQTT](https://img.shields.io/badge/Protocol-MQTT-3C5280?style=for-the-badge&logo=mqtt&logoColor=white)
</div>

<br/>

## 📖 Tentang Proyek
Repositori ini berisi *source code* untuk sistem kontrol **Robot Mecanum Roda 3 (Omnidirectional)**. Robot ini dikendalikan secara nirkabel dengan mikrokontroler ESP32, menggunakan protokol komunikasi **MQTT** untuk mengirim dan menerima perintah (*command*) secara *real-time*. 

Proyek ini dirancang khusus untuk memenuhi tugas pada mata kuliah **Pemrograman Robotika (IFB-308)**.

---

## 👥 Tim Pengembang

| NRP | Nama Mahasiswa | 
| :---: | :--- | 
| **15-2023-117** | 🧑‍💻 M Rafly AL Ghifari R | 
| **15-2023-132** | 🧑‍💻 Muhamad Abu Yusuf | 
| **15-2023-167** | 🧑‍💻 Raelqiansyah P D | 
| **15-2023-185** | 🧑‍💻 Richa Romadhoni |
| **15-2023-186** | 👩‍💻 Difie Anggely | 
| **15-2024-213** | 👩‍💻 Disa Putri Agustin | 

---

## ✨ Fitur Utama

- 🏎️ **Kinematik Mecanum Drive** – Algoritma pergerakan 3 roda ke segala arah (Maju, Mundur, Geser Kiri/Kanan, dan Rotasi).
- 🌐 **Teleoperasi MQTT** – Menggunakan *payload* berbasis JSON untuk transmisi kendali robot yang ringan dan responsif.
- ⚽ **Sistem Penendang (Kicker)** – Integrasi perangkat solenoid aktif (melalui relay) untuk mekanisme menendang bola.
- 🛑 **Safety Timeout (Auto-Stop)** – Fitur proteksi yang akan otomatis mematikan semua mesin (motor) dalam `500ms` apabila koneksi dari pengontrol terputus.
- 📡 **Sistem Anti-Tabrak** – Penggunaan *Sensor Ultrasonik* dengan umpan balik berupa sirine/bunyi *Buzzer* ketika mendeteksi halangan dalam jarak kurang dari 10 cm.

---

## 📂 Struktur Direktori

*Source code* lengkap untuk *board* mikrokontroler dapat ditemukan dalam folder `esp32/`:

* 📁 **`esp32/v1/`** : Versi purwarupa (awal) dari sistem ESP32.
* 📁 **`esp32/v2/`** : Pengembangan versi dengan penambahan *debug print* PWM di Serial Monitor untuk kebutuhan fase pengujian (*testing*).
* 📁 **`esp32/v3/`** : Versi kode pembersihan (*clean code*) tanpa spam serial monitor dengan perbaikan *bug* safety auto-stop.
* 📁 **`esp32/v4/`** 🌟 : **Versi Final**. Terdapat pembaharuan logika kinematik yang tepat untuk pergerakan *Omni-wheel* 3 roda (menyelesaikan masalah robot miring saat maju dan memutar saat menggeser) serta penyesuaian kalibrasi kekuatan motor.

---

## 🚀 Cara Instalasi & Penggunaan (Flashing ESP32)

1. Pastikan Anda telah menginstal **Arduino IDE** dengan *board manager* ESP32.
2. Tambahkan ekstensi (*Library*) berikut melalui *Library Manager* di aplikasi Arduino IDE:
   - `PubSubClient` *(oleh Nick O'Leary)*
   - `ArduinoJson` *(oleh Benoit Blanchon)*
3. Buka file `esp32/v4/esp32.ino`.
4. Sesuaikan konfigurasi jaringan (*Network*) Anda pada blok kode berikut:
   ```cpp
   const char* ssid = "NAMA_WIFI_ANDA";
   const char* password = "PASSWORD_WIFI_ANDA";
   ```
5. *Compile* dan lakukan *Upload* program ke dalam papan sirkuit **ESP32 Dev Module**.
6. Gunakan aplikasi/pengontrol eksternal yang mendukung MQTT, hubungkan ke server `broker.hivemq.com:1883`, lalu kirim JSON perintah ke *topic*: **`robot/cmd`**.

---
<div align="center">
  <sub>Dibuat dengan ❤️ oleh Tim Proyek Pemrograman Robotika IFB-308</sub>
</div>
