from contextlib import asynccontextmanager

from fastapi import FastAPI

from app.api import auth, devices
from app.core.auth import SimpleAuthMiddleware
from app.core.config import settings
from app.db.session import Base, SessionLocal, engine
from app.mqtt import publisher
from app.services import user_service


@asynccontextmanager
async def lifespan(app: FastAPI):
    Base.metadata.create_all(bind=engine)

    db = SessionLocal()
    try:
        user_service.ensure_default_user(db, settings.auth_username, settings.auth_password)
    finally:
        db.close()

    publisher.connect()
    yield
    publisher.disconnect()


app = FastAPI(title="Smart Light Switch API", lifespan=lifespan)

app.add_middleware(SimpleAuthMiddleware)

app.include_router(auth.router)
app.include_router(devices.router)


@app.get("/health")
def health():
    return {"status": "ok"}
