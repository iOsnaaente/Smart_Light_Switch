from fastapi import APIRouter

from app.schemas.auth import LoginRequest, LoginResponse
from app.core.config import settings

router = APIRouter(tags=["auth"])


@router.post("/login", response_model=LoginResponse)
def login(payload: LoginRequest):
    success = payload.username == settings.auth_username and payload.password == settings.auth_password
    return LoginResponse(success=success)
