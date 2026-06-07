from datetime import datetime

from pydantic import BaseModel, ConfigDict


class DeviceStateResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    user_id: int
    device_id: str
    relay: bool
    automatic_mode: bool
    dimmer: int
    online: bool
    updated_at: datetime


class TelemetryResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    user_id: int
    device_id: str
    sp: float | None = None
    mv: float | None = None
    current: float | None = None
    power: float | None = None
    lux: float | None = None
    natural_lux: float | None = None
    kwh_today: float | None = None
    timestamp: datetime


class TelemetryHistoryPoint(BaseModel):
    timestamp: datetime
    lux: float | None = None
    natural_lux: float | None = None
    dimmer: int | None = None


class RelayCommandResponse(BaseModel):
    user_id: int
    device_id: str
    relay: bool
    published: bool


class DeviceSummary(BaseModel):
    id: str
    name: str
    room: str
    online: bool
    automatic_mode: bool
    relay_on: bool
    lux: float
    dimmer: int


class DeviceDetail(DeviceSummary):
    natural_lux: float
    setpoint: float
    power_w: float
    kwh_today: float
    firmware: str | None = None
    rssi: int | None = None
    wifi_name: str | None = None
    active_schedules: int
    last_seen: datetime | None = None
    dimmer_min: int
    dimmer_max: int


class ModeRequest(BaseModel):
    automatic_mode: bool


class ModeResponse(BaseModel):
    device_id: str
    automatic_mode: bool
    published: bool


class DimmerRequest(BaseModel):
    dimmer: int


class DimmerResponse(BaseModel):
    device_id: str
    dimmer: int
    published: bool


class SetpointRequest(BaseModel):
    setpoint: float


class DimmerLimitsRequest(BaseModel):
    dimmer_min: int
    dimmer_max: int


class CalibrateResponse(BaseModel):
    device_id: str
    published: bool


class DeviceUpdateRequest(BaseModel):
    name: str | None = None
    room: str | None = None


class ScheduleRequest(BaseModel):
    label: str
    time: str
    days_of_week: list[str]
    action: str


class ScheduleResponse(BaseModel):
    id: int
    label: str
    time: str
    days_of_week: list[str]
    action: str
    enabled: bool

    @classmethod
    def from_model(cls, schedule) -> "ScheduleResponse":
        return cls(
            id=schedule.id,
            label=schedule.label,
            time=schedule.time,
            days_of_week=schedule.days_of_week.split(",") if schedule.days_of_week else [],
            action=schedule.action,
            enabled=schedule.enabled,
        )
