"""Generates the SmartLight app icon (assets/icon/*.png) without external
dependencies (stdlib only: struct + zlib for PNG encoding). The mark is the
same amber bulb used by SmartLightBulbIcon in lib/widgets/animated_icons.dart,
rasterized with signed-distance fields for clean anti-aliased edges.

Run with: python tool/generate_app_icon.py
Re-run any time the mark or palette needs to change, then regenerate the
platform icons with: dart run flutter_launcher_icons
"""
import math
import struct
import zlib

W = H = 1024
CX, CY = 512.0, 512.0

PAPER = (0xFA, 0xF8, 0xF4)
ACCENT = (0xD9, 0x94, 0x2B)       # warm lamp light
ACCENT_INK = (0x9A, 0x64, 0x12)   # ring / base
HIGHLIGHT = (0xFF, 0xD2, 0x7E)    # glass shine
THREAD = (0x7A, 0x5A, 0x1E)       # base thread lines


def sdf_circle(dx, dy, r):
    return math.sqrt(dx * dx + dy * dy) - r


def sdf_round_rect(dx, dy, hw, hh, rad):
    qx = abs(dx) - (hw - rad)
    qy = abs(dy) - (hh - rad)
    outside = math.sqrt(max(qx, 0.0) ** 2 + max(qy, 0.0) ** 2)
    inside = min(max(qx, qy), 0.0)
    return outside + inside - rad


def shapes_at(scale):
    """Bulb mark shapes, uniformly scaled about the canvas center."""
    def s(v):
        return CX + (v - CX) * scale, CY + (v - CY) * scale

    bulb_cx, bulb_cy = s(CX)[0], 470.0
    bulb_cy = CY + (bulb_cy - CY) * scale
    r_outer = 300.0 * scale
    r_inner = 262.0 * scale

    base_cy = CY + ((470.0 + 300.0 * 0.92) - CY) * scale

    return [
        ("circle", bulb_cx, bulb_cy, r_outer, 0, ACCENT_INK, 1.0),
        ("circle", bulb_cx, bulb_cy, r_inner, 0, ACCENT, 1.0),
        ("circle", bulb_cx - 95.0 * scale, bulb_cy - 110.0 * scale, 80.0 * scale, 0, HIGHLIGHT, 0.55),
        ("rrect", bulb_cx, base_cy, 125.0 * scale, 78.0 * scale, 30.0 * scale, ACCENT_INK, 1.0),
        ("rrect", bulb_cx, CY + ((470.0 + 230.0) - CY) * scale, 95.0 * scale, 8.0 * scale, 8.0 * scale, THREAD, 0.9),
        ("rrect", bulb_cx, CY + ((470.0 + 270.0) - CY) * scale, 95.0 * scale, 8.0 * scale, 8.0 * scale, THREAD, 0.9),
    ]


def coverage(shape, x, y):
    kind = shape[0]
    if kind == "circle":
        _, cx, cy, r, _, _, _ = shape
        d = sdf_circle(x - cx, y - cy, r)
    else:
        _, cx, cy, hw, hh, rad, _, _ = shape
        d = sdf_round_rect(x - cx, y - cy, hw, hh, rad)
    return max(0.0, min(1.0, 0.5 - d))


def bbox(shape):
    if shape[0] == "circle":
        _, cx, cy, r, _, _, _ = shape
        pad = r + 2
        return cx - pad, cy - pad, cx + pad, cy + pad
    _, cx, cy, hw, hh, rad, _, _ = shape
    pad = 2
    return cx - hw - pad, cy - hh - pad, cx + hw + pad, cy + hh + pad


def shape_color_alpha(shape):
    if shape[0] == "circle":
        return shape[5], shape[6]
    return shape[6], shape[7]


def render(transparent_bg, scale):
    bg = (0, 0, 0, 0) if transparent_bg else (PAPER[0], PAPER[1], PAPER[2], 255)
    pixels = bytearray(bg * (W * H))

    for shape in shapes_at(scale):
        color, base_alpha = shape_color_alpha(shape)
        x0, y0, x1, y1 = bbox(shape)
        x0 = max(0, int(x0)); y0 = max(0, int(y0))
        x1 = min(W, int(x1) + 1); y1 = min(H, int(y1) + 1)
        for y in range(y0, y1):
            row_off = y * W * 4
            for x in range(x0, x1):
                c = coverage(shape, x + 0.5, y + 0.5)
                if c <= 0:
                    continue
                sa = c * base_alpha
                off = row_off + x * 4
                dr, dg, db, da = pixels[off], pixels[off + 1], pixels[off + 2], pixels[off + 3]
                da_f = da / 255.0
                out_a = sa + da_f * (1 - sa)
                if out_a <= 0:
                    continue
                out_r = (color[0] * sa + dr * da_f * (1 - sa)) / out_a
                out_g = (color[1] * sa + dg * da_f * (1 - sa)) / out_a
                out_b = (color[2] * sa + db * da_f * (1 - sa)) / out_a
                pixels[off] = int(round(out_r))
                pixels[off + 1] = int(round(out_g))
                pixels[off + 2] = int(round(out_b))
                pixels[off + 3] = int(round(out_a * 255))
    return pixels


def write_png(path, pixels):
    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0)
    raw = bytearray()
    for y in range(H):
        raw.append(0)
        raw.extend(pixels[y * W * 4:(y + 1) * W * 4])
    idat = zlib.compress(bytes(raw), 9)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", idat))
        f.write(chunk(b"IEND", b""))


if __name__ == "__main__":
    write_png("assets/icon/app_icon.png", render(transparent_bg=False, scale=1.0))
    write_png("assets/icon/app_icon_foreground.png", render(transparent_bg=True, scale=0.62))
    print("wrote assets/icon/app_icon.png and assets/icon/app_icon_foreground.png")
