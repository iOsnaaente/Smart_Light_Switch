from datetime import datetime

from pydantic import BaseModel, ConfigDict


class DeviceStateResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    device_id: str
    relay: bool
    updated_at: datetime


class TelemetryResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    device_id: str
    sp: float | None = None
    mv: float | None = None
    current: float | None = None
    power: float | None = None
    timestamp: datetime


class RelayCommandResponse(BaseModel):
    device_id: str
    relay: bool
    published: bool
