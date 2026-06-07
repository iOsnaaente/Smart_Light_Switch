from starlette.middleware.base import BaseHTTPMiddleware
from starlette.requests import Request
from starlette.responses import JSONResponse

from app.core.config import settings

PUBLIC_PATHS = {"/login", "/docs", "/openapi.json", "/redoc"}


class SimpleAuthMiddleware(BaseHTTPMiddleware):
    async def dispatch(self, request: Request, call_next):
        if request.url.path in PUBLIC_PATHS:
            return await call_next(request)

        username = request.headers.get("X-Username")
        password = request.headers.get("X-Password")

        if username != settings.auth_username or password != settings.auth_password:
            return JSONResponse(status_code=401, content={"detail": "Invalid credentials"})

        return await call_next(request)
