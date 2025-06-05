/*
 * Scan code map for Atari ST.
 * Copyright (C) 2025 Rob Gowin
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <avr/pgmspace.h>

const char st_make_code_map[] PROGMEM = {
    0, //
    67, // F9
    0, //
    63, // F5
    61, // F3
    59, // F1
    60, // F2
    97, // F12
    0, //
    68, // F10
    66, // F8
    64, // F6
    62, // F4
    15, // Tab
    41, // Backtick/Tilde (`~)
    0, //
    0, //
    56, // Left Alt
    42, // Left Shift
    0, //
    29, // Left Ctrl
    16, // Q
    2, // 1
    0, //
    0, //
    0, //
    44, // Z
    31, // S
    30, // A
    17, // W
    3, // 2
    0, //
    0, //
    46, // C
    45, // X
    32, // D
    18, // E
    5, // 4
    4, // 3
    0, //
    0, //
    57, // Space
    47, // V
    33, // F
    20, // T
    19, // R
    6, // 5
    0, //
    0, //
    49, // N
    48, // B
    35, // H
    34, // G
    21, // Y
    7, // 6
    0, //
    0, //
    0, //
    50, // M
    36, // J
    22, // U
    8, // 7
    9, // 8
    0, //
    0, //
    51, // Comma (,<)
    37, // K
    23, // I
    24, // O
    11, // 0
    10, // 9
    0, //
    0, //
    52, // Period (.>)
    53, // Slash (/?)
    38, // L
    39, // Semicolon (;:)
    25, // P
    12, // Minus (-_)
    0, //
    0, //
    0, //
    40, // Apostrophe ('")
    0, //
    26, // Left Bracket ([{)
    13, // Equals (=+)
    0, //
    0, //
    58, // CapsLock
    54, // Right Shift
    28, // Enter
    27, // Right Bracket (]})
    0, //
    43, // Backslash (\|)
    0, //
    0, //
    0, //
    96, // UK \| between left shift and Z
    0, //
    0, //
    0, //
    0, //
    14, // Backspace
    0, //
    0, //
    109, // Keypad 1/End
    0, //
    106, // Keypad 4/Left
    103, // Keypad 7/Home
    0, //
    0, //
    0, //
    112, // Keypad 0/Ins
    113, // Keypad ./Del
    110, // Keypad 2/Down
    107, // Keypad 5
    108, // Keypad 6/Right
    104, // Keypad 8/Up
    1, // Escape
    -1, // NumLock
    98, // F11
    78, // Keypad +
    111, // Keypad 3/PgDn
    74, // Keypad -
    102, // Keypad *
    105, // Keypad 9/PgUp
    -1, // ScrollLock
    0, //
    0, //
    0, //
    0, //
    65, // F7
};

const char st_extended_make_code_map[] PROGMEM = {
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    56, // Right Alt
    0, //
    0, //
    29, // Right Ctrl
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    -1, // Left GUI (Windows)
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    -1, // Right GUI (Windows)
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    -1, // Menu
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    101, // Keypad /
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    114, // Keypad Enter
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    0, //
    79, // End
    0, //
    75, // Left Arrow
    71, // Home
    0, //
    0, //
    0, //
    82, // Insert
    83, // Delete
    80, // Down Arrow
    0, //
    77, // Right Arrow
    72, // Up Arrow
    0, //
    0, //
    0, //
    0, //
    81, // Page Down
    0, //
    0, //
    73, // Page Up
    0, //
    0, //
};
const char string_00[] PROGMEM = "";
const char string_01[] PROGMEM = "0";
const char string_02[] PROGMEM = "1";
const char string_03[] PROGMEM = "2";
const char string_04[] PROGMEM = "3";
const char string_05[] PROGMEM = "4";
const char string_06[] PROGMEM = "5";
const char string_07[] PROGMEM = "6";
const char string_08[] PROGMEM = "7";
const char string_09[] PROGMEM = "8";
const char string_10[] PROGMEM = "9";
const char string_11[] PROGMEM = "A";
const char string_12[] PROGMEM = "Apostrophe ('\")";
const char string_13[] PROGMEM = "B";
const char string_14[] PROGMEM = "Backslash (\\|)";
const char string_15[] PROGMEM = "Backspace";
const char string_16[] PROGMEM = "Backtick/Tilde (`~)";
const char string_17[] PROGMEM = "C";
const char string_18[] PROGMEM = "CapsLock";
const char string_19[] PROGMEM = "Comma (,<)";
const char string_20[] PROGMEM = "D";
const char string_21[] PROGMEM = "E";
const char string_22[] PROGMEM = "Enter";
const char string_23[] PROGMEM = "Equals (=+)";
const char string_24[] PROGMEM = "Escape";
const char string_25[] PROGMEM = "F";
const char string_26[] PROGMEM = "F1";
const char string_27[] PROGMEM = "F10";
const char string_28[] PROGMEM = "F11";
const char string_29[] PROGMEM = "F12";
const char string_30[] PROGMEM = "F2";
const char string_31[] PROGMEM = "F3";
const char string_32[] PROGMEM = "F4";
const char string_33[] PROGMEM = "F5";
const char string_34[] PROGMEM = "F6";
const char string_35[] PROGMEM = "F7";
const char string_36[] PROGMEM = "F8";
const char string_37[] PROGMEM = "F9";
const char string_38[] PROGMEM = "G";
const char string_39[] PROGMEM = "H";
const char string_40[] PROGMEM = "I";
const char string_41[] PROGMEM = "J";
const char string_42[] PROGMEM = "K";
const char string_43[] PROGMEM = "Keypad *";
const char string_44[] PROGMEM = "Keypad +";
const char string_45[] PROGMEM = "Keypad -";
const char string_46[] PROGMEM = "Keypad ./Del";
const char string_47[] PROGMEM = "Keypad 0/Ins";
const char string_48[] PROGMEM = "Keypad 1/End";
const char string_49[] PROGMEM = "Keypad 2/Down";
const char string_50[] PROGMEM = "Keypad 3/PgDn";
const char string_51[] PROGMEM = "Keypad 4/Left";
const char string_52[] PROGMEM = "Keypad 5";
const char string_53[] PROGMEM = "Keypad 6/Right";
const char string_54[] PROGMEM = "Keypad 7/Home";
const char string_55[] PROGMEM = "Keypad 8/Up";
const char string_56[] PROGMEM = "Keypad 9/PgUp";
const char string_57[] PROGMEM = "L";
const char string_58[] PROGMEM = "Left Alt";
const char string_59[] PROGMEM = "Left Bracket ([{)";
const char string_60[] PROGMEM = "Left Ctrl";
const char string_61[] PROGMEM = "Left Shift";
const char string_62[] PROGMEM = "M";
const char string_63[] PROGMEM = "Minus (-_)";
const char string_64[] PROGMEM = "N";
const char string_65[] PROGMEM = "NumLock";
const char string_66[] PROGMEM = "O";
const char string_67[] PROGMEM = "P";
const char string_68[] PROGMEM = "Period (.>)";
const char string_69[] PROGMEM = "Q";
const char string_70[] PROGMEM = "R";
const char string_71[] PROGMEM = "Right Bracket (]})";
const char string_72[] PROGMEM = "Right Shift";
const char string_73[] PROGMEM = "S";
const char string_74[] PROGMEM = "ScrollLock";
const char string_75[] PROGMEM = "Semicolon (;:)";
const char string_76[] PROGMEM = "Slash (/?)";
const char string_77[] PROGMEM = "Space";
const char string_78[] PROGMEM = "T";
const char string_79[] PROGMEM = "Tab";
const char string_80[] PROGMEM = "U";
const char string_81[] PROGMEM = "V";
const char string_82[] PROGMEM = "W";
const char string_83[] PROGMEM = "X";
const char string_84[] PROGMEM = "Y";
const char string_85[] PROGMEM = "Z";

PGM_P const ps2_make_code_map_flash[] PROGMEM = {
    string_00, //
    string_37, // F9
    string_00, //
    string_33, // F5
    string_31, // F3
    string_26, // F1
    string_30, // F2
    string_29, // F12
    string_00, //
    string_27, // F10
    string_36, // F8
    string_34, // F6
    string_32, // F4
    string_79, // Tab
    string_16, // Backtick/Tilde (`~)
    string_00, //
    string_00, //
    string_58, // Left Alt
    string_61, // Left Shift
    string_00, //
    string_60, // Left Ctrl
    string_69, // Q
    string_02, // 1
    string_00, //
    string_00, //
    string_00, //
    string_85, // Z
    string_73, // S
    string_11, // A
    string_82, // W
    string_03, // 2
    string_00, //
    string_00, //
    string_17, // C
    string_83, // X
    string_20, // D
    string_21, // E
    string_05, // 4
    string_04, // 3
    string_00, //
    string_00, //
    string_77, // Space
    string_81, // V
    string_25, // F
    string_78, // T
    string_70, // R
    string_06, // 5
    string_00, //
    string_00, //
    string_64, // N
    string_13, // B
    string_39, // H
    string_38, // G
    string_84, // Y
    string_07, // 6
    string_00, //
    string_00, //
    string_00, //
    string_62, // M
    string_41, // J
    string_80, // U
    string_08, // 7
    string_09, // 8
    string_00, //
    string_00, //
    string_19, // Comma (,<)
    string_42, // K
    string_40, // I
    string_66, // O
    string_01, // 0
    string_10, // 9
    string_00, //
    string_00, //
    string_68, // Period (.>)
    string_76, // Slash (/?)
    string_57, // L
    string_75, // Semicolon (;:)
    string_67, // P
    string_63, // Minus (-_)
    string_00, //
    string_00, //
    string_00, //
    string_12, // Apostrophe ('\")
    string_00, //
    string_59, // Left Bracket ([{)
    string_23, // Equals (=+)
    string_00, //
    string_00, //
    string_18, // CapsLock
    string_72, // Right Shift
    string_22, // Enter
    string_71, // Right Bracket (]})
    string_00, //
    string_14, // Backslash (\\|)
    string_00, //
    string_00, //
    string_00, //
    string_00, //
    string_00, //
    string_00, //
    string_00, //
    string_00, //
    string_15, // Backspace
    string_00, //
    string_00, //
    string_48, // Keypad 1/End
    string_00, //
    string_51, // Keypad 4/Left
    string_54, // Keypad 7/Home
    string_00, //
    string_00, //
    string_00, //
    string_47, // Keypad 0/Ins
    string_46, // Keypad ./Del
    string_49, // Keypad 2/Down
    string_52, // Keypad 5
    string_53, // Keypad 6/Right
    string_55, // Keypad 8/Up
    string_24, // Escape
    string_65, // NumLock
    string_28, // F11
    string_44, // Keypad +
    string_50, // Keypad 3/PgDn
    string_45, // Keypad -
    string_43, // Keypad *
    string_56, // Keypad 9/PgUp
    string_74, // ScrollLock
    string_00, //
    string_00, //
    string_00, //
    string_00, //
    string_35, // F7
};
const char ext_string_00[] PROGMEM = "";
const char ext_string_01[] PROGMEM = "Delete";
const char ext_string_02[] PROGMEM = "Down Arrow";
const char ext_string_03[] PROGMEM = "End";
const char ext_string_04[] PROGMEM = "Home";
const char ext_string_05[] PROGMEM = "Insert";
const char ext_string_06[] PROGMEM = "Keypad /";
const char ext_string_07[] PROGMEM = "Keypad Enter";
const char ext_string_08[] PROGMEM = "Left Arrow";
const char ext_string_09[] PROGMEM = "Left GUI (Windows)";
const char ext_string_10[] PROGMEM = "Menu";
const char ext_string_11[] PROGMEM = "Page Down";
const char ext_string_12[] PROGMEM = "Page Up";
const char ext_string_13[] PROGMEM = "Right Alt";
const char ext_string_14[] PROGMEM = "Right Arrow";
const char ext_string_15[] PROGMEM = "Right Ctrl";
const char ext_string_16[] PROGMEM = "Right GUI (Windows)";
const char ext_string_17[] PROGMEM = "Up Arrow";

PGM_P const extended_ps2_make_code_map_flash[] PROGMEM = {
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_13, // Right Alt
    ext_string_00, //
    ext_string_00, //
    ext_string_15, // Right Ctrl
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_09, // Left GUI (Windows)
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_16, // Right GUI (Windows)
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_10, // Menu
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_06, // Keypad /
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_07, // Keypad Enter
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_03, // End
    ext_string_00, //
    ext_string_08, // Left Arrow
    ext_string_04, // Home
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_05, // Insert
    ext_string_01, // Delete
    ext_string_02, // Down Arrow
    ext_string_00, //
    ext_string_14, // Right Arrow
    ext_string_17, // Up Arrow
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_00, //
    ext_string_11, // Page Down
    ext_string_00, //
    ext_string_00, //
    ext_string_12, // Page Up
    ext_string_00, //
    ext_string_00, //
};
