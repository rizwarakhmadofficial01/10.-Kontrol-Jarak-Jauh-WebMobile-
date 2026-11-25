"""
Pertemuan 10 - Simulasi perangkat IoT (pengganti ESP32).

Fungsi:
1. Membaca suhu (simulasi, atau DHT22 asli di Raspberry Pi)
2. Publish suhu ke MQTT setiap 5 detik
3. Subscribe topik kontrol LED (ON/OFF) dan PWM (0-255), lalu kirim feedback status
"""
import random
import threading
import time

import paho.mqtt.client as mqtt

# ================= KONFIGURASI =================
MQTT_BROKER = "localhost"          # Mosquitto lokal
MQTT_PORT = 1883
MQTT_CLIENT_ID = "PythonDevice-001"
TOPIC_PREFIX = "iot/kelas"         # untuk tugas ganti jadi "iot/NIM_ANDA"
INTERVAL_PUBLISH = 5               # detik

TOPIC_SUHU = f"{TOPIC_PREFIX}/sensor/suhu"
TOPIC_LED = f"{TOPIC_PREFIX}/aktuator/led"
TOPIC_PWM = f"{TOPIC_PREFIX}/aktuator/pwm"
TOPIC_STATUS = f"{TOPIC_PREFIX}/status/led"        # feedback status LED
TOPIC_STATUS_PWM = f"{TOPIC_PREFIX}/status/pwm"    # feedback nilai PWM

# ================= STATE GLOBAL =================
led_state = "OFF"
pwm_value = 0


# ================= CALLBACK MQTT =================
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code.is_failure:
        print(f"[MQTT] Gagal terhubung: {reason_code}")
        return
    print("[MQTT] Terhubung ke broker")
    client.subscribe([(TOPIC_LED, 0), (TOPIC_PWM, 0)])
    print(f"[MQTT] Subscribe ke topik: {TOPIC_LED} dan {TOPIC_PWM}")


def on_message(client, userdata, msg):
    """Dipanggil setiap ada pesan pada topik yang di-subscribe."""
    global led_state, pwm_value
    payload = msg.payload.decode("utf-8", errors="replace").strip()
    print(f"[MQTT] Pesan masuk [{msg.topic}]: {payload}")

    if msg.topic == TOPIC_LED:
        if payload in ("ON", "1"):
            led_state = "ON"
            print("[AKTUATOR] LED MENYALA")
        elif payload in ("OFF", "0"):
            led_state = "OFF"
            print("[AKTUATOR] LED MATI")
        else:
            print(f"[AKTUATOR] Perintah tidak dikenal: {payload}")
            return
        client.publish(TOPIC_STATUS, led_state, qos=0)
        print(f"[MQTT] Publish status: {TOPIC_STATUS} -> {led_state}")

    elif msg.topic == TOPIC_PWM:
        try:
            nilai = int(float(payload))
        except ValueError:
            print(f"[AKTUATOR] Nilai PWM tidak valid: {payload}")
            return
        if not 0 <= nilai <= 255:
            print(f"[AKTUATOR] PWM di luar rentang 0-255: {nilai}")
            return
        pwm_value = nilai
        print(f"[AKTUATOR] PWM LED = {pwm_value} ({round(pwm_value / 255 * 100)}%)")
        client.publish(TOPIC_STATUS_PWM, str(pwm_value), qos=0)
        print(f"[MQTT] Publish status: {TOPIC_STATUS_PWM} -> {pwm_value}")


def on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    print(f"[MQTT] Terputus (kode: {reason_code}). Mencoba reconnect...")


# ================= PEMBACAAN SENSOR =================
def baca_suhu():
    """Simulasi DHT22. Untuk DHT22 asli di Raspberry Pi, ganti dengan Adafruit_DHT."""
    return round(27.0 + random.uniform(-1.5, 1.5), 2)


# ================= THREAD PUBLISH SENSOR =================
def publish_loop(client):
    """Loop terpisah agar publish sensor tidak memblokir penerimaan pesan."""
    while True:
        try:
            suhu = baca_suhu()
            if suhu is not None and client.is_connected():
                client.publish(TOPIC_SUHU, str(suhu), qos=0)
                print(f"[SENSOR] Publish suhu: {TOPIC_SUHU} -> {suhu} °C")
        except Exception as err:  # jangan sampai thread mati
            print(f"[SENSOR] Error: {err}")
        time.sleep(INTERVAL_PUBLISH)


# ================= MAIN =================
def main():
    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id=MQTT_CLIENT_ID,
        protocol=mqtt.MQTTv311,
    )
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect
    client.reconnect_delay_set(min_delay=1, max_delay=30)

    print(f"[MQTT] Menghubungkan ke {MQTT_BROKER}:{MQTT_PORT}...")
    client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
    client.loop_start()          # jangan pakai loop_forever(), akan memblokir
    time.sleep(1)

    threading.Thread(target=publish_loop, args=(client,), daemon=True).start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[MAIN] Menghentikan program...")
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
