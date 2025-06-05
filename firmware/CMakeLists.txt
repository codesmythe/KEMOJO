
cmake_minimum_required(VERSION 3.20)

project(ikbd)

set(CMAKE_CXX_STANDARD 11)

set(MCU atmega328p)
set(AVRDUDE_DIR /opt/local)
set(AVRDUDE_PORT  /dev/cu.usbmodem4101)

set_property(SOURCE firmware.ino PROPERTY LANGUAGE CXX)

include_directories(lib/arduino/core lib/arduino/variants/standard)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-threadsafe-statics -fpermissive" )
add_compile_options(-c -Os -Wall -ffunction-sections -fdata-sections -flto -mmcu=${MCU})

add_compile_definitions(ARDUINO_AVR_UNO;F_CPU=7372800L;ARDUINO_ARCH_AVR;ARDUINO=10808)

add_link_options(-Os -flto -fuse-linker-plugin -mmcu=${MCU} -Wl,--gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map -lm)

SET(LIBCORE_SOURCES  lib/arduino/core/HardwareSerial.cpp lib/arduino/core/HardwareSerial0.cpp
        lib/arduino/core/Print.cpp lib/arduino/core/WInterrupts.c lib/arduino/core/WMath.cpp
        lib/arduino/core/WString.cpp lib/arduino/core/abi.cpp lib/arduino/core/hooks.c lib/arduino/core/main.cpp
        lib/arduino/core/new.cpp lib/arduino/core/wiring.c lib/arduino/core/wiring_digital.c)

add_executable(ikbd firmware.ino ikbd.c ps2_keyboard.cpp ps2_mouse.cpp ps2.cpp util.cpp ${LIBCORE_SOURCES})

set(lfuse 0xf7)
set(hfuse 0xd7)
set(efuse 0xfc)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${HEX_FILE}
        COMMENT "Building ${HEX_FILE}")

add_custom_target(FLASH
        ${AVRDUDE_DIR}/bin/avrdude -C ${AVRDUDE_DIR}/etc/avrdude.conf -v -p${MCU} -cstk500v1 -b19200 -P${AVRDUDE_PORT}
        -U lfuse:w:${lfuse}:m -U hfuse:w:${hfuse}:m -U efuse:w:${efuse}:m  -U flash:w:${HEX_FILE}:i
        DEPENDS ${PROJECT_NAME}
        COMMENT "Flash to ${MCU}")
