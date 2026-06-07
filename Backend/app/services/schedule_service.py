from sqlalchemy.orm import Session

from app.models import Schedule


def list_schedules(db: Session, user_id: int, device_id: str) -> list[Schedule]:
    return (
        db.query(Schedule)
        .filter(Schedule.user_id == user_id, Schedule.device_id == device_id)
        .order_by(Schedule.created_at.asc())
        .all()
    )


def get_schedule(db: Session, user_id: int, device_id: str, schedule_id: int) -> Schedule | None:
    return (
        db.query(Schedule)
        .filter(Schedule.user_id == user_id, Schedule.device_id == device_id, Schedule.id == schedule_id)
        .first()
    )


def create_schedule(
    db: Session,
    user_id: int,
    device_id: str,
    label: str,
    time: str,
    days_of_week: list[str],
    action: str,
) -> Schedule:
    schedule = Schedule(
        user_id=user_id,
        device_id=device_id,
        label=label,
        time=time,
        days_of_week=",".join(days_of_week),
        action=action,
    )
    db.add(schedule)
    db.commit()
    db.refresh(schedule)
    return schedule


def delete_schedule(db: Session, user_id: int, device_id: str, schedule_id: int) -> bool:
    schedule = get_schedule(db, user_id, device_id, schedule_id)
    if schedule is None:
        return False
    db.delete(schedule)
    db.commit()
    return True


def count_enabled(db: Session, user_id: int, device_id: str) -> int:
    return (
        db.query(Schedule)
        .filter(Schedule.user_id == user_id, Schedule.device_id == device_id, Schedule.enabled.is_(True))
        .count()
    )
