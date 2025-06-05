/*
  Hatari - joy.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef HATARI_JOY_H
#define HATARI_JOY_H

enum {
    JOYSTICK_SPACE_NULL,          /* Not up/down */
    JOYSTICK_SPACE_DOWN,
    JOYSTICK_SPACE_DOWNED,
    JOYSTICK_SPACE_UP
};

#define JOYRANGE_UP_VALUE     -16384     /* Joystick ranges in XY */
#define JOYRANGE_DOWN_VALUE    16383
#define JOYRANGE_LEFT_VALUE   -16384
#define JOYRANGE_RIGHT_VALUE   16383

#define ATARIJOY_BITMASK_UP    0x01
#define ATARIJOY_BITMASK_DOWN  0x02
#define ATARIJOY_BITMASK_LEFT  0x04
#define ATARIJOY_BITMASK_RIGHT 0x08
#define ATARIJOY_BITMASK_FIRE  0x80

#endif /* ifndef HATARI_JOY_H */
