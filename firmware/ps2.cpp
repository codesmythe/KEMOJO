// ps2.cpp
// Copyright (c) 2023 Daniel Cliche
// SPDX-License-Identifier: MIT

#include "ps2.h"
#include "Arduino.h"

void ps2_pull_low(const int pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void ps2_pull_high(const int pin)
{
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
}

static void wait_for_pin_low(const int pin)
{
    // Just spin while the pin is high
    while (digitalRead(pin))
        ;
}

static int wait_for_pin_low_with_timeout(const int pin, const unsigned long timeout)
{
    int timed_out = 1;
    const unsigned long end = millis() + timeout;
    while (millis() < end) {
        if (!digitalRead(pin)) {
            timed_out = 0;
            break;
        }
    }
    return timed_out;
}

static void wait_for_pin_high(const int pin)
{
    // Just spin while the pin is low
    while (!digitalRead(pin))
        ;
}

static void wait_for_either_pin_high(const int pin1, const int pin2)
{
    while (!digitalRead(pin1) || !digitalRead(pin2))
        ;
}

static int wait_for_either_pin_high_with_timeout(const int pin1, const int pin2, const unsigned long timeout)
{
    int timed_out = 1;
    const unsigned long end = millis() + timeout;
    while (millis() < end) {
        if (digitalRead(pin1) || digitalRead(pin2)) {
            timed_out = 0;
            break;
        }
    }
    return timed_out;
}

static int wait_for_pin_high_with_timeout(const int pin, const unsigned long timeout)
{
    int timed_out = 1;
    const unsigned long end = millis() + timeout;
    while (millis() < end) {
        if (digitalRead(pin)) {
            timed_out = 0;
            break;
        }
    }
    return timed_out;
}

static int ps2_read_bit(const int clk_pin, const int data_pin)
{
    wait_for_pin_low(clk_pin);
    const int bit = digitalRead(data_pin);
    wait_for_pin_high(clk_pin);
    return bit;
}

uint8_t ps2_read_byte(const int clk_pin, const int data_pin)
{
    uint8_t data = 0;
    ps2_pull_high(clk_pin);
    ps2_pull_high(data_pin);
    delayMicroseconds(50);

    wait_for_pin_low(clk_pin);
    delayMicroseconds(5);
    wait_for_pin_high(clk_pin);

    for (int i = 0; i < 8; i++)
        bitWrite(data, i, ps2_read_bit(clk_pin, data_pin));
    ps2_read_bit(clk_pin, data_pin);         // parity bit
    ps2_read_bit(clk_pin, data_pin);         // stop bit
    ps2_pull_low(clk_pin);
    return data;
}

void ps2_write_byte(int clk_pin, int data_pin, uint8_t data)
{
    char i;
    char parity = 1;
    ps2_pull_high(data_pin);
    ps2_pull_high(clk_pin);
    delayMicroseconds(300);
    ps2_pull_low(clk_pin);
    delayMicroseconds(300);
    ps2_pull_low(data_pin);
    delayMicroseconds(10);
    ps2_pull_high(clk_pin);             // start bit
    wait_for_pin_low(clk_pin);       // wait for mouse to take control of clock
    // clock is low, and we are clear to send data
    for (i = 0; i < 8; i++) {
        if (data & 0x01) {
            ps2_pull_high(data_pin);
        } else {
            ps2_pull_low(data_pin);
        }
        wait_for_pin_high(clk_pin);
        wait_for_pin_low(clk_pin);
        parity = parity ^ (data & 0x01);
        data >>= 1;
    }
    // parity
    if (parity) {
        ps2_pull_high(data_pin);
    } else {
        ps2_pull_low(data_pin);
    }
    wait_for_pin_high(clk_pin);
    wait_for_pin_low(clk_pin);
    ps2_pull_high(data_pin);
    delayMicroseconds(50);
    wait_for_pin_low(clk_pin);
    wait_for_either_pin_high(clk_pin, data_pin); // wait for mouse to switch modes
    ps2_pull_low(clk_pin);                       // put a hold on the incoming data
}

int ps2_write_byte_with_timeout(const int clk_pin, const int data_pin, uint8_t data, const unsigned long timeout)
{
    char i;
    char parity = 1;
    ps2_pull_high(data_pin);
    ps2_pull_high(clk_pin);
    delayMicroseconds(300);
    ps2_pull_low(clk_pin);
    delayMicroseconds(300);
    ps2_pull_low(data_pin);
    delayMicroseconds(10);
    ps2_pull_high(clk_pin);             // start bit
    if (wait_for_pin_low_with_timeout(clk_pin, timeout)) return 1; // wait for mouse to take control of clock
    // clock is low, and we are clear to send data
    for (i = 0; i < 8; i++) {
        if (data & 0x01) {
            ps2_pull_high(data_pin);
        } else {
            ps2_pull_low(data_pin);
        }
        if (wait_for_pin_high_with_timeout(clk_pin, timeout)) return 1;
        if (wait_for_pin_low_with_timeout(clk_pin, timeout)) return 1;
        parity = parity ^ (data & 0x01);
        data >>= 1;
    }
    // parity
    if (parity) {
        ps2_pull_high(data_pin);
    } else {
        ps2_pull_low(data_pin);
    }
    if (wait_for_pin_high_with_timeout(clk_pin, timeout)) return 1;
    if (wait_for_pin_low_with_timeout(clk_pin, timeout)) return 1;
    ps2_pull_high(data_pin);
    delayMicroseconds(50);
    if (wait_for_pin_low_with_timeout(clk_pin, timeout)) return 1;
    if (wait_for_either_pin_high_with_timeout(clk_pin, data_pin, timeout)) return 1; // wait for mouse to switch modes
    ps2_pull_low(clk_pin);                       // put a hold on the incoming data
    return 0; // Did not timeout.
}
