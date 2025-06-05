// kemojo_firmware.cpp
// Copyright (c) 2025 Rob Gowin
// Copyright (c) 2023-2024 Daniel Cliche
// SPDX-License-Identifier: MIT

#include "ps2_keyboard.h"
#include "ps2_mouse.h"

#include "config.h"
#include "ikbd.h"
#include "ps2.h"
#include "util.h"

PS2Keyboard keyboard;
PS2Mouse mouse;
KEYBOARD Keyboard;
KEYBOARD_PROCESSOR KeyboardProcessor;

#include <Arduino.h>

#include "scan_code_maps_flash.c"

PROGMEM constexpr char key_press_msg[] = "    Key pressed: ";
PROGMEM constexpr char key_release_msg[] = "    Key released: ";
PROGMEM constexpr char scan_code_msg[] = "Scan code: 0x";
PROGMEM constexpr char st_scan_code_msg[] = "    Atari ST Scan Code: 0x";

void setup()
{
  // Configure the debug LED pin as output
  pinMode(PIN7, OUTPUT);
  Serial.begin(SERIAL_BAUD_RATE);

  // Set up the joystick ports by setting bits 5:0 of ports B and C to inputs.
  DDRB = DDRB & 0xC0;
  DDRC = DDRC & 0xC0;
  // And pull them up.
  PORTB = PORTB | 0x3F;
  PORTC = PORTC | 0x3F;

#if KEYBOARD_ENA
    PS2Keyboard::begin(PS2_KEYBOARD_CLK_PIN, PS2_KEYBOARD_DATA_PIN);
#endif
#if MOUSE_ENA
    PS2Mouse::begin(PS2_MOUSE_CLK_PIN, PS2_MOUSE_DATA_PIN);
#endif
    IKBD_Reset(true);
}

void turn_LED_on()
{
  digitalWrite(PIN7, 0);
}

void turn_LED_off()
{
  digitalWrite(PIN7, 1);
}

void check_ikbd_output_buffer()
{
  if ( Keyboard.NbBytesInOutputBuffer > 0 && Keyboard.PauseOutput == false ) {
    const unsigned char ch = Keyboard.Buffer[ Keyboard.BufferHead++ ];
    Keyboard.BufferHead &= KEYBOARD_BUFFER_MASK;
    Keyboard.NbBytesInOutputBuffer--;
    send_byte(ch);
  }
}

#define Y_OVERFLOW (1 << 7)
#define X_OVERFLOW (1 << 6)
#define Y_SIGN (1 << 5)
#define X_SIGN (1 << 4)
#define MIDDLE_BUTTON (1 << 2)
#define RIGHT_BUTTON (1 << 1)
#define LEFT_BUTTON (1 << 0)

#if DEBUG

static void print_hex_byte(uint8_t value) {
  uint8_t upper = (value >> 4) & 0xF, lower = value & 0xF;
  Serial.write(upper < 10 ? upper + '0' : upper + 'A' - 10);
  Serial.write(lower < 10 ? lower + '0' : lower + 'A' - 10);
}

static void show_scan_code(uint8_t code)
{
  Serial.print(reinterpret_cast<const __FlashStringHelper *>(scan_code_msg));
  print_hex_byte(code);
  Serial.println("");
}

static void show_st_scan_code(uint8_t code)
{
  Serial.print(reinterpret_cast<const __FlashStringHelper *>(st_scan_code_msg));
  print_hex_byte(code);
  Serial.println("");
}

static void show_key(uint8_t code, uint8_t extended, uint8_t brk)
{
  char key[20];
  if (extended) strcpy_P(key, reinterpret_cast<const char *>(pgm_read_word(&extended_ps2_make_code_map_flash[code])));
  else strcpy_P(key, reinterpret_cast<const char *>(pgm_read_word(&ps2_make_code_map_flash[code])));
  if (brk) Serial.print(reinterpret_cast<const __FlashStringHelper *>(key_release_msg));
  else Serial.print(reinterpret_cast<const __FlashStringHelper *>(key_press_msg));

  Serial.print(key);
  Serial.print("   --->   ");
}

#endif

