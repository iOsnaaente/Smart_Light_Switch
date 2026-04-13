# CMake script to convert binary image to C source code
# Usage: cmake -DIMG_NAME=... -DINPUT_BIN=... -DOUTPUT_C=... -P bin2c.cmake

if(NOT DEFINED IMG_NAME OR NOT DEFINED INPUT_BIN OR NOT DEFINED OUTPUT_C)
    message(FATAL_ERROR "Required variables not set")
endif()

# Read binary file
file(READ "${INPUT_BIN}" HEX_DATA HEX)

# Read binary as raw bytes
file(READ "${INPUT_BIN}" BINARY_DATA)
string(LENGTH "${BINARY_DATA}" FILE_SIZE)

# Convert to hex string with proper formatting
set(C_DATA "")
set(COUNTER 0)

foreach(BYTE_HEX_STR IN ZIP_LISTS HEX_DATA HEX_DATA_ZIPPED)
    if(COUNTER EQUAL 0)
        string(APPEND C_DATA "    ")
    endif()
    
    list(APPEND C_DATA "0x${HEX_DATA}")
    math(EXPR COUNTER "(${COUNTER} + 1) % 16")
    
    if(COUNTER EQUAL 0)
        string(APPEND C_DATA ",\n")
    else()
        string(APPEND C_DATA ",")
    endif()
endforeach()

# Generate C source - simpler approach using xxd output
execute_process(
    COMMAND python3 -c "
import sys
with open('${INPUT_BIN}', 'rb') as f:
    data = f.read()
with open('${OUTPUT_C}', 'w') as f:
    f.write('/* Auto-generated image data */\n')
    f.write('/* Generated from: ${IMG_NAME} */\n')
    f.write('#include \"image_${IMG_NAME}.h\"\n\n')
    f.write('const uint8_t image_${IMG_NAME}_data[] = {\n')
    for i, b in enumerate(data):
        if i % 16 == 0:
            f.write('    ')
        f.write('0x{:02x}'.format(b))
        if i < len(data) - 1:
            f.write(',')
        if (i + 1) % 16 == 0:
            f.write('\n')
    if len(data) % 16 != 0:
        f.write('\n')
    f.write('};\n')
"
)

message(STATUS "Generated ${OUTPUT_C}")
