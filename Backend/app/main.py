from contextlib import asynccontextmanager

from fastapi import FastAPI

from app.api import auth, devices
from app.core.auth import SimpleAuthMiddleware
from app.db.session import Base, engine
from app.mqtt import publisher


@asynccontextmanager
async def lifespan(app: FastAPI):
    Base.metadata.create_all(bind=engine)
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
