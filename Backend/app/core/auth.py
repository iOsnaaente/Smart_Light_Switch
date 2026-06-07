from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import JSONResponse

from app.db.session import SessionLocal
from app.services import user_service

PUBLIC_PATHS = {"/login", "/docs", "/openapi.json", "/redoc"}


class SimpleAuthMiddleware(BaseHTTPMiddleware):
    async def dispatch(self, request: Request, call_next):
        if request.url.path in PUBLIC_PATHS:
            return await call_next(request)

        username = request.headers.get("X-Username")
        password = request.headers.get("X-Password")

        if username is None or password is None:
            return JSONResponse(status_code=401, content={"detail": "Invalid credentials"})

        db = SessionLocal()
        try:
            user = user_service.authenticate(db, username, password)
        finally:
            db.close()

        if user is None:
            return JSONResponse(status_code=401, content={"detail": "Invalid credentials"})

        request.state.user_id = user.id
        request.state.username = user.username

        return await call_next(request)