extern "C" {
  uint8_t Joy_GetStickData(const int nStJoyId)
  {
    // FIXME: Deal with joystick emulation and autofire. See full function in Hatari src/joy.c.
    uint8_t result = 0;
    // Get the value of the GPIO port
    uint8_t gpio_value = (nStJoyId == 0 ? PINB : PINC) & 0x3F;

    // Arrange the bits in the order expected by the IKBD protocol.
    if (!(gpio_value & GPIO_MASK_UP))    result |= ATARIJOY_BITMASK_UP;
    if (!(gpio_value & GPIO_MASK_DOWN))  result |= ATARIJOY_BITMASK_DOWN;
    if (!(gpio_value & GPIO_MASK_LEFT))  result |= ATARIJOY_BITMASK_LEFT;
    if (!(gpio_value & GPIO_MASK_RIGHT)) result |= ATARIJOY_BITMASK_RIGHT;
    if (!(gpio_value & GPIO_MASK_FIRE))  result |= ATARIJOY_BITMASK_FIRE;

    return result;
  }
}

#if KEYBOARD_ENA


void poll_keyboard()
{
  bool avail = false, buffer_overflow = false, leave_led_on = false;
  static bool brk = false, extended = false, caps_lock_led = false;
  const uint8_t code = PS2Keyboard::read(&avail, &buffer_overflow);
  if (buffer_overflow) leave_led_on = true;
  if (avail) {
#if DEBUG
    show_scan_code(code);
#endif
    turn_LED_on();
    if (code == 0xF0) brk = true;
    else if (code == 0xE0) extended = true;
    else if (code == 0xE1) { // Pause/Break handing
      // E1 14 77 E1 F0 14 F0 77
      // Eat the next seven bytes
      int remaining = 7;

      while (remaining) {
        do {
          PS2Keyboard::read(&avail, &buffer_overflow);
        } while (!avail);
        remaining--;
      }
      // This key press/release is completely ignored.
    } else if (code != 0xAA) {
      uint8_t st_scan_code = pgm_read_byte(&st_make_code_map[code]);
      if (extended) st_scan_code = pgm_read_byte(&st_extended_make_code_map[code]);
      IKBD_PressSTKey(st_scan_code, !brk);
      if (code == 0x58 && !brk) { // caps lock
        caps_lock_led = !caps_lock_led;
        PS2Keyboard::set_caps_lock_led(caps_lock_led);
      }
      brk = false;
      extended = false;
    }
    if (!leave_led_on) turn_LED_off();
  }
}
#endif

void poll_mouse()
{
  bool avail = false, buffer_overflow = false, leave_led_on = false;
  const uint8_t mstat = PS2Mouse::read(&avail, &buffer_overflow);
  if (buffer_overflow) leave_led_on = true;
  if (avail) {
    turn_LED_on();
    uint8_t dx8 = 0, dy8 = 0;
    do {
      dx8 = PS2Mouse::read(&avail, &buffer_overflow);
      if (buffer_overflow) leave_led_on = true;
    } while (!avail); // Keep attempting to read a byte while nothing available in the buffer.
    do {
      dy8 = PS2Mouse::read(&avail, &buffer_overflow);
      if (buffer_overflow) leave_led_on = true;
    } while (!avail);
    unsigned int dx = dx8, dy = dy8;
    if (mstat & X_SIGN) dx = 0xFF00 | dx8; /* sign extend */
    if (mstat & Y_SIGN) dy = 0xFF00 | dy8;
    KeyboardProcessor.Mouse.dx += (int) dx;
    KeyboardProcessor.Mouse.dy += (int) dy;

    if (mstat & RIGHT_BUTTON) Keyboard.bRButtonDown |= BUTTON_MOUSE;
    else Keyboard.bRButtonDown &= ~BUTTON_MOUSE;

    if (mstat & LEFT_BUTTON) Keyboard.bLButtonDown |= BUTTON_MOUSE;
    else Keyboard.bLButtonDown &= ~BUTTON_MOUSE;

    /* FIXME: Deal with mouse overflow bits. */
    if (!leave_led_on) turn_LED_off();
  }
}

void loop() {
  bool avail = false;
  // See if there is an incoming command byte.
  const unsigned char c = recv_byte(&avail);
  if (avail) IKBD_RunKeyboardCommand(c);
  // Next, we will check if there is keyboard or mouse activity.
  poll_mouse();
#if KEYBOARD_ENA
  poll_keyboard();
#endif
  // See if the IKBD has any response.
  IKBD_SendAutoKeyboardCommands();
  check_ikbd_output_buffer();
}
