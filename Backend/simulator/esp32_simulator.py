"""
Simulador de um dispositivo ESP32 (interruptor inteligente).

Conecta no broker MQTT local, escuta comandos (relé, modo, dimmer, setpoint,
limites de dimmer, calibração) e responde publicando o novo estado, além de
publicar telemetria simulada periodicamente (incluindo luminosidade e energia).

Uso:
    python esp32_simulator.py [user_id] [device_id]

Exemplo:
    python esp32_simulator.py 1 switch01
"""

import json
import random
import sys
import threading
import time

import paho.mqtt.client as mqtt

MQTT_HOST = "localhost"
MQTT_PORT = 1883
TELEMETRY_INTERVAL_SECONDS = 5

user_id = sys.argv[1] if len(sys.argv) > 1 else "1"
device_id = sys.argv[2] if len(sys.argv) > 2 else "switch01"
label = f"{user_id}/{device_id}"
command_topic = f"devices/{user_id}/{device_id}/command"
state_topic = f"devices/{user_id}/{device_id}/state"
telemetry_topic = f"devices/{user_id}/{device_id}/telemetry"

relay_state = False
automatic_mode = False
dimmer = 100
dimmer_min = 10
dimmer_max = 100
setpoint = 400.0
natural_lux = 200.0
kwh_today = 0.0


def publish_state(client: mqtt.Client) -> None:
    payload = json.dumps({
        "relay": relay_state,
        "automatic_mode": automatic_mode,
        "dimmer": dimmer,
        "rssi": random.randint(-75, -40),
        "firmware": "sim-1.0.0",
        "wifi_name": "SimuladorWiFi",
    })
    client.publish(state_topic, payload, qos=1, retain=True)
    print(f"[{label}] -> {state_topic}: {payload}")


def on_connect(client, userdata, flags, reason_code, properties=None):
    print(f"[{label}] conectado ao broker (reason_code={reason_code})")
    client.subscribe(command_topic, qos=1)
    print(f"[{label}] escutando comandos em {command_topic}")
    publish_state(client)


def on_message(client, userdata, msg: mqtt.MQTTMessage):
    global relay_state, automatic_mode, dimmer, dimmer_min, dimmer_max, setpoint
    try:
        payload = json.loads(msg.payload.decode("utf-8"))
    except (ValueError, UnicodeDecodeError):
        print(f"[{label}] comando inválido recebido: {msg.payload!r}")
        return

    print(f"[{label}] <- {msg.topic}: {payload}")

    if "relay" in payload:
        relay_state = bool(payload["relay"])
    if "mode" in payload:
        automatic_mode = payload["mode"] == "auto"
    if "dimmer" in payload:
        dimmer = max(dimmer_min, min(dimmer_max, int(payload["dimmer"])))
    if "setpoint" in payload:
        setpoint = float(payload["setpoint"])
    if "dimmer_limits" in payload:
        limits = payload["dimmer_limits"]
        dimmer_min = int(limits.get("min", dimmer_min))
        dimmer_max = int(limits.get("max", dimmer_max))
        dimmer = max(dimmer_min, min(dimmer_max, dimmer))
    if payload.get("calibrate"):
        print(f"[{label}] calibrando sensor de luminosidade...")

    publish_state(client)


def telemetry_loop(client: mqtt.Client) -> None:
    global kwh_today
    while True:
        time.sleep(TELEMETRY_INTERVAL_SECONDS)
        base_power = 121.0 if relay_state else 0.0
        power = round(base_power + random.uniform(-2.0, 2.0), 1) if relay_state else 0.0
        kwh_today += power * (TELEMETRY_INTERVAL_SECONDS / 3600.0) / 1000.0

        if automatic_mode:
            lux = round(setpoint + random.uniform(-15.0, 15.0), 1)
        else:
            lux = round((dimmer / 100.0) * 800.0 + random.uniform(-10.0, 10.0), 1)

        payload = json.dumps({
            "sp": round(random.uniform(218.0, 222.0), 1),
            "mv": round(random.uniform(218.0, 222.0), 1),
            "current": round(random.uniform(0.0, 0.6), 2) if relay_state else 0.0,
            "power": power,
            "lux": lux,
            "natural_lux": round(natural_lux + random.uniform(-20.0, 20.0), 1),
            "dimmer": dimmer,
            "kwh_today": round(kwh_today, 3),
        })
        client.publish(telemetry_topic, payload, qos=1)
        print(f"[{label}] -> {telemetry_topic}: {payload}")


def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=f"esp32-sim-{user_id}-{device_id}")
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)

    threading.Thread(target=telemetry_loop, args=(client,), daemon=True).start()

    client.loop_forever()


if __name__ == "__main__":
    main()
