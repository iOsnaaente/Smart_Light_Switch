from datetime import datetime, timezone

from fastapi import APIRouter, Depends, HTTPException, Query, Request, status
from sqlalchemy.orm import Session

from app.db.session import get_db
from app.mqtt import publisher
from app.schemas.device import (
    CalibrateResponse,
    DeviceDetail,
    DeviceStateResponse,
    DeviceSummary,
    DeviceUpdateRequest,
    DimmerLimitsRequest,
    DimmerRequest,
    DimmerResponse,
    ModeRequest,
    ModeResponse,
    RelayCommandResponse,
    ScheduleRequest,
    ScheduleResponse,
    SetpointRequest,
    TelemetryHistoryPoint,
    TelemetryResponse,
)
from app.services import device_service, schedule_service

router = APIRouter(prefix="/devices", tags=["devices"])


def _get_owned_device(db: Session, user_id: int, device_id: str):
    device = device_service.get_device(db, user_id, device_id)
    if device is None:
        raise HTTPException(status_code=404, detail="Device not found")
    return device


@router.get("", response_model=list[DeviceSummary])
def list_devices(request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    devices = device_service.list_devices(db, user_id)
    return [device_service.build_device_view(db, device) for device in devices]


@router.get("/{device_id}", response_model=DeviceDetail)
def get_device(device_id: str, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    device = _get_owned_device(db, user_id, device_id)
    return device_service.build_device_view(db, device)


@router.put("/{device_id}", response_model=DeviceDetail)
def update_device(device_id: str, payload: DeviceUpdateRequest, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    device = device_service.update_device(db, user_id, device_id, name=payload.name, room=payload.room)
    return device_service.build_device_view(db, device)


@router.delete("/{device_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_device(device_id: str, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    device_service.delete_device(db, user_id, device_id)


@router.post("/{device_id}/relay/on", response_model=RelayCommandResponse)
def turn_relay_on(device_id: str, request: Request):
    user_id = request.state.user_id
    published = publisher.publish_command(user_id, device_id, {"relay": True})
    return RelayCommandResponse(user_id=user_id, device_id=device_id, relay=True, published=published)


@router.post("/{device_id}/relay/off", response_model=RelayCommandResponse)
def turn_relay_off(device_id: str, request: Request):
    user_id = request.state.user_id
    published = publisher.publish_command(user_id, device_id, {"relay": False})
    return RelayCommandResponse(user_id=user_id, device_id=device_id, relay=False, published=published)


@router.post("/{device_id}/mode", response_model=ModeResponse)
def set_mode(device_id: str, payload: ModeRequest, request: Request):
    user_id = request.state.user_id
    mode = "auto" if payload.automatic_mode else "manual"
    published = publisher.publish_command(user_id, device_id, {"mode": mode})
    return ModeResponse(device_id=device_id, automatic_mode=payload.automatic_mode, published=published)


@router.post("/{device_id}/dimmer", response_model=DimmerResponse)
def set_dimmer(device_id: str, payload: DimmerRequest, request: Request):
    user_id = request.state.user_id
    published = publisher.publish_command(user_id, device_id, {"dimmer": payload.dimmer})
    return DimmerResponse(device_id=device_id, dimmer=payload.dimmer, published=published)


@router.put("/{device_id}/setpoint", response_model=DeviceDetail)
def set_setpoint(device_id: str, payload: SetpointRequest, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    device = device_service.update_setpoint(db, user_id, device_id, payload.setpoint)
    publisher.publish_command(user_id, device_id, {"setpoint": payload.setpoint})
    return device_service.build_device_view(db, device)


@router.put("/{device_id}/dimmer-limits", response_model=DeviceDetail)
def set_dimmer_limits(device_id: str, payload: DimmerLimitsRequest, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    device = device_service.update_dimmer_limits(db, user_id, device_id, payload.dimmer_min, payload.dimmer_max)
    publisher.publish_command(
        user_id, device_id, {"dimmer_limits": {"min": payload.dimmer_min, "max": payload.dimmer_max}}
    )
    return device_service.build_device_view(db, device)


@router.post("/{device_id}/calibrate-sensor", response_model=CalibrateResponse)
def calibrate_sensor(device_id: str, request: Request):
    user_id = request.state.user_id
    published = publisher.publish_command(user_id, device_id, {"calibrate": True})
    return CalibrateResponse(device_id=device_id, published=published)


@router.get("/{device_id}/state", response_model=DeviceStateResponse)
def get_state(device_id: str, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    device = _get_owned_device(db, user_id, device_id)
    state = device_service.get_latest_state(db, user_id, device_id)
    if state is None:
        raise HTTPException(status_code=404, detail="No state found for this device")
    return DeviceStateResponse(
        user_id=state.user_id,
        device_id=state.device_id,
        relay=state.relay,
        automatic_mode=state.automatic_mode,
        dimmer=state.dimmer,
        online=device_service.is_online(device),
        updated_at=state.updated_at,
    )


@router.get("/{device_id}/telemetry/latest", response_model=TelemetryResponse)
def get_latest_telemetry(device_id: str, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    telemetry = device_service.get_latest_telemetry(db, user_id, device_id)
    if telemetry is None:
        raise HTTPException(status_code=404, detail="No telemetry found for this device")
    return telemetry


@router.get("/{device_id}/telemetry/history", response_model=list[TelemetryHistoryPoint])
def get_telemetry_history(device_id: str, request: Request, range: str = Query("today"), db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    if range != "today":
        raise HTTPException(status_code=400, detail="Unsupported range; only 'today' is available")
    today_start = datetime.now(timezone.utc).replace(hour=0, minute=0, second=0, microsecond=0)
    return device_service.get_telemetry_history(db, user_id, device_id, since=today_start)


@router.get("/{device_id}/schedules", response_model=list[ScheduleResponse])
def list_schedules(device_id: str, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    schedules = schedule_service.list_schedules(db, user_id, device_id)
    return [ScheduleResponse.from_model(schedule) for schedule in schedules]


@router.post("/{device_id}/schedules", response_model=ScheduleResponse, status_code=status.HTTP_201_CREATED)
def create_schedule(device_id: str, payload: ScheduleRequest, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    schedule = schedule_service.create_schedule(
        db, user_id, device_id, label=payload.label, time=payload.time, days_of_week=payload.days_of_week, action=payload.action
    )
    return ScheduleResponse.from_model(schedule)


@router.delete("/{device_id}/schedules/{schedule_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_schedule(device_id: str, schedule_id: int, request: Request, db: Session = Depends(get_db)):
    user_id = request.state.user_id
    _get_owned_device(db, user_id, device_id)
    if not schedule_service.delete_schedule(db, user_id, device_id, schedule_id):
        raise HTTPException(status_code=404, detail="Schedule not found")
