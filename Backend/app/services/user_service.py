from sqlalchemy.orm import Session

from app.core.security import hash_password, verify_password
from app.models import User


def get_user_by_username(db: Session, username: str) -> User | None:
    return db.query(User).filter(User.username == username).first()


def create_user(db: Session, username: str, password: str) -> User:
    user = User(username=username, password_hash=hash_password(password))
    db.add(user)
    db.commit()
    db.refresh(user)
    return user


def authenticate(db: Session, username: str, password: str) -> User | None:
    user = get_user_by_username(db, username)
    if user is None or not verify_password(password, user.password_hash):
        return None
    return user


def ensure_default_user(db: Session, username: str, password: str) -> User:
    user = get_user_by_username(db, username)
    if user is None:
        user = create_user(db, username, password)
    return user
