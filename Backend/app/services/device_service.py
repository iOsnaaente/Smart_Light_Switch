from sqlalchemy.orm import Session

from app.models import Device, DeviceState, Telemetry


def get_or_create_device(db: Session, device_id: str) -> Device:
    device = db.query(Device).filter(Device.device_id == device_id).first()
    if device is None:
        device = Device(device_id=device_id, name=device_id)
        db.add(device)
        db.commit()
        db.refresh(device)
    return device


def get_latest_state(db: Session, device_id: str) -> DeviceState | None:
    return (
        db.query(DeviceState)
        .filter(DeviceState.device_id == device_id)
        .order_by(DeviceState.updated_at.desc())
        .first()
    )


def get_latest_telemetry(db: Session, device_id: str) -> Telemetry | None:
    return (
        db.query(Telemetry)
        .filter(Telemetry.device_id == device_id)
        .order_by(Telemetry.timestamp.desc())
        .first()
    )


def upsert_state(db: Session, device_id: str, relay: bool) -> DeviceState:
    get_or_create_device(db, device_id)
    state = db.query(DeviceState).filter(DeviceState.device_id == device_id).first()
    if state is None:
        state = DeviceState(device_id=device_id, relay=relay)
        db.add(state)
    else:
        state.relay = relay
    db.commit()
    db.refresh(state)
    return state


def insert_telemetry(
    db: Session,
    device_id: str,
    sp: float | None,
    mv: float | None,
    current: float | None,
    power: float | None,
) -> Telemetry:
    get_or_create_device(db, device_id)
    telemetry = Telemetry(device_id=device_id, sp=sp, mv=mv, current=current, power=power)
    db.add(telemetry)
    db.commit()
    db.refresh(telemetry)
    return telemetry
