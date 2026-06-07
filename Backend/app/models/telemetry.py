from datetime import datetime

from sqlalchemy import ForeignKey, String, Float, Integer, DateTime, func
from sqlalchemy.orm import Mapped, mapped_column

from app.db.session import Base


class Telemetry(Base):
    __tablename__ = "telemetry"

    id: Mapped[int] = mapped_column(primary_key=True, index=True)
    user_id: Mapped[int] = mapped_column(ForeignKey("users.id"), index=True, nullable=False)
    device_id: Mapped[str] = mapped_column(String, index=True, nullable=False)
    sp: Mapped[float] = mapped_column(Float, nullable=True)
    mv: Mapped[float] = mapped_column(Float, nullable=True)
    current: Mapped[float] = mapped_column(Float, nullable=True)
    power: Mapped[float] = mapped_column(Float, nullable=True)
    lux: Mapped[float] = mapped_column(Float, nullable=True)
    natural_lux: Mapped[float] = mapped_column(Float, nullable=True)
    dimmer: Mapped[int] = mapped_column(Integer, nullable=True)
    kwh_today: Mapped[float] = mapped_column(Float, nullable=True)
    timestamp: Mapped[datetime] = mapped_column(DateTime(timezone=True), server_default=func.now())
