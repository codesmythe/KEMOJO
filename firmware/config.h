// config.h
// Copyright (c) 2023-2024 Daniel Cliche
// SPDX-License-Identifier: MIT

#ifndef __CONFIG_H__
#define __CONFIG_H__

// Serial port
#define SERIAL_BAUD_RATE 9600

#define KEYBOARD_ENA 1
#define MOUSE_ENA 1

// Pinout
#define PS2_MOUSE_CLK_PIN 2      // ATMEGA PIN 4
#define PS2_KEYBOARD_CLK_PIN 3   // ATMEGA PIN 5
#define PS2_MOUSE_DATA_PIN 4     // ATMEGA PIN 6
#define PS2_KEYBOARD_DATA_PIN 5  // ATMEGA PIN 11

#define PS2_TIMEOUT 2000

#define DEBUG 0

#endif
