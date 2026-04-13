#!/usr/bin/env python3
"""
Convert PNG/JPG image to RGB565 raw binary format for ESP32 display.
Resizes to target dimensions and generates C header with embedded image data.

Usage:
    python convert_image.py <input_image> <output_h> <output_bin> <width> <height>
"""

import sys
import struct
from pathlib import Path

def rgb565(r, g, b):
    """Convert RGB888 to RGB565 format (5-6-5 bits)"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_image_to_rgb565(input_path, width, height, output_bin, output_h):
    """Convert image to RGB565 binary and generate C header"""
    
    try:
        from PIL import Image
    except ImportError:
        print("ERROR: Pillow library not found. Install with: pip install Pillow")
        sys.exit(1)
    
    # Open and resize image
    print(f"Reading image: {input_path}")
    img = Image.open(input_path)
    print(f"Original size: {img.size[0]}x{img.size[1]}")
    
    # Convert to RGB if needed
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # Resize to target dimensions
    img_resized = img.resize((width, height), Image.Resampling.LANCZOS)
    print(f"Resized to: {width}x{height}")
    
    # Convert to RGB565
    pixels = img_resized.getdata()
    rgb565_data = bytearray()
    
    for pixel in pixels:
        r, g, b = pixel
        color = rgb565(r, g, b)
        # Write as little-endian 16-bit (LSB first)
        rgb565_data.append(color & 0xFF)
        rgb565_data.append((color >> 8) & 0xFF)
    
    # Write binary file
    with open(output_bin, 'wb') as f:
        f.write(rgb565_data)
    print(f"Written binary: {output_bin} ({len(rgb565_data)} bytes)")
    
    # Generate C header file with image data
    image_name = Path(input_path).stem.upper()
    
    header_content = \
    f"""// Auto-generated image header
    // Generated from: {Path(input_path).name}
    // Dimensions: {width}x{height}
    // Format: RGB565 (16-bit per pixel)

    #ifndef IMAGE_{image_name}_H_
    #define IMAGE_{image_name}_H_

    #include <stdint.h>

    #define IMAGE_{image_name}_WIDTH  {width}
    #define IMAGE_{image_name}_HEIGHT {height}
    #define IMAGE_{image_name}_SIZE   ({width} * {height} * 2)  // bytes

    // Raw RGB565 image data
    extern const uint8_t image_{image_name.lower()}_data[];

    #endif // IMAGE_{image_name}_H_
    """
    
    with open(output_h, 'w') as f:
        f.write(header_content)
    print(f"Written header: {output_h}")
    
    return len(rgb565_data)

if __name__ == '__main__':
    if len(sys.argv) != 6:
        print("Usage: convert_image.py <input_image> <output_h> <output_bin> <width> <height>")
        print("Example: convert_image.py photo.png image.h image.bin 240 320")
        sys.exit(1)
    
    input_image = sys.argv[1]
    output_h = sys.argv[2]
    output_bin = sys.argv[3]
    width = int(sys.argv[4])
    height = int(sys.argv[5])
    
    if not Path(input_image).exists():
        print(f"ERROR: Image file not found: {input_image}")
        sys.exit(1)
    
    convert_image_to_rgb565(input_image, width, height, output_bin, output_h)
    print("Conversion complete!")
