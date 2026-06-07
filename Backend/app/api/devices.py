from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session

from app.db.session import get_db
from app.mqtt import publisher
from app.schemas.device import DeviceStateResponse, TelemetryResponse, RelayCommandResponse
from app.services import device_service

router = APIRouter(prefix="/devices", tags=["devices"])


@router.post("/{device_id}/relay/on", response_model=RelayCommandResponse)
def turn_relay_on(device_id: str):
    published = publisher.publish_command(device_id, True)
    return RelayCommandResponse(device_id=device_id, relay=True, published=published)


@router.post("/{device_id}/relay/off", response_model=RelayCommandResponse)
def turn_relay_off(device_id: str):
    published = publisher.publish_command(device_id, False)
    return RelayCommandResponse(device_id=device_id, relay=False, published=published)


@router.get("/{device_id}/state", response_model=DeviceStateResponse)
def get_state(device_id: str, db: Session = Depends(get_db)):
    state = device_service.get_latest_state(db, device_id)
    if state is None:
        raise HTTPException(status_code=404, detail="No state found for this device")
    return state


@router.get("/{device_id}/telemetry/latest", response_model=TelemetryResponse)
def get_latest_telemetry(device_id: str, db: Session = Depends(get_db)):
    telemetry = device_service.get_latest_telemetry(db, device_id)
    if telemetry is None:
        raise HTTPException(status_code=404, detail="No telemetry found for this device")
    return telemetry
