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


def publish_command(user_id: int, device_id: str, payload: dict) -> bool:
    topic = f"devices/{user_id}/{device_id}/command"
    result = _client.publish(topic, json.dumps(payload), qos=1)
    return result.rc == mqtt.MQTT_ERR_SUCCESS
