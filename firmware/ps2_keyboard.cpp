// ps2_keyboard.cpp
// Copyright (c) 2023-2024 Daniel Cliche
// SPDX-License-Identifier: MIT

// Ref.:
//  - https://github.com/kristopher/PS2-Mouse-Arduino
//  - https://gist.github.com/mifritscher/fe8058a9ec294522a88d3d62fb9f6498

#include "ps2_keyboard.h"
#include "ps2.h"

#include <Arduino.h>

#include "config.h"

#define BUFFER_SIZE 128

static volatile uint8_t g_buffer[BUFFER_SIZE];
static volatile uint8_t g_head, g_tail;
static volatile bool g_buffer_overflow;
static uint8_t g_clk_pin, g_data_pin;

static void clk_interrupt()
{
    static uint8_t bitcount = 0;
    static uint8_t incoming = 0;
    static uint32_t prev_ms = 0;

    const uint8_t val = digitalRead(g_data_pin);
    const uint32_t now_ms = millis();
    if (now_ms - prev_ms > 250) {
        bitcount = 0;
        incoming = 0;
    }
    prev_ms = now_ms;
    const uint8_t n = bitcount - 1;
    if (n <= 7) {
        incoming |= (val << n);
    }
    bitcount++;
    if (bitcount == 11) {
        uint8_t i = g_head + 1;
        if (i >= BUFFER_SIZE)
            i = 0;
        if (i != g_tail) {
            g_buffer[i] = incoming;
            g_head = i;
        } else {
            g_buffer_overflow = true;
        }
        bitcount = 0;
        incoming = 0;
    }
}

PS2Keyboard::PS2Keyboard() = default;

uint8_t PS2Keyboard::read(bool *avail, bool *buffer_overflow)
{
    *avail = false;
    *buffer_overflow = g_buffer_overflow;
    g_buffer_overflow = false;

    uint8_t i = g_tail;
    if (i == g_head)
        return 0;
    i++;
    if (i >= BUFFER_SIZE)
        i = 0;
    const uint8_t c = g_buffer[i];
    g_tail = i;
    *avail = true;
    return c;
}

void PS2Keyboard::begin(const int clk_pin, const int data_pin)
{
    g_clk_pin = clk_pin;
    g_data_pin = data_pin;

    // initialize the keyboard
    ps2_pull_high(clk_pin);
    ps2_pull_high(data_pin);
    delay(20);
    if (ps2_write_byte_with_timeout(clk_pin, data_pin, 0xff, PS2_TIMEOUT)) return;  // send reset
    //ps2_write_byte(clk_pin, data_pin, 0xff);  // send reset

    ps2_read_byte(clk_pin, data_pin);         // read ack
    delayMicroseconds(100);
    ps2_pull_high(clk_pin);
    ps2_pull_high(data_pin);

    // attach the interrupt handler
    g_head = 0;
    g_tail = 0;
    g_buffer_overflow = false;
    attachInterrupt(digitalPinToInterrupt(clk_pin), clk_interrupt, FALLING);
}

void PS2Keyboard::set_caps_lock_led(const bool caps_lock_led)
{
    // Send LED change command.
    ps2_write_byte(g_clk_pin, g_data_pin, 0xED);
    delayMicroseconds(100);
    // Read ack
    ps2_read_byte(g_clk_pin, g_data_pin);
    delayMicroseconds(100);
    // Send new caps lock state
    ps2_write_byte(g_clk_pin, g_data_pin, caps_lock_led ? 0x4 : 0);
    delayMicroseconds(100);
    // Read ack.
    ps2_read_byte(g_clk_pin, g_data_pin);
    delayMicroseconds(100);
}
