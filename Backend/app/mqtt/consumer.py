import json
import logging

import paho.mqtt.client as mqtt

from app.core.config import settings
from app.db.session import Base, engine, SessionLocal
from app.services import device_service

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("mqtt-consumer")

STATE_TOPIC = "devices/+/state"
TELEMETRY_TOPIC = "devices/+/telemetry"


def _extract_device_id(topic: str) -> str:
    return topic.split("/")[1]


def _on_connect(client, userdata, flags, reason_code, properties=None):
    logger.info("Connected to MQTT broker with reason code %s", reason_code)
    client.subscribe([(STATE_TOPIC, 1), (TELEMETRY_TOPIC, 1)])


def _on_message(client, userdata, msg: mqtt.MQTTMessage):
    try:
        device_id = _extract_device_id(msg.topic)
        payload = json.loads(msg.payload.decode("utf-8"))
    except (IndexError, ValueError, UnicodeDecodeError) as exc:
        logger.warning("Ignoring invalid message on %s: %s", msg.topic, exc)
        return

    db = SessionLocal()
    try:
        if msg.topic.endswith("/state"):
            relay = bool(payload.get("relay"))
            device_service.upsert_state(db, device_id, relay)
            logger.info("Updated state for %s: relay=%s", device_id, relay)
        elif msg.topic.endswith("/telemetry"):
            device_service.insert_telemetry(
                db,
                device_id,
                sp=payload.get("sp"),
                mv=payload.get("mv"),
                current=payload.get("current"),
                power=payload.get("power"),
            )
            logger.info("Inserted telemetry for %s", device_id)
    finally:
        db.close()


def main():
    Base.metadata.create_all(bind=engine)

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="mqtt-consumer")
    if settings.mqtt_username:
        client.username_pw_set(settings.mqtt_username, settings.mqtt_password)

    client.on_connect = _on_connect
    client.on_message = _on_message

    client.connect(settings.mqtt_host, settings.mqtt_port, keepalive=60)
    client.loop_forever()


if __name__ == "__main__":
    main()
