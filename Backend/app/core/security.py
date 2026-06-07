import hashlib
import hmac
import secrets

_ALGORITHM = "sha256"
_ITERATIONS = 260_000


def hash_password(password: str) -> str:
    salt = secrets.token_hex(16)
    digest = hashlib.pbkdf2_hmac(_ALGORITHM, password.encode("utf-8"), bytes.fromhex(salt), _ITERATIONS)
    return f"{salt}${digest.hex()}"


def verify_password(password: str, password_hash: str) -> bool:
    salt, _, digest_hex = password_hash.partition("$")
    if not digest_hex:
        return False
    expected = hashlib.pbkdf2_hmac(_ALGORITHM, password.encode("utf-8"), bytes.fromhex(salt), _ITERATIONS)
    return hmac.compare_digest(expected.hex(), digest_hex)
