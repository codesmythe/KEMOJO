/*
  Hatari - ikbd.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef HATARI_IKBD_H
#define HATARI_IKBD_H

#include <stdbool.h>
#include <stdio.h>

/* Keyboard processor details */

typedef struct {
    int X,Y;                        /* Position of mouse */
    int MaxX,MaxY;                  /* Max limits of mouse */
    uint8_t PrevReadAbsMouseButtons;  /* Previous button mask for 'IKBD_Cmd_ReadAbsMousePos' */
} ABS_MOUSE;

typedef struct {
    int dx, dy;                     /* Mouse delta to be added */
    int DeltaX,DeltaY;              /* Final XY mouse position delta */
    int XScale,YScale;              /* Scale of mouse */
    int XThreshold,YThreshold;      /* Threshold */
    uint8_t KeyCodeDeltaX,KeyCodeDeltaY;    /* Delta X,Y for mouse keycode mode */
    int YAxis;                      /* Y-Axis direction */
    uint8_t Action;                   /* Bit 0-Report abs position of press, Bit 1-Report abs on release */
} MOUSE;

typedef struct {
    uint8_t JoyData[2];               /* Joystick details */
    uint8_t PrevJoyData[2];           /* Previous joystick details, used to check for 'IKBD_SendAutoJoysticks' */
} JOY;

typedef struct {
    ABS_MOUSE  Abs;
    MOUSE    Mouse;
    JOY      Joy;
    int MouseMode;                  /* AUTOMODE_xxxx */
    int JoystickMode;               /* AUTOMODE_xxxx */
} KEYBOARD_PROCESSOR;

/* Keyboard state */
#define KBD_MAX_SCANCODE          0x72
#define SIZE_KEYBOARD_BUFFER      512    /* Allow this many bytes to be stored in buffer (waiting to send to ACIA) */
#define KEYBOARD_BUFFER_MASK      (SIZE_KEYBOARD_BUFFER-1)
#define SIZE_KEYBOARDINPUT_BUFFER 8
typedef struct {
    uint8_t KeyStates[KBD_MAX_SCANCODE + 1];	/* State of ST keys, TRUE is down */

    uint8_t Buffer[SIZE_KEYBOARD_BUFFER];		/* Keyboard output buffer */
    int BufferHead,BufferTail;			/* Pointers into above buffer */
    int NbBytesInOutputBuffer;			/* Number of bytes in output buffer */
    bool PauseOutput;				/* If true, don't send bytes anymore (see command 0x13) */

    uint8_t InputBuffer[SIZE_KEYBOARDINPUT_BUFFER];	/* Buffer for data send from CPU to keyboard processor (commands) */
    int nBytesInInputBuffer;			/* Number of command bytes in above buffer */

    int bLButtonDown,bRButtonDown;                /* Mouse states in emulation system, BUTTON_xxxx */
    int bOldLButtonDown,bOldRButtonDown;
    int LButtonDblClk,RButtonDblClk;
    unsigned int LButtonHistory, RButtonHistory;

    int AutoSendCycles;				/* Number of cpu cycles to call INTERRUPT_IKBD_AUTOSEND */
} KEYBOARD;

/* Button states, a bit mask so can mimic joystick/right mouse button duplication */
#define BUTTON_NULL      0x00     /* Button states, so can OR together mouse/joystick buttons */
#define BUTTON_MOUSE     0x01
#define BUTTON_JOYSTICK  0x02

/* Mouse/Joystick modes */
#define AUTOMODE_OFF			0
#define AUTOMODE_MOUSEREL		1
#define AUTOMODE_MOUSEABS		2
#define AUTOMODE_MOUSECURSOR		3
#define AUTOMODE_JOYSTICK		4
#define AUTOMODE_JOYSTICK_MONITORING	5

enum {
    JOYID_JOYSTICK0,
    JOYID_JOYSTICK1,
    JOYID_JOYPADA,
    JOYID_JOYPADB,
    JOYID_PARPORT1,
    JOYID_PARPORT2,
    JOYSTICK_COUNT
};

/* 0xfffc00 (read status from ACIA) */
#define ACIA_STATUS_REGISTER__RX_BUFFER_FULL  0x01
#define ACIA_STATUS_REGISTER__TX_BUFFER_EMPTY  0x02
#define ACIA_STATUS_REGISTER__OVERRUN_ERROR    0x20
#define ACIA_STATUS_REGISTER__INTERRUPT_REQUEST  0x80

extern KEYBOARD_PROCESSOR KeyboardProcessor;
extern KEYBOARD Keyboard;

#ifdef __cplusplus
extern "C" {
#endif
extern void IKBD_Reset(bool bCold);

extern void IKBD_MemorySnapShot_Capture(bool bSave);

//extern void IKBD_InterruptHandler_ResetTimer();
extern void IKBD_InterruptHandler_AutoSend();

extern void IKBD_UpdateClockOnVBL();

extern void IKBD_PressSTKey(uint8_t ScanCode, bool bPress);

extern void IKBD_Info(FILE *fp, uint32_t dummy);

extern void IKBD_RunKeyboardCommand(uint8_t aciabyte);

extern void IKBD_SendAutoKeyboardCommands(void);

#define ATARIJOY_BITMASK_UP    0x01
#define ATARIJOY_BITMASK_DOWN  0x02
#define ATARIJOY_BITMASK_LEFT  0x04
#define ATARIJOY_BITMASK_RIGHT 0x08
#define ATARIJOY_BITMASK_FIRE  0x80

#define GPIO_MASK_UP    0x20
#define GPIO_MASK_DOWN  0x08
#define GPIO_MASK_LEFT  0x02
#define GPIO_MASK_RIGHT 0x01
#define GPIO_MASK_FIRE  0x10


/*-----------------------------------------------------------------------*/
/**
 * Read PC joystick and return ST format byte, i.e. lower 4 bits direction
 * and top bit fire.
 * NOTE : ID 0 is Joystick 0/Mouse and ID 1 is Joystick 1 (default),
 *        ID 2 and 3 are STE joypads and ID 4 and 5 are parport joysticks.
 */

extern uint8_t Joy_GetStickData(int nStJoyId);
#ifdef __cplusplus
}
#endif

#endif  /* HATARI_IKBD_H */
