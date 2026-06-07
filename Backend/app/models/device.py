from datetime import datetime

from sqlalchemy import ForeignKey, String, Float, Integer, DateTime, UniqueConstraint, func
from sqlalchemy.orm import Mapped, mapped_column

from app.db.session import Base


class Device(Base):
    __tablename__ = "devices"
    __table_args__ = (UniqueConstraint("user_id", "device_id", name="uq_devices_user_id_device_id"),)

    id: Mapped[int] = mapped_column(primary_key=True, index=True)
    user_id: Mapped[int] = mapped_column(ForeignKey("users.id"), index=True, nullable=False)
    device_id: Mapped[str] = mapped_column(String, index=True, nullable=False)
    name: Mapped[str] = mapped_column(String, nullable=True)
    room: Mapped[str] = mapped_column(String, nullable=False, default="Geral", server_default="Geral")
    firmware: Mapped[str] = mapped_column(String, nullable=True)
    wifi_name: Mapped[str] = mapped_column(String, nullable=True)
    setpoint: Mapped[float] = mapped_column(Float, nullable=False, default=400.0, server_default="400")
    dimmer_min: Mapped[int] = mapped_column(Integer, nullable=False, default=10, server_default="10")
    dimmer_max: Mapped[int] = mapped_column(Integer, nullable=False, default=100, server_default="100")
    last_seen: Mapped[datetime] = mapped_column(DateTime(timezone=True), nullable=True)
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now())
