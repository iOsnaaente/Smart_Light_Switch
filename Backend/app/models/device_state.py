from datetime import datetime

from sqlalchemy import ForeignKey, String, Boolean, Integer, DateTime, func
from sqlalchemy.orm import Mapped, mapped_column

from app.db.session import Base


class DeviceState(Base):
    __tablename__ = "device_state"

    id: Mapped[int] = mapped_column(primary_key=True, index=True)
    user_id: Mapped[int] = mapped_column(ForeignKey("users.id"), index=True, nullable=False)
    device_id: Mapped[str] = mapped_column(String, index=True, nullable=False)
    relay: Mapped[bool] = mapped_column(Boolean, nullable=False, default=False)
    automatic_mode: Mapped[bool] = mapped_column(Boolean, nullable=False, default=False, server_default="false")
    dimmer: Mapped[int] = mapped_column(Integer, nullable=False, default=0, server_default="0")
    rssi: Mapped[int] = mapped_column(Integer, nullable=True)
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), server_default=func.now(), onupdate=func.now()
    )
