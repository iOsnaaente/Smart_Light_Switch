from datetime import datetime, timedelta, timezone

from sqlalchemy.orm import Session

from app.models import Device, DeviceState, Schedule, Telemetry
from app.services import schedule_service

ONLINE_THRESHOLD = timedelta(minutes=2)
HISTORY_BUCKET_MINUTES = 15


def get_or_create_device(db: Session, user_id: int, device_id: str) -> Device:
    device = (
        db.query(Device)
        .filter(Device.user_id == user_id, Device.device_id == device_id)
        .first()
    )
    now = datetime.now(timezone.utc)
    if device is None:
        device = Device(user_id=user_id, device_id=device_id, name=device_id, last_seen=now)
        db.add(device)
    else:
        device.last_seen = now
    db.commit()
    db.refresh(device)
    return device


def list_devices(db: Session, user_id: int) -> list[Device]:
    return (
        db.query(Device)
        .filter(Device.user_id == user_id)
        .order_by(Device.created_at.asc())
        .all()
    )


def get_device(db: Session, user_id: int, device_id: str) -> Device | None:
    return (
        db.query(Device)
        .filter(Device.user_id == user_id, Device.device_id == device_id)
        .first()
    )


def update_device(db: Session, user_id: int, device_id: str, name: str | None = None, room: str | None = None) -> Device | None:
    device = get_device(db, user_id, device_id)
    if device is None:
        return None
    if name is not None:
        device.name = name
    if room is not None:
        device.room = room
    db.commit()
    db.refresh(device)
    return device


def update_setpoint(db: Session, user_id: int, device_id: str, setpoint: float) -> Device | None:
    device = get_device(db, user_id, device_id)
    if device is None:
        return None
    device.setpoint = setpoint
    db.commit()
    db.refresh(device)
    return device


def update_dimmer_limits(db: Session, user_id: int, device_id: str, dimmer_min: int, dimmer_max: int) -> Device | None:
    device = get_device(db, user_id, device_id)
    if device is None:
        return None
    device.dimmer_min = dimmer_min
    device.dimmer_max = dimmer_max
    db.commit()
    db.refresh(device)
    return device


def update_reported_info(db: Session, user_id: int, device_id: str, firmware: str | None = None, wifi_name: str | None = None) -> None:
    device = get_or_create_device(db, user_id, device_id)
    changed = False
    if firmware is not None and device.firmware != firmware:
        device.firmware = firmware
        changed = True
    if wifi_name is not None and device.wifi_name != wifi_name:
        device.wifi_name = wifi_name
        changed = True
    if changed:
        db.commit()


def delete_device(db: Session, user_id: int, device_id: str) -> bool:
    device = get_device(db, user_id, device_id)
    if device is None:
        return False
    db.query(DeviceState).filter(DeviceState.user_id == user_id, DeviceState.device_id == device_id).delete()
    db.query(Telemetry).filter(Telemetry.user_id == user_id, Telemetry.device_id == device_id).delete()
    db.query(Schedule).filter(Schedule.user_id == user_id, Schedule.device_id == device_id).delete()
    db.delete(device)
    db.commit()
    return True


def get_latest_state(db: Session, user_id: int, device_id: str) -> DeviceState | None:
    return (
        db.query(DeviceState)
        .filter(DeviceState.user_id == user_id, DeviceState.device_id == device_id)
        .order_by(DeviceState.updated_at.desc())
        .first()
    )


def get_latest_telemetry(db: Session, user_id: int, device_id: str) -> Telemetry | None:
    return (
        db.query(Telemetry)
        .filter(Telemetry.user_id == user_id, Telemetry.device_id == device_id)
        .order_by(Telemetry.timestamp.desc())
        .first()
    )


