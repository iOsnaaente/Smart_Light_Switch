from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    database_url: str = "postgresql://postgres:postgres@postgres:5432/smart_switch"

    mqtt_host: str = "mosquitto"
    mqtt_port: int = 1883
    mqtt_username: str | None = None
    mqtt_password: str | None = None

    auth_username: str = "admin"
    auth_password: str = "admin"

    class Config:
        env_file = ".env"


settings = Settings()
