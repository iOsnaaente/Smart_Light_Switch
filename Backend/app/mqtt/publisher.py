import json

import paho.mqtt.client as mqtt

from app.core.config import settings

_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="fastapi-publisher")

if settings.mqtt_username:
    _client.username_pw_set(settings.mqtt_username, settings.mqtt_password)


def connect():
    _client.connect(settings.mqtt_host, settings.mqtt_port, keepalive=60)
    _client.loop_start()


def disconnect():
    _client.loop_stop()
    _client.disconnect()


def publish_command(device_id: str, relay: bool) -> bool:
    topic = f"devices/{device_id}/command"
    payload = json.dumps({"relay": relay})
    result = _client.publish(topic, payload, qos=1)
    return result.rc == mqtt.MQTT_ERR_SUCCESS