def upsert_state(
    db: Session,
    user_id: int,
    device_id: str,
    relay: bool,
    automatic_mode: bool | None = None,
    dimmer: int | None = None,
    rssi: int | None = None,
) -> DeviceState:
    get_or_create_device(db, user_id, device_id)
    state = (
        db.query(DeviceState)
        .filter(DeviceState.user_id == user_id, DeviceState.device_id == device_id)
        .first()
    )
    if state is None:
        state = DeviceState(
            user_id=user_id,
            device_id=device_id,
            relay=relay,
            automatic_mode=bool(automatic_mode),
            dimmer=dimmer or 0,
            rssi=rssi,
        )
        db.add(state)
    else:
        state.relay = relay
        if automatic_mode is not None:
            state.automatic_mode = automatic_mode
        if dimmer is not None:
            state.dimmer = dimmer
        if rssi is not None:
            state.rssi = rssi
    db.commit()
    db.refresh(state)
    return state


def insert_telemetry(
    db: Session,
    user_id: int,
    device_id: str,
    sp: float | None,
    mv: float | None,
    current: float | None,
    power: float | None,
    lux: float | None = None,
    natural_lux: float | None = None,
    dimmer: int | None = None,
    kwh_today: float | None = None,
) -> Telemetry:
    get_or_create_device(db, user_id, device_id)
    telemetry = Telemetry(
        user_id=user_id,
        device_id=device_id,
        sp=sp,
        mv=mv,
        current=current,
        power=power,
        lux=lux,
        natural_lux=natural_lux,
        dimmer=dimmer,
        kwh_today=kwh_today,
    )
    db.add(telemetry)
    db.commit()
    db.refresh(telemetry)
    return telemetry


def is_online(device: Device) -> bool:
    if device.last_seen is None:
        return False
    last_seen = device.last_seen
    if last_seen.tzinfo is None:
        last_seen = last_seen.replace(tzinfo=timezone.utc)
    return datetime.now(timezone.utc) - last_seen < ONLINE_THRESHOLD


def _average(values: list[float | None]) -> float | None:
    present = [v for v in values if v is not None]
    if not present:
        return None
    return sum(present) / len(present)


def get_telemetry_history(db: Session, user_id: int, device_id: str, since: datetime) -> list[dict]:
    rows = (
        db.query(Telemetry)
        .filter(Telemetry.user_id == user_id, Telemetry.device_id == device_id, Telemetry.timestamp >= since)
        .order_by(Telemetry.timestamp.asc())
        .all()
    )

    buckets: dict[datetime, list[Telemetry]] = {}
    for row in rows:
        bucket_minute = (row.timestamp.minute // HISTORY_BUCKET_MINUTES) * HISTORY_BUCKET_MINUTES
        bucket_key = row.timestamp.replace(minute=bucket_minute, second=0, microsecond=0)
        buckets.setdefault(bucket_key, []).append(row)

    history = []
    for bucket_key in sorted(buckets):
        items = buckets[bucket_key]
        avg_dimmer = _average([item.dimmer for item in items])
        history.append({
            "timestamp": bucket_key,
            "lux": _average([item.lux for item in items]),
            "natural_lux": _average([item.natural_lux for item in items]),
            "dimmer": int(round(avg_dimmer)) if avg_dimmer is not None else None,
        })
    return history


def build_device_view(db: Session, device: Device) -> dict:
    state = get_latest_state(db, device.user_id, device.device_id)
    telemetry = get_latest_telemetry(db, device.user_id, device.device_id)
    active_schedules = schedule_service.count_enabled(db, device.user_id, device.device_id)

    return {
        "id": device.device_id,
        "name": device.name or device.device_id,
        "room": device.room,
        "online": is_online(device),
        "automatic_mode": state.automatic_mode if state else False,
        "relay_on": state.relay if state else False,
        "lux": telemetry.lux if telemetry and telemetry.lux is not None else 0.0,
        "natural_lux": telemetry.natural_lux if telemetry and telemetry.natural_lux is not None else 0.0,
        "setpoint": device.setpoint,
        "dimmer": state.dimmer if state else 0,
        "power_w": telemetry.power if telemetry and telemetry.power is not None else 0.0,
        "kwh_today": telemetry.kwh_today if telemetry and telemetry.kwh_today is not None else 0.0,
        "firmware": device.firmware,
        "rssi": state.rssi if state else None,
        "wifi_name": device.wifi_name,
        "active_schedules": active_schedules,
        "last_seen": device.last_seen,
        "dimmer_min": device.dimmer_min,
        "dimmer_max": device.dimmer_max,
    }
