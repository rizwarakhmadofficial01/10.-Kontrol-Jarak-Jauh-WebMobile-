# Pertemuan 10 - Kontrol Jarak Jauh (Node-RED + MQTT)

Pemrograman Internet of Things (SIF1502), Prodi Sistem Informasi, UNISKA Banjarmasin.

**Nama:** _isi nama_  |  **NIM:** _isi NIM_  |  **Kelas:** _isi kelas_

Dashboard Node-RED menampilkan suhu (gauge) dan mengontrol LED (switch) serta kecerahan PWM (slider) lewat MQTT.
Perangkat disimulasikan dengan skrip Python (`iot_device.py`); kode ESP32 asli tersedia di `esp32_kontrol/`.

## Arsitektur

```
iot_device.py --publish suhu--> Mosquitto --> Node-RED --> Dashboard (/ui)
iot_device.py <--subscribe led/pwm-- Mosquitto <-- Node-RED <-- switch & slider
iot_device.py --publish status (feedback)--> Mosquitto --> Node-RED (teks status)
```

| Fungsi | Topik | Arah |
|---|---|---|
| Suhu | `iot/kelas/sensor/suhu` | Perangkat -> Node-RED |
| Kontrol LED (ON/OFF) | `iot/kelas/aktuator/led` | Node-RED -> Perangkat |
| Kontrol PWM (0-255) | `iot/kelas/aktuator/pwm` | Node-RED -> Perangkat |
| Status LED / PWM | `iot/kelas/status/led`, `iot/kelas/status/pwm` | Perangkat -> Node-RED |

## Isi repo

```
README.md
iot_device.py              # simulasi perangkat (pengganti ESP32)
requirements.txt
flow-pertemuan10.json      # ekspor flow Node-RED
esp32_kontrol/esp32_kontrol.ino   # kode ESP32 (PubSubClient)
docs/screenshots/          # bukti pengujian
```

## Cara menjalankan

1. Mosquitto: `cd /d D:\Mosquitto` lalu `mosquitto.exe -c mosquitto.conf -v` (conf berisi `listener 1883` dan `allow_anonymous true`).
2. Node-RED: `node-red`, pasang palet `node-red-dashboard` (Manage palette), Import `flow-pertemuan10.json`, Deploy.
3. Perangkat: `pip install -r requirements.txt` lalu `python iot_device.py`.
4. Buka `http://localhost:1880/ui`.

## Hasil pengujian

Isi kolom hasil sesuai pengujian Anda.

| Uji | Hasil |
|---|---|
| Lapisan 1: Mosquitto (`mosquitto_sub` / `mosquitto_pub`) | |
| Lapisan 2: Python publish suhu dan menerima perintah LED | |
| Lapisan 3A: Gauge suhu berubah tiap 5 detik | |
| Lapisan 3B: Switch LED -> Python menerima, status dashboard berubah | |
| Lapisan 3C: Kontrol dari luar dashboard (`mosquitto_pub OFF`) tersinkron | |
| Slider PWM -> Python menerima nilai, status PWM berubah | |
| Ketahanan: restart Python / Mosquitto / perintah `BLAH` | |

## Screenshot

![Mosquitto](docs/screenshots/01-mosquitto.png)
![Python](docs/screenshots/02-python.png)
![Dashboard](docs/screenshots/03-dashboard.png)
![Uji kontrol](docs/screenshots/04-kontrol.png)
![Flow](docs/screenshots/05-flow.png)

## Catatan

Kode `esp32_kontrol.ino` belum diuji pada perangkat fisik. Untuk ESP32 asli, isi SSID, password WiFi, dan IP komputer broker; pada Windows izinkan port 1883 di firewall.
