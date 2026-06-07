from fastapi import APIRouter, Depends
from sqlalchemy.orm import Session

from app.db.session import get_db
from app.schemas.auth import LoginRequest, LoginResponse
from app.services import user_service

router = APIRouter(tags=["auth"])


@router.post("/login", response_model=LoginResponse)
def login(payload: LoginRequest, db: Session = Depends(get_db)):
    user = user_service.authenticate(db, payload.username, payload.password)
    if user is None:
        return LoginResponse(success=False)
    return LoginResponse(success=True, user_id=user.id)
