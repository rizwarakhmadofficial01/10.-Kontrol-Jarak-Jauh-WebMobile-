/*
  Pertemuan 10 - Kontrol Jarak Jauh (ESP32)
  - Publish suhu DHT22 setiap 3 detik
  - Subscribe perintah LED/relay (ON/OFF) dan PWM (0-255)
  - Publish feedback status setelah perintah diterima

  Library (Library Manager): PubSubClient, DHT sensor library, Adafruit Unified Sensor
  Wiring: DHT22 data -> GPIO 4 | LED/relay -> GPIO 26 | LED PWM -> GPIO 2
*/
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ===== KONFIGURASI (sesuaikan) =====
const char* WIFI_SSID   = "NAMA_WIFI";
const char* WIFI_PASS   = "PASSWORD_WIFI";
const char* MQTT_BROKER = "192.168.1.10";   // IP komputer yang menjalankan Mosquitto
const int   MQTT_PORT   = 1883;

#define PREFIX "iot/kelas"                  // untuk tugas: "iot/NIM_ANDA"
#define TOPIC_SUHU        PREFIX "/sensor/suhu"
#define TOPIC_LED         PREFIX "/aktuator/led"
#define TOPIC_PWM         PREFIX "/aktuator/pwm"
#define TOPIC_STATUS_LED  PREFIX "/status/led"
#define TOPIC_STATUS_PWM  PREFIX "/status/pwm"

#define DHT_PIN  4
#define DHT_TYPE DHT22
#define LED_PIN  26   // relay / LED on-off
#define PWM_PIN  2    // LED kecerahan (PWM)

const unsigned long INTERVAL_SENSOR = 3000;  // ms

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastSensor = 0;
unsigned long lastReconnect = 0;

void pwmWrite(int nilai) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PWM_PIN, nilai);
#else
  ledcWrite(0, nilai);
#endif
}

void connectWiFi() {
  Serial.print("Menghubungkan WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi terhubung, IP: ");
  Serial.println(WiFi.localIP());
}

// Dipanggil setiap ada pesan pada topik yang di-subscribe
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  message.trim();
  Serial.printf("Pesan masuk [%s]: %s\n", topic, message.c_str());

  if (String(topic) == TOPIC_LED) {
    if (message == "ON" || message == "1") {
      digitalWrite(LED_PIN, HIGH);
      mqtt.publish(TOPIC_STATUS_LED, "ON");
    } else if (message == "OFF" || message == "0") {
      digitalWrite(LED_PIN, LOW);
      mqtt.publish(TOPIC_STATUS_LED, "OFF");
    } else {
      Serial.println("Perintah tidak dikenal");
    }
  } else if (String(topic) == TOPIC_PWM) {
    int nilai = message.toInt();
    if (nilai >= 0 && nilai <= 255) {
      pwmWrite(nilai);
      mqtt.publish(TOPIC_STATUS_PWM, String(nilai).c_str());
    } else {
      Serial.println("Nilai PWM di luar 0-255");
    }
  }
}

void reconnectMQTT() {
  if (mqtt.connected()) return;
  if (millis() - lastReconnect < 5000) return;   // coba tiap 5 detik, tanpa memblokir
  lastReconnect = millis();
  Serial.print("Menghubungkan MQTT...");
  String clientId = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (mqtt.connect(clientId.c_str())) {
    Serial.println("terhubung");
    mqtt.subscribe(TOPIC_LED);
    mqtt.subscribe(TOPIC_PWM);
  } else {
    Serial.printf("gagal, rc=%d\n", mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PWM_PIN, 5000, 8);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PWM_PIN, 0);
#endif
  pwmWrite(0);
  dht.begin();
  connectWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  reconnectMQTT();
  mqtt.loop();

  if (mqtt.connected() && millis() - lastSensor >= INTERVAL_SENSOR) {
    lastSensor = millis();
    float suhu = dht.readTemperature();
    if (isnan(suhu)) {
      Serial.println("Gagal membaca DHT22");
    } else {
      mqtt.publish(TOPIC_SUHU, String(suhu, 1).c_str());
      Serial.printf("Publish suhu: %.1f C\n", suhu);
    }
  }
}
