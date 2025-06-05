/* Derived from: */
/*
  Hatari - ikbd.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  AVR port Copyright (c) 2025 Rob Gowin
  SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <stdint.h>
#include "ikbd.h"
#include "joy.h"
#include <Arduino.h>
#include <string.h>

#define TRACE_IKBD_CMDS 1
#define TRACE_IKBD_ACIA 2
#define TRACE_IKBD_ALL (TRACE_IKBD_CMDS | TRACE_IKBD_ACIA)

#define ACIA_CYCLES    7200         /* Cycles (Multiple of 4) between sent to ACIA from keyboard along serial line - 500Hz/64, (approx' 6920-7200cycles from test program) */
#define ABS_X_ONRESET    0          /* Initial XY for absolute mouse position after RESET command */
#define ABS_Y_ONRESET    0
#define ABS_MAX_X_ONRESET  320      /* Initial absolute mouse limits after RESET command */
#define ABS_MAY_Y_ONRESET  200      /* These values are never actually used as user MUST call 'IKBD_Cmd_AbsMouseMode' before ever using them */
#define ABS_PREVBUTTONS  (0x02|0x8) /* Don't report any buttons up on first call to 'IKBD_Cmd_ReadAbsMousePos' */

#define IKBD_ROM_VERSION        0xF1    /* On reset, the IKBD will return either 0xF0 or 0xF1, depending on the IKBD's ROM */
/* version. Only very early ST returned 0xF0, so we use 0xF1 which is the most common case.*/
/* Beside, some programs explicitly wait for 0xF1 after a reset (Dragonnels demo) */

#define CALL_VAR(func)  { ((void(*)(void))func)(); }

static bool bMouseDisabled, bJoystickDisabled;
static bool bDuringResetCriticalTime, bBothMouseAndJoy;
static bool bMouseEnabledDuringReset;


static void LOG_TRACE(int level, ...)
{
    // Nothing right now.
}

typedef struct {
    /* Date/Time is stored in the IKBD using 6 bytes in BCD format */
    /* Clock is cleared on cold reset, but keeps its values on warm reset */
    /* Original RAM location :  $82=year $83=month $84=day $85=hour $86=minute $87=second */
    uint8_t         Clock[ 6 ];
    int64_t         Clock_micro;                            /* Incremented every VBL to update Clock[] every second */

} IKBD_STRUCT;

static IKBD_STRUCT      IKBD;
static IKBD_STRUCT      *pIKBD = &IKBD;

static void     IKBD_Boot_ROM ( bool ClearAllRAM );
static bool     IKBD_OutputBuffer_CheckFreeCount ( int Nb );
static int		IKBD_Delay_Random ( int min, int max );

static void     IKBD_Cmd_Return_Byte ( uint8_t Data );
static void     IKBD_Cmd_Return_Byte_Delay ( uint8_t Data, int Delay_Cycles );
static void		IKBD_Send_Byte_Delay ( uint8_t Data, int Delay_Cycles );

static uint8_t  ScanCodeState[ 128 ];                   /* state of each key : 0=released 1=pressed */


/* List of possible keyboard commands, others are seen as NOPs by keyboard processor */
static void IKBD_Cmd_Reset();
static void IKBD_Cmd_MouseAction();
static void IKBD_Cmd_RelMouseMode();
static void IKBD_Cmd_AbsMouseMode();
static void IKBD_Cmd_MouseCursorKeycodes();
static void IKBD_Cmd_SetMouseThreshold();
static void IKBD_Cmd_SetMouseScale();
static void IKBD_Cmd_ReadAbsMousePos();
static void IKBD_Cmd_SetInternalMousePos();
static void IKBD_Cmd_SetYAxisDown();
static void IKBD_Cmd_SetYAxisUp();
static void IKBD_Cmd_StartKeyboardTransfer();
static void IKBD_Cmd_TurnMouseOff();
static void IKBD_Cmd_StopKeyboardTransfer();
static void IKBD_Cmd_ReturnJoystickAuto();
static void IKBD_Cmd_StopJoystick();
static void IKBD_Cmd_ReturnJoystick();
static void IKBD_Cmd_SetJoystickMonitoring(void);
static void IKBD_Cmd_SetJoystickFireDuration(void);
static void IKBD_Cmd_SetCursorForJoystick(void);
static void IKBD_Cmd_DisableJoysticks(void);
static void IKBD_Cmd_ReportMouseAction(void);
static void IKBD_Cmd_ReportMouseMode(void);
static void IKBD_Cmd_ReportMouseThreshold(void);
static void IKBD_Cmd_ReportMouseScale(void);
static void IKBD_Cmd_ReportMouseVertical(void);
static void IKBD_Cmd_ReportMouseAvailability(void);
static void IKBD_Cmd_ReportJoystickMode(void);
static void IKBD_Cmd_ReportJoystickAvailability(void);

/* Keyboard Command */
static const struct {
    uint8_t Command;
    uint8_t NumParameters;
    void (*pCallFunction)();
} KeyboardCommands[] = {
    /* Known messages, counts include command byte */
    { 0x80,2,  IKBD_Cmd_Reset },
    { 0x07,2,  IKBD_Cmd_MouseAction },
    { 0x08,1,  IKBD_Cmd_RelMouseMode },
    { 0x09,5,  IKBD_Cmd_AbsMouseMode },
    { 0x0A,3,  IKBD_Cmd_MouseCursorKeycodes },
    { 0x0B,3,  IKBD_Cmd_SetMouseThreshold },
    { 0x0C,3,  IKBD_Cmd_SetMouseScale },
    { 0x0D,1,  IKBD_Cmd_ReadAbsMousePos },
    { 0x0E,6,  IKBD_Cmd_SetInternalMousePos },
    { 0x0F,1,  IKBD_Cmd_SetYAxisDown },
    { 0x10,1,  IKBD_Cmd_SetYAxisUp },
    { 0x11,1,  IKBD_Cmd_StartKeyboardTransfer },
    { 0x12,1,  IKBD_Cmd_TurnMouseOff },
    { 0x13,1,  IKBD_Cmd_StopKeyboardTransfer },
    { 0x14,1,  IKBD_Cmd_ReturnJoystickAuto },
    { 0x15,1,  IKBD_Cmd_StopJoystick },
    { 0x16,1,  IKBD_Cmd_ReturnJoystick },
    { 0x17,2,  IKBD_Cmd_SetJoystickMonitoring },
    { 0x18,1,  IKBD_Cmd_SetJoystickFireDuration },
    { 0x19,7,  IKBD_Cmd_SetCursorForJoystick },
    { 0x1A,1,  IKBD_Cmd_DisableJoysticks },

    /* Report message (top bit set) */
    {0x87, 1, IKBD_Cmd_ReportMouseAction},
    {0x88, 1, IKBD_Cmd_ReportMouseMode},
    {0x89, 1, IKBD_Cmd_ReportMouseMode},
    {0x8A, 1, IKBD_Cmd_ReportMouseMode},
    {0x8B, 1, IKBD_Cmd_ReportMouseThreshold},
    {0x8C, 1, IKBD_Cmd_ReportMouseScale},
    {0x8F, 1, IKBD_Cmd_ReportMouseVertical},
    {0x90, 1, IKBD_Cmd_ReportMouseVertical},
    {0x92, 1, IKBD_Cmd_ReportMouseAvailability},
    {0x94, 1, IKBD_Cmd_ReportJoystickMode},
    {0x95, 1, IKBD_Cmd_ReportJoystickMode},
    {0x99, 1, IKBD_Cmd_ReportJoystickMode},
    {0x9A, 1, IKBD_Cmd_ReportJoystickAvailability},

    {0xFF, 0, NULL} /* Term */

};

/*-----------------------------------------------------------------------*/
/**
 * Init the IKBD processor.
 * Connect the IKBD RX/TX callback functions to the ACIA.
 * This is called only once, when the emulator starts.
 */
void	IKBD_Init ( void )
{
    LOG_TRACE ( TRACE_IKBD_ALL, "ikbd init\n" );
}

/* This function is called after a hardware reset of the IKBD.
 * Cold reset is when the computer is turned off/on.
 * Warm reset is when the reset button is pressed or the 68000
 * RESET instruction is used.
 * We clear the serial interface and we execute the function
 * that emulates booting the ROM at 0xF000.
 */
void    IKBD_Reset ( bool bCold )
{
    /* On cold reset, clear the whole RAM (including clock data) */
    /* On warm reset, the clock data should be kept */
    if ( bCold )
        IKBD_Boot_ROM ( true );
    else
        IKBD_Boot_ROM ( false );
}

static void SetupTimer1OneShot()
{
    // Configure Timer1 to interrupt in 1/16 of a second.
    noInterrupts();           // Disable interrupts during setup

    // Clear Timer1 control registers
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;               // Initialize counter

    // Set compare match register for 1Hz increments
    // 16MHz Arduino clock / 256 prescaler / 16Hz = 3906
    // 7372800 Hz Arduino clock / 256 prescaral / 16 Hz =
    /* OCR1A = 3906; *//* Perhaps should calc based on IKBD_RESET_CYCLES */
    OCR1A = 1800;
    // Turn on CTC mode (Clear Timer on Compare Match)
    TCCR1B |= (1 << WGM12);

    // Set prescaler to 256
    TCCR1B |= (1 << CS12);

    // Enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

    interrupts();            // Enable interrupts
}

volatile bool ledState = false;

/* This function emulates the boot code stored in the ROM at address $F000.
 * This boot code is called either after a hardware reset, or when the
 * reset command ($80 $01) is received.
 * Depending on the conditions, we should clear the clock data or not (the
 * real IKBD will test+clear RAM either in range $80-$FF or in range $89-$FF)
 */
static void     IKBD_Boot_ROM ( bool ClearAllRAM )
{
    int     i;


    LOG_TRACE ( TRACE_IKBD_ALL, "ikbd boot rom clear_all=%s\n", ClearAllRAM?"yes":"no" );

    /* Clear clock data when the 128 bytes of RAM are cleared */
    if ( ClearAllRAM ) {
        /* Clear clock data on cold reset */
        for ( i=0 ; i<6 ; i++ )
            pIKBD->Clock[ i ] = 0;
        pIKBD->Clock_micro = 0;
    }

    /* Set default reporting mode for mouse/joysticks */
    KeyboardProcessor.MouseMode = AUTOMODE_MOUSEREL;
    KeyboardProcessor.JoystickMode = AUTOMODE_JOYSTICK;

    KeyboardProcessor.Abs.X = ABS_X_ONRESET;
    KeyboardProcessor.Abs.Y = ABS_Y_ONRESET;
    KeyboardProcessor.Abs.MaxX = ABS_MAX_X_ONRESET;
    KeyboardProcessor.Abs.MaxY = ABS_MAY_Y_ONRESET;
    KeyboardProcessor.Abs.PrevReadAbsMouseButtons = ABS_PREVBUTTONS;

    KeyboardProcessor.Mouse.DeltaX = KeyboardProcessor.Mouse.DeltaY = 0;
    KeyboardProcessor.Mouse.XScale = KeyboardProcessor.Mouse.YScale = 0;
    KeyboardProcessor.Mouse.XThreshold = KeyboardProcessor.Mouse.YThreshold = 1;
    KeyboardProcessor.Mouse.KeyCodeDeltaX = KeyboardProcessor.Mouse.KeyCodeDeltaY = 1;
    KeyboardProcessor.Mouse.YAxis = 1;          /* Y origin at top */
    KeyboardProcessor.Mouse.Action = 0;

    KeyboardProcessor.Joy.PrevJoyData[0] = KeyboardProcessor.Joy.PrevJoyData[1] = 0;

    for ( i=0 ; i<128 ; i++ )
        ScanCodeState[ i ] = 0;                         /* key is released */

    /* Reset our keyboard states and clear key state table */
    Keyboard.BufferHead = Keyboard.BufferTail = 0;
    Keyboard.NbBytesInOutputBuffer = 0;
    Keyboard.nBytesInInputBuffer = 0;
    Keyboard.PauseOutput = false;

    memset(Keyboard.KeyStates, 0, sizeof(Keyboard.KeyStates));
    Keyboard.bLButtonDown = BUTTON_NULL;
    Keyboard.bRButtonDown = BUTTON_NULL;
    Keyboard.bOldLButtonDown = Keyboard.bOldRButtonDown = BUTTON_NULL;
    Keyboard.LButtonDblClk = Keyboard.RButtonDblClk = 0;
    Keyboard.LButtonHistory = Keyboard.RButtonHistory = 0;

    /* Store bool for when disable mouse or joystick */
    bMouseDisabled = bJoystickDisabled = false;
    /* do emulate hardware 'quirk' where if disable both with 'x' time
     * of a RESET command they are ignored! */
    bDuringResetCriticalTime = true;
    bBothMouseAndJoy = false;
    bMouseEnabledDuringReset = false;

#if 0
    /* Remove any custom handlers used to emulate code loaded to the 6301's RAM */
    if ( ( MemoryLoadNbBytesLeft != 0 ) || ( IKBD_ExeMode == true ) ) {
        LOG_TRACE ( TRACE_IKBD_ALL, "ikbd stop memory load and turn off custom exe\n" );

        MemoryLoadNbBytesLeft = 0;
        pIKBD_CustomCodeHandler_Read = NULL;
        pIKBD_CustomCodeHandler_Write = NULL;
        IKBD_ExeMode = false;
    }
#endif

    /* During the boot, the IKBD will test all the keys to ensure no key */
    /* is stuck. We use a timer to emulate the time needed for this part */
    /* (eg Lotus Turbo Esprit 2 requires at least a delay of 50000 cycles */
    /* or it will crash during start up) */
    /* For debug, turn on the debug LED (set its pin low) during the reset period. */
    ledState = false;
    digitalWrite(PIN7, ledState);
    SetupTimer1OneShot();

#if 0
    /* Add auto-update function to the queue */
    /* We add it only if it was not active, else this can lead to unresponsive keyboard/input */
    /* when RESET instruction is called in a loop in less than 150000 cycles */
    Keyboard.AutoSendCycles = 150000;                               /* approx every VBL */
    if ( CycInt_InterruptActive ( INTERRUPT_IKBD_AUTOSEND ) == false )
        CycInt_AddRelativeInterrupt ( Keyboard.AutoSendCycles, INT_CPU8_CYCLE, INTERRUPT_IKBD_AUTOSEND );
#endif
    LOG_TRACE ( TRACE_IKBD_ALL, "ikbd reset done, starting reset timer\n" );
}

ISR(TIMER1_COMPA_vect)
{
    /* We are using the timer as a one shot, so turn off its interrupt. */
    //TIMSK1 &= ~(1 << OCIE1A); // Disable timer compare interrupt

    /* Reset timer is over */
    bDuringResetCriticalTime = false;
    bMouseEnabledDuringReset = false;

    /* Return $F1 when IKBD's boot is complete */
    IKBD_Cmd_Return_Byte_Delay ( IKBD_ROM_VERSION, IKBD_Delay_Random ( 0, 3000 ) );

    /*
     * Toggle the LED state, which should turn it off. If this interrupt handler is called multiple times,
     * the LED will blink.
     */
    ledState = !ledState;
    digitalWrite(PIN7, ledState);
    TIMSK1 &= ~(1 << OCIE1A);
    //digitalWrite(PIN7, true); // Pin high turns off the LED.
}

/*-----------------------------------------------------------------------*/
/**
 * Return true if the output buffer can store 'Nb' new bytes,
 * else return false.
 * Some games like 'Downfall' or 'Fokker' are continually issuing the same
 * IKBD_Cmd_ReturnJoystick command without waiting for the returned bytes,
 * which will fill the output buffer faster than the CPU can empty it.
 * In that case, new messages must be discarded until the buffer has some room
 * again for a whole packet.
 */
static bool     IKBD_OutputBuffer_CheckFreeCount ( int Nb )
{
    // fprintf ( stderr , "check %d %d head %d tail %d\n" , Nb , SIZE_KEYBOARD_BUFFER - Keyboard.NbBytesInOutputBuffer ,
    //       Keyboard.BufferHead , Keyboard.BufferTail );

    if ( SIZE_KEYBOARD_BUFFER - Keyboard.NbBytesInOutputBuffer >= Nb )
        return true;

    else {
        LOG_TRACE ( TRACE_IKBD_ACIA, "ikbd acia output buffer is full, can't send %d bytes\n",
                    Nb);
        return false;
    }
}



/* From lib/arduino/core/WMath.cpp */
long arduino_random(const long howsmall, const long howbig)
{
    if (howsmall >= howbig) return howsmall;
    return random() % howbig + howsmall;
}

/*-----------------------------------------------------------------------*/
/**
 * Return a random number between 'min' and 'max'.
 * This is used when the IKBD send bytes to the ACIA, to add some
 * randomness to the delay (on real hardware, the delay is not constant
 * when a command return some bytes).
 */
static int	IKBD_Delay_Random ( const int min, const int max )
{
    return (int)arduino_random(min, max);
}


/*-----------------------------------------------------------------------*/
/**
 * This function will buffer all the bytes returned by a specific
 * IKBD_Cmd_xxx command.
 */
static void     IKBD_Cmd_Return_Byte ( uint8_t Data )
{
    IKBD_Send_Byte_Delay ( Data, 0 );
}

/**
 * Same as IKBD_Cmd_Return_Byte, but with a delay before transmitting
 * the byte.
 */
static void     IKBD_Cmd_Return_Byte_Delay ( uint8_t Data, int Delay_Cycles )
{
    IKBD_Send_Byte_Delay ( Data, Delay_Cycles );
}

/*-----------------------------------------------------------------------*/
/**
 * Send bytes from the IKBD to the ACIA. We store the bytes in a buffer
 * and we pull a new byte each time TDR needs to be re-filled.
 *
 * A possible delay can be specified to simulate the fact that some IKBD's
 * commands don't return immediately the first byte. This delay is given
 * in 68000 cycles at 8 MHz and should be converted to a number of bits
 * at the chosen baud rate.
 */
static void	IKBD_Send_Byte_Delay ( const uint8_t Data, int Delay_Cycles )
{
    //fprintf ( stderr , "send byte=0x%02x delay=%d\n" , Data , Delay_Cycles );
    /* Is keyboard initialised yet ? Ignore any bytes until it is */
    if ( bDuringResetCriticalTime ) {
        LOG_TRACE ( TRACE_IKBD_ACIA, "ikbd is resetting, can't send byte=0x%02x\n", Data);
        return;
    }

    /* Check we have space to add one byte */
    if ( IKBD_OutputBuffer_CheckFreeCount ( 1 ) ) {
        /* Add byte to our buffer */
        Keyboard.Buffer[Keyboard.BufferTail++] = Data;
        Keyboard.BufferTail &= KEYBOARD_BUFFER_MASK;
        Keyboard.NbBytesInOutputBuffer++;
    } else LOG_TRACE(TRACE_IKBD_ACIA, "IKBD buffer is full, can't send 0x%02x!\n", Data );
}

/*-----------------------------------------------------------------------*/
/**
 * Calculate out 'delta' that mouse has moved by each frame, and add this to our internal keyboard position
 */
static void IKBD_UpdateInternalMousePosition(void)
{

    KeyboardProcessor.Mouse.DeltaX = KeyboardProcessor.Mouse.dx;
    KeyboardProcessor.Mouse.DeltaY = KeyboardProcessor.Mouse.dy;
    KeyboardProcessor.Mouse.dx = 0;
    KeyboardProcessor.Mouse.dy = 0;

    /* Update internal mouse coords - Y axis moves according to YAxis setting(up/down) */
    /* Limit to Max X/Y(inclusive) */
    if ( KeyboardProcessor.Mouse.XScale > 1 )
        KeyboardProcessor.Abs.X += KeyboardProcessor.Mouse.DeltaX * KeyboardProcessor.Mouse.XScale;
    else
        KeyboardProcessor.Abs.X += KeyboardProcessor.Mouse.DeltaX;
    if (KeyboardProcessor.Abs.X < 0)
        KeyboardProcessor.Abs.X = 0;
    if (KeyboardProcessor.Abs.X > KeyboardProcessor.Abs.MaxX)
        KeyboardProcessor.Abs.X = KeyboardProcessor.Abs.MaxX;

    if ( KeyboardProcessor.Mouse.YScale > 1 )
        KeyboardProcessor.Abs.Y += KeyboardProcessor.Mouse.DeltaY*KeyboardProcessor.Mouse.YAxis * KeyboardProcessor.Mouse.YScale;
    else
        KeyboardProcessor.Abs.Y += KeyboardProcessor.Mouse.DeltaY*KeyboardProcessor.Mouse.YAxis;
    if (KeyboardProcessor.Abs.Y < 0)
        KeyboardProcessor.Abs.Y = 0;
    if (KeyboardProcessor.Abs.Y > KeyboardProcessor.Abs.MaxY)
        KeyboardProcessor.Abs.Y = KeyboardProcessor.Abs.MaxY;

}

/*-----------------------------------------------------------------------*/
/**
 * Convert button to bool value
 */
static bool IKBD_ButtonBool(int Button)
{
    /* Button pressed? */
    if (Button)
        return true;
    return false;
}


/*-----------------------------------------------------------------------*/
/**
 * Return true if buttons match, use this as buttons are a mask and not boolean
 */
static bool IKBD_ButtonsEqual(int Button1,int Button2)
{
    /* Return bool compare */
    return (IKBD_ButtonBool(Button1) == IKBD_ButtonBool(Button2));
}


/*-----------------------------------------------------------------------*/
/**
 * According to if the mouse is enabled or not the joystick 1 fire
 * button/right mouse button will become the same button. That means
 * pressing one will also press the other and vice-versa.
 * If both mouse and joystick are enabled, report it as a mouse button
 * (needed by the game Big Run for example).
 */
static void IKBD_DuplicateMouseFireButtons(void)
{
    /* If mouse is off then joystick fire button goes to joystick */
    if (KeyboardProcessor.MouseMode == AUTOMODE_OFF) {
        /* If pressed right mouse button, should go to joystick 1 */
        if (Keyboard.bRButtonDown&BUTTON_MOUSE)
            KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK1] |= ATARIJOY_BITMASK_FIRE;
        /* And left mouse button, should go to joystick 0 */
        if (Keyboard.bLButtonDown&BUTTON_MOUSE)
            KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK0] |= ATARIJOY_BITMASK_FIRE;
    }
    /* If mouse is on, joystick 1 fire button goes to the mouse instead */
    else {
        /* Is fire button pressed? */
        if (KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK1]&ATARIJOY_BITMASK_FIRE) {
            KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK1] &= ~ATARIJOY_BITMASK_FIRE;  /* Clear fire button bit */
            Keyboard.bRButtonDown |= BUTTON_JOYSTICK;  /* Mimic right mouse button */
        } else
            Keyboard.bRButtonDown &= ~BUTTON_JOYSTICK;
    }
}

/*-----------------------------------------------------------------------*/
/**
 * Send 'relative' mouse position
 * In case DeltaX or DeltaY are more than 127 units, we send the position
 * using several packets (with a while loop)
 */
static void IKBD_SendRelMousePacket(void)
{
    int ByteRelX,ByteRelY;
    uint8_t Header;

    while ( true ) {
        ByteRelX = KeyboardProcessor.Mouse.DeltaX;
        if ( ByteRelX > 127 )		ByteRelX = 127;
        if ( ByteRelX < -128 )		ByteRelX = -128;

        ByteRelY = KeyboardProcessor.Mouse.DeltaY;
        if ( ByteRelY > 127 )		ByteRelY = 127;
        if ( ByteRelY < -128 )		ByteRelY = -128;

        if ( ( ( ByteRelX < 0 ) && ( ByteRelX <= -KeyboardProcessor.Mouse.XThreshold ) )
                || ( ( ByteRelX > 0 ) && ( ByteRelX >= KeyboardProcessor.Mouse.XThreshold ) )
                || ( ( ByteRelY < 0 ) && ( ByteRelY <= -KeyboardProcessor.Mouse.YThreshold ) )
                || ( ( ByteRelY > 0 ) && ( ByteRelY >= KeyboardProcessor.Mouse.YThreshold ) )
                || ( !IKBD_ButtonsEqual(Keyboard.bOldLButtonDown,Keyboard.bLButtonDown ) )
                || ( !IKBD_ButtonsEqual(Keyboard.bOldRButtonDown,Keyboard.bRButtonDown ) ) ) {
            Header = 0xf8;
            if (Keyboard.bLButtonDown)
                Header |= 0x02;
            if (Keyboard.bRButtonDown)
                Header |= 0x01;

            if ( IKBD_OutputBuffer_CheckFreeCount ( 3 ) ) {
                IKBD_Cmd_Return_Byte (Header);
                IKBD_Cmd_Return_Byte (ByteRelX);
                IKBD_Cmd_Return_Byte (ByteRelY*KeyboardProcessor.Mouse.YAxis);
            }

            KeyboardProcessor.Mouse.DeltaX -= ByteRelX;
            KeyboardProcessor.Mouse.DeltaY -= ByteRelY;

            /* Store buttons for next time around */
            Keyboard.bOldLButtonDown = Keyboard.bLButtonDown;
            Keyboard.bOldRButtonDown = Keyboard.bRButtonDown;
        }

        else
            break;					/* exit the while loop */
    }
}


/**
 * Get joystick data
 */
static void IKBD_GetJoystickData(void)
{
    /* Joystick 1 */
    KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK1] = Joy_GetStickData(JOYID_JOYSTICK1);

    /* If mouse is on, joystick 0 is not connected */
    if (KeyboardProcessor.MouseMode==AUTOMODE_OFF
            || (bBothMouseAndJoy && KeyboardProcessor.MouseMode==AUTOMODE_MOUSEREL))
        KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK0] = Joy_GetStickData(JOYID_JOYSTICK0);
    else
        KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK0] = 0x00;
}


/*-----------------------------------------------------------------------*/
/**
 * Send 'joysticks' bit masks
 */
static void IKBD_SendAutoJoysticks(void)
{
    uint8_t JoyData;

    /* Did joystick 0/mouse change? */
    JoyData = KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK0];
    if (JoyData!=KeyboardProcessor.Joy.PrevJoyData[JOYID_JOYSTICK0]) {
        if ( IKBD_OutputBuffer_CheckFreeCount ( 2 ) ) {
            IKBD_Cmd_Return_Byte (0xFE);			/* Joystick 0 / Mouse */
            IKBD_Cmd_Return_Byte (JoyData);
        }
        KeyboardProcessor.Joy.PrevJoyData[JOYID_JOYSTICK0] = JoyData;
    }

    /* Did joystick 1(default) change? */
    JoyData = KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK1];
    if (JoyData!=KeyboardProcessor.Joy.PrevJoyData[JOYID_JOYSTICK1]) {
        if ( IKBD_OutputBuffer_CheckFreeCount ( 2 ) ) {
            IKBD_Cmd_Return_Byte (0xFF);			/* Joystick 1 */
            IKBD_Cmd_Return_Byte (JoyData);
        }
        KeyboardProcessor.Joy.PrevJoyData[JOYID_JOYSTICK1] = JoyData;
    }
}

/*-----------------------------------------------------------------------*/
/**
 * Send 'joysticks' bit masks when in monitoring mode
 *	%000000xy	; where y is JOYSTICK1 Fire button
 *			; and x is JOYSTICK0 Fire button
 *	%nnnnmmmm	; where m is JOYSTICK1 state
 *			; and n is JOYSTICK0 state
 */
static void IKBD_SendAutoJoysticksMonitoring(void)
{
    uint8_t Byte1;
    uint8_t Byte2;

    Byte1 = ( ( KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK0] & ATARIJOY_BITMASK_FIRE ) >> 6 )
            | ( ( KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK1] & ATARIJOY_BITMASK_FIRE ) >> 7 );

    Byte2 = ( ( KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK0] & 0x0f ) << 4 )
            | ( KeyboardProcessor.Joy.JoyData[JOYID_JOYSTICK1] & 0x0f );

    IKBD_Cmd_Return_Byte (Byte1);
    IKBD_Cmd_Return_Byte (Byte2);
    //fprintf ( stderr , "joystick monitoring %x %x VBL=%d HBL=%d\n" , Byte1 , Byte2 , nVBLs , nHBL );
}

/*-----------------------------------------------------------------------*/
/**
 * Send packets which are generated from the mouse action settings
 * If relative mode is on, still generate these packets
 */
static void IKBD_SendOnMouseAction(void)
{
    bool bReportPosition = false;

    /* Report buttons as keys? Do in relative/absolute mode */
    if (KeyboardProcessor.Mouse.Action&0x4) {
        if ( IKBD_OutputBuffer_CheckFreeCount ( 2 ) ) {
            /* Left button? */
            if ( (IKBD_ButtonBool(Keyboard.bLButtonDown) && (!IKBD_ButtonBool(Keyboard.bOldLButtonDown))) )
                IKBD_Cmd_Return_Byte (0x74);		/* Left */
            else if ( (IKBD_ButtonBool(Keyboard.bOldLButtonDown) && (!IKBD_ButtonBool(Keyboard.bLButtonDown))) )
                IKBD_Cmd_Return_Byte (0x74|0x80);
            /* Right button? */
            if ( (IKBD_ButtonBool(Keyboard.bRButtonDown) && (!IKBD_ButtonBool(Keyboard.bOldRButtonDown))) )
                IKBD_Cmd_Return_Byte (0x75);		/* Right */
            else if ( (IKBD_ButtonBool(Keyboard.bOldRButtonDown) && (!IKBD_ButtonBool(Keyboard.bRButtonDown))) )
                IKBD_Cmd_Return_Byte (0x75|0x80);
        }
        /* Ignore bottom two bits, so return now */
        return;
    }

    /* Check MouseAction - report position on press/release */
    /* MUST do this before update relative positions as buttons get reset */
    if (KeyboardProcessor.Mouse.Action&0x3) {
        /* Check for 'press'? */
        if (KeyboardProcessor.Mouse.Action&0x1) {
            /* Did 'press' mouse buttons? */
            if ( (IKBD_ButtonBool(Keyboard.bLButtonDown) && (!IKBD_ButtonBool(Keyboard.bOldLButtonDown))) ) {
                bReportPosition = true;
                KeyboardProcessor.Abs.PrevReadAbsMouseButtons &= ~0x04;
                KeyboardProcessor.Abs.PrevReadAbsMouseButtons |= 0x02;
            }
            if ( (IKBD_ButtonBool(Keyboard.bRButtonDown) && (!IKBD_ButtonBool(Keyboard.bOldRButtonDown))) ) {
                bReportPosition = true;
                KeyboardProcessor.Abs.PrevReadAbsMouseButtons &= ~0x01;
                KeyboardProcessor.Abs.PrevReadAbsMouseButtons |= 0x08;
            }
        }
        /* Check for 'release'? */
        if (KeyboardProcessor.Mouse.Action&0x2) {
            /* Did 'release' mouse buttons? */
            if ( (IKBD_ButtonBool(Keyboard.bOldLButtonDown) && (!IKBD_ButtonBool(Keyboard.bLButtonDown))) ) {
                bReportPosition = true;
                KeyboardProcessor.Abs.PrevReadAbsMouseButtons &= ~0x08;
                KeyboardProcessor.Abs.PrevReadAbsMouseButtons |= 0x01;
            }
            if ( (IKBD_ButtonBool(Keyboard.bOldRButtonDown) && (!IKBD_ButtonBool(Keyboard.bRButtonDown))) ) {
                bReportPosition = true;
                KeyboardProcessor.Abs.PrevReadAbsMouseButtons &= ~0x02;
                KeyboardProcessor.Abs.PrevReadAbsMouseButtons |= 0x04;
            }
        }

        /* Do need to report? */
        if (bReportPosition) {
            /* Only report if mouse in absolute mode */
            if (KeyboardProcessor.MouseMode==AUTOMODE_MOUSEABS) {
                LOG_TRACE(TRACE_IKBD_CMDS, "Report ABS on MouseAction\n");
                IKBD_Cmd_ReadAbsMousePos();
            }
        }
    }
}

/*-----------------------------------------------------------------------*/
/**
 * Send mouse movements as cursor keys
 */
static void IKBD_SendCursorMousePacket(void)
{
    int i=0;

    /* Run each 'Delta' as cursor presses */
    /* Limit to '10' loops as host mouse cursor might have a VERY poor quality. */
    /* Eg, a single mouse movement on and ST gives delta's of '1', mostly, */
    /* but host mouse might go as high as 20+! */
//fprintf ( stderr , "key %d %d %d %d %d %d\n" , KeyboardProcessor.Mouse.DeltaX , KeyboardProcessor.Mouse.DeltaY, Keyboard.bOldLButtonDown,Keyboard.bLButtonDown, Keyboard.bOldRButtonDown,Keyboard.bRButtonDown );
    while ( (i<10) && ((KeyboardProcessor.Mouse.DeltaX!=0) || (KeyboardProcessor.Mouse.DeltaY!=0)
                       || (!IKBD_ButtonsEqual(Keyboard.bOldLButtonDown,Keyboard.bLButtonDown)) || (!IKBD_ButtonsEqual(Keyboard.bOldRButtonDown,Keyboard.bRButtonDown))) ) {
        if ( KeyboardProcessor.Mouse.DeltaX != 0 ) {
            /* Left? */
            if (KeyboardProcessor.Mouse.DeltaX <= -KeyboardProcessor.Mouse.KeyCodeDeltaX) {
                if ( IKBD_OutputBuffer_CheckFreeCount ( 2 ) ) {
                    IKBD_Cmd_Return_Byte (75);		/* Left cursor */
                    IKBD_Cmd_Return_Byte (75|0x80);
                }
                KeyboardProcessor.Mouse.DeltaX += KeyboardProcessor.Mouse.KeyCodeDeltaX;
            }
            /* Right? */
            if (KeyboardProcessor.Mouse.DeltaX >= KeyboardProcessor.Mouse.KeyCodeDeltaX) {
                if ( IKBD_OutputBuffer_CheckFreeCount ( 2 ) ) {
                    IKBD_Cmd_Return_Byte (77);		/* Right cursor */
                    IKBD_Cmd_Return_Byte (77|0x80);
                }
                KeyboardProcessor.Mouse.DeltaX -= KeyboardProcessor.Mouse.KeyCodeDeltaX;
            }
        }

        if ( KeyboardProcessor.Mouse.DeltaY != 0 ) {
            /* Up? */
            if (KeyboardProcessor.Mouse.DeltaY <= -KeyboardProcessor.Mouse.KeyCodeDeltaY) {
                if ( IKBD_OutputBuffer_CheckFreeCount ( 2 ) ) {
                    IKBD_Cmd_Return_Byte (72);		/* Up cursor */
                    IKBD_Cmd_Return_Byte (72|0x80);
                }
                KeyboardProcessor.Mouse.DeltaY += KeyboardProcessor.Mouse.KeyCodeDeltaY;
            }
            /* Down? */
            if (KeyboardProcessor.Mouse.DeltaY >= KeyboardProcessor.Mouse.KeyCodeDeltaY) {
                if ( IKBD_OutputBuffer_CheckFreeCount ( 2 ) ) {
                    IKBD_Cmd_Return_Byte (80);		/* Down cursor */
                    IKBD_Cmd_Return_Byte (80|0x80);
                }
                KeyboardProcessor.Mouse.DeltaY -= KeyboardProcessor.Mouse.KeyCodeDeltaY;
            }
        }

        if ( IKBD_OutputBuffer_CheckFreeCount ( 2 ) ) {
            /* Left button? */
            if ( (IKBD_ButtonBool(Keyboard.bLButtonDown) && (!IKBD_ButtonBool(Keyboard.bOldLButtonDown))) )
                IKBD_Cmd_Return_Byte (0x74);		/* Left */
            else if ( (IKBD_ButtonBool(Keyboard.bOldLButtonDown) && (!IKBD_ButtonBool(Keyboard.bLButtonDown))) )
                IKBD_Cmd_Return_Byte (0x74|0x80);
            /* Right button? */
            if ( (IKBD_ButtonBool(Keyboard.bRButtonDown) && (!IKBD_ButtonBool(Keyboard.bOldRButtonDown))) )
                IKBD_Cmd_Return_Byte (0x75);		/* Right */
            else if ( (IKBD_ButtonBool(Keyboard.bOldRButtonDown) && (!IKBD_ButtonBool(Keyboard.bRButtonDown))) )
                IKBD_Cmd_Return_Byte (0x75|0x80);
        }
        Keyboard.bOldLButtonDown = Keyboard.bLButtonDown;
        Keyboard.bOldRButtonDown = Keyboard.bRButtonDown;

        /* Count, so exit if try too many times! */
        i++;
    }
}


/*-----------------------------------------------------------------------*/
/**
 * Return packets from keyboard for auto, rel mouse, joystick etc...
 */
void IKBD_SendAutoKeyboardCommands(void)
{
    /* Don't do anything until processor is first reset */
    if ( bDuringResetCriticalTime )
        return;

    /* Read joysticks for this frame */
    IKBD_GetJoystickData();

    /* Check for double-clicks in maximum speed mode */
    //IKBD_CheckForDoubleClicks();

    /* Handle Joystick/Mouse fire buttons */
    IKBD_DuplicateMouseFireButtons();

    /* Send any packets which are to be reported by mouse action */
    IKBD_SendOnMouseAction();

    /* Update internal mouse absolute position by find 'delta' of mouse movement */
    IKBD_UpdateInternalMousePosition();

    /* If IKBD is monitoring only joysticks, don't report other events */
    if ( KeyboardProcessor.JoystickMode == AUTOMODE_JOYSTICK_MONITORING ) {
        IKBD_SendAutoJoysticksMonitoring();
        return;
    }

    /* Send automatic joystick packets */
    if (KeyboardProcessor.JoystickMode==AUTOMODE_JOYSTICK)
        IKBD_SendAutoJoysticks();
    /* Send automatic relative mouse positions(absolute are not send automatically) */
    if (KeyboardProcessor.MouseMode==AUTOMODE_MOUSEREL)
        IKBD_SendRelMousePacket();
    /* Send cursor key directions for movements */
    else if (KeyboardProcessor.MouseMode==AUTOMODE_MOUSECURSOR)
        IKBD_SendCursorMousePacket();

    /* Store buttons for next time around */
    Keyboard.bOldLButtonDown = Keyboard.bLButtonDown;
    Keyboard.bOldRButtonDown = Keyboard.bRButtonDown;
#if 0
    /* Send joystick button '2' as 'Space bar' key - MUST do here so does not get mixed up in middle of joystick packets! */
    if (JoystickSpaceBar==JOYSTICK_SPACE_DOWN) {
        IKBD_PressSTKey(57, true);                /* Press */
        JoystickSpaceBar = JOYSTICK_SPACE_DOWNED; /* Pressed */
    } else if (JoystickSpaceBar==JOYSTICK_SPACE_UP) {
        IKBD_PressSTKey(57, false);               /* Release */
        JoystickSpaceBar = JOYSTICK_SPACE_NULL;   /* Complete */
    }

    /* If we're executing a custom IKBD program, call it to process the key/mouse/joystick event */
    if ( IKBD_ExeMode && pIKBD_CustomCodeHandler_Read )
        (*pIKBD_CustomCodeHandler_Read) ();
#endif
}

/*-----------------------------------------------------------------------*/
/**
 * When press/release key under host OS, execute this function.
 */
void IKBD_PressSTKey(uint8_t ScanCode, bool bPress)
{
    /* If IKBD is monitoring only joysticks, don't report key */
    if ( KeyboardProcessor.JoystickMode == AUTOMODE_JOYSTICK_MONITORING )
        return;

    /* Store the state of each ST scancode : 1=pressed 0=released */
    if ( bPress )           ScanCodeState[ ScanCode & 0x7f ] = 1;
    else                    ScanCodeState[ ScanCode & 0x7f ] = 0;

    if (!bPress)
        ScanCode |= 0x80;                               /* Set top bit if released key */

    if ( IKBD_OutputBuffer_CheckFreeCount ( 1 ) ) {
        IKBD_Cmd_Return_Byte (ScanCode);                /* Add to the IKBD's output buffer */
    }
#if 0
    /* If we're executing a custom IKBD program, call it to process the key event */
    if ( IKBD_ExeMode && pIKBD_CustomCodeHandler_Read )
        (*pIKBD_CustomCodeHandler_Read) ();
#endif
}



/*-----------------------------------------------------------------------*/
/**
 * On ST if disable Mouse AND Joystick with a set time of a RESET command they are
 * actually turned back on! (A number of games do this so can get mouse and joystick
 * packets at the same time)
 */
static void IKBD_CheckResetDisableBug(void)
{
    /* Have disabled BOTH mouse and joystick? */
    if (bMouseDisabled && bJoystickDisabled) {
        /* And in critical time? */
        if (bDuringResetCriticalTime) {
            /* Emulate relative mouse and joystick reports being turned back on */
            KeyboardProcessor.MouseMode = AUTOMODE_MOUSEREL;
            KeyboardProcessor.JoystickMode = AUTOMODE_JOYSTICK;
            bBothMouseAndJoy = true;

            LOG_TRACE(TRACE_IKBD_ALL, "ikbd cancel commands 0x12 and 0x1a received during reset,"
                                      " enabling joystick and mouse reporting at the same time\n" );
        }
    }
}

/*-----------------------------------------------------------------------*/
/**
 * When a byte is received by the IKBD, it is added to a small 8 byte buffer.
 * - If the first byte is a valid command, we wait for additional bytes if needed
 *   and then we execute the command's handler.
 * - If the first byte is not a valid command or after a successful command, we
 *   empty the input buffer (extra bytes, if any, are lost)
 * - If the input buffer is full when a new byte is received, the new byte is lost.
 * - In case the first byte read is not a valid command then IKBD does nothing
 *   (it doesn't return any byte to indicate the command was not recognized)
 */
void IKBD_RunKeyboardCommand(uint8_t aciabyte)
{
    int i=0;

    /* Write into our keyboard input buffer if it's not full yet */
    if ( Keyboard.nBytesInInputBuffer < SIZE_KEYBOARDINPUT_BUFFER )
        Keyboard.InputBuffer[Keyboard.nBytesInInputBuffer++] = aciabyte;

    /* Now check bytes to see if we have a valid/in-valid command string set */
    while (KeyboardCommands[i].Command!=0xff) {
        /* Found command? */
        if (KeyboardCommands[i].Command==Keyboard.InputBuffer[0]) {
            /* If the command is complete (with its possible parameters) we can execute it */
            /* Else, we wait for the next bytes until the command is complete */
            if (KeyboardCommands[i].NumParameters==Keyboard.nBytesInInputBuffer) {
                /* Any new valid command will unpause the output (if command 0x13 was used) */
                Keyboard.PauseOutput = false;

                CALL_VAR(KeyboardCommands[i].pCallFunction);
                Keyboard.nBytesInInputBuffer = 0;       /* Clear input buffer after processing a command */
            }

            return;
        }

        i++;
    }

    /* Command not known, reset buffer(IKBD assumes a NOP) */
    Keyboard.nBytesInInputBuffer = 0;
}


/************************************************************************/
/* List of keyboard commands handled by the standard IKBD's ROM.        */
/* Each IKBD's command is emulated to get the same result as if we were */
/* running a full HD6301 emulation.                                     */
/************************************************************************/


/*-----------------------------------------------------------------------*/
/**
 * RESET
 *
 * 0x80
 * 0x01
 *
 * Performs self test and checks for stuck (closed) keys, if OK returns
 * IKBD_ROM_VERSION (0xF1). Otherwise returns break codes for keys (not emulated).
 */
static void IKBD_Cmd_Reset()
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_Reset\n");

    /* Check that 0x01 was received after 0x80 */
    if (Keyboard.InputBuffer[1] == 0x01) {
        IKBD_Boot_ROM ( false );
    }
    /* else if not 0x80,0x01 just ignore */
}

/*-----------------------------------------------------------------------*/
/**
 * SET MOUSE BUTTON ACTION
 *
 * 0x07
 * %00000mss  ; mouse button action
 *       ;  (m is presumed =1 when in MOUSE KEYCODE mode)
 *       ; mss=0xy, mouse button press or release causes mouse
 *       ;  position report
 *       ;  where y=1, mouse key press causes absolute report
 *       ;  and x=1, mouse key release causes absolute report
 *       ; mss=100, mouse buttons act like keys
 */
static void IKBD_Cmd_MouseAction()
{
    KeyboardProcessor.Mouse.Action = Keyboard.InputBuffer[1];
    KeyboardProcessor.Abs.PrevReadAbsMouseButtons = ABS_PREVBUTTONS;

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_MouseAction %d\n", KeyboardProcessor.Mouse.Action);
}

/*-----------------------------------------------------------------------*/
/**
 * SET RELATIVE MOUSE POSITION REPORTING
 *
 * 0x08
 */
static void IKBD_Cmd_RelMouseMode()
{
    KeyboardProcessor.MouseMode = AUTOMODE_MOUSEREL;

    /* Some games (like Barbarian by Psygnosis) enable both, mouse and
     * joystick directly after a reset. This causes the IKBD to send both
     * type of packets. To emulate this feature, we've got to remember
     * that the mouse has been enabled during reset. */
    if (bDuringResetCriticalTime)
        bMouseEnabledDuringReset = true;

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_RelMouseMode\n");
}

/*-----------------------------------------------------------------------*/
/**
 * SET ABSOLUTE MOUSE POSITIONING
 *
 * 0x09
 * XMSB      ;X maximum (in scaled mouse clicks)
 * XLSB
 * YMSB      ;Y maximum (in scaled mouse clicks)
 * YLSB
 */
static void IKBD_Cmd_AbsMouseMode()
{
    /* These maximums are 'inclusive' */
    KeyboardProcessor.MouseMode = AUTOMODE_MOUSEABS;
    KeyboardProcessor.Abs.MaxX = Keyboard.InputBuffer[1]<<8 | Keyboard.InputBuffer[2];
    KeyboardProcessor.Abs.MaxY = Keyboard.InputBuffer[3]<<8 | Keyboard.InputBuffer[4];

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_AbsMouseMode %d,%d\n",
              KeyboardProcessor.Abs.MaxX, KeyboardProcessor.Abs.MaxY);
}

/*-----------------------------------------------------------------------*/
/**
 * SET MOUSE KEYCODE MODE
 *
 * 0x0A
 * deltax      ; distance in X clicks to return (LEFT) or (RIGHT)
 * deltay      ; distance in Y clicks to return (UP) or (DOWN)
 */
static void IKBD_Cmd_MouseCursorKeycodes()
{
    KeyboardProcessor.MouseMode = AUTOMODE_MOUSECURSOR;
    KeyboardProcessor.Mouse.KeyCodeDeltaX = Keyboard.InputBuffer[1];
    KeyboardProcessor.Mouse.KeyCodeDeltaY = Keyboard.InputBuffer[2];

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_MouseCursorKeycodes %d,%d\n",
              (int)KeyboardProcessor.Mouse.KeyCodeDeltaX,
              (int)KeyboardProcessor.Mouse.KeyCodeDeltaY);
}


/*-----------------------------------------------------------------------*/
/**
 * SET MOUSE THRESHOLD
 *
 * 0x0B
 * X      ; x threshold in mouse ticks (positive integers)
 * Y      ; y threshold in mouse ticks (positive integers)
 */
static void IKBD_Cmd_SetMouseThreshold()
{
    KeyboardProcessor.Mouse.XThreshold = Keyboard.InputBuffer[1];
    KeyboardProcessor.Mouse.YThreshold = Keyboard.InputBuffer[2];

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_SetMouseThreshold %d,%d\n",
              KeyboardProcessor.Mouse.XThreshold, KeyboardProcessor.Mouse.YThreshold);
}


/*-----------------------------------------------------------------------*/
/**
 * SET MOUSE SCALE
 *
 * 0x0C
 * X      ; horizontal mouse ticks per internal X
 * Y      ; vertical mouse ticks per internal Y
 */
static void IKBD_Cmd_SetMouseScale()
{
    KeyboardProcessor.Mouse.XScale = Keyboard.InputBuffer[1];
    KeyboardProcessor.Mouse.YScale = Keyboard.InputBuffer[2];

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_SetMouseScale %d,%d\n",
              KeyboardProcessor.Mouse.XScale, KeyboardProcessor.Mouse.YScale);
}

/*-----------------------------------------------------------------------*/
/**
 * INTERROGATE MOUSE POSITION
 *
 * 0x0D
 *   Returns:  0xF7  ; absolute mouse position header
 *     BUTTONS
 *       0000dcba
 *       where a is right button down since last interrogation
 *       b is right button up since last
 *       c is left button down since last
 *       d is left button up since last
 *     XMSB      ; X coordinate
 *     XLSB
 *     YMSB      ; Y coordinate
 *     YLSB
 */
static void IKBD_Cmd_ReadAbsMousePos()
{
    /* Test buttons */
    uint8_t Buttons = 0;
    /* Set buttons to show if up/down */
    if (Keyboard.bRButtonDown)
        Buttons |= 0x01;
    else
        Buttons |= 0x02;
    if (Keyboard.bLButtonDown)
        Buttons |= 0x04;
    else
        Buttons |= 0x08;
    /* Mask off it didn't send last time */
    const uint8_t PrevButtons = KeyboardProcessor.Abs.PrevReadAbsMouseButtons;
    KeyboardProcessor.Abs.PrevReadAbsMouseButtons = Buttons;
    Buttons &= ~PrevButtons;

    /* And send packet */
    if ( IKBD_OutputBuffer_CheckFreeCount ( 6 ) ) {
        IKBD_Cmd_Return_Byte_Delay (0xf7, 18000-ACIA_CYCLES);
        IKBD_Cmd_Return_Byte (Buttons);
        IKBD_Cmd_Return_Byte ((unsigned int)KeyboardProcessor.Abs.X>>8);
        IKBD_Cmd_Return_Byte ((unsigned int)KeyboardProcessor.Abs.X&0xff);
        IKBD_Cmd_Return_Byte ((unsigned int)KeyboardProcessor.Abs.Y>>8);
        IKBD_Cmd_Return_Byte ((unsigned int)KeyboardProcessor.Abs.Y&0xff);
    }

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReadAbsMousePos %d,%d 0x%X\n",
              KeyboardProcessor.Abs.X, KeyboardProcessor.Abs.Y, Buttons);
}

/*-----------------------------------------------------------------------*/
/**
 * LOAD MOUSE POSITION
 *
 * 0x0E
 * 0x00      ; filler
 * XMSB      ; X coordinate
 * XLSB      ; (in scaled coordinate system)
 * YMSB      ; Y coordinate
 * YLSB
 */
static void IKBD_Cmd_SetInternalMousePos()
{
    /* Setting these do not clip internal position(this happens on next update) */
    KeyboardProcessor.Abs.X = Keyboard.InputBuffer[2]<<8 | Keyboard.InputBuffer[3];
    KeyboardProcessor.Abs.Y = Keyboard.InputBuffer[4]<<8 | Keyboard.InputBuffer[5];

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_SetInternalMousePos %d,%d\n",
              KeyboardProcessor.Abs.X, KeyboardProcessor.Abs.Y);
}


/*-----------------------------------------------------------------------*/
/**
 * SET Y=0 AT BOTTOM
 *
 * 0x0F
 */
static void IKBD_Cmd_SetYAxisDown()
{
    /* KeyboardProcessor.Mouse.YAxis = -1; */
    /*
     * Changing this because PS/2 mouse seems to use the opposite sense and there isn't
     * a way to change it.
     */
    KeyboardProcessor.Mouse.YAxis = 1;
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_SetYAxisDown\n");
}


/*-----------------------------------------------------------------------*/
/**
 * SET Y=0 AT TOP
 *
 * 0x10
 */
static void IKBD_Cmd_SetYAxisUp()
{
    /* KeyboardProcessor.Mouse.YAxis = 1; */
    /*
     * Changing this because PS/2 mouse seems to use the opposite sense and there isn't
     * a way to change it.
     */
    KeyboardProcessor.Mouse.YAxis = -1;
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_SetYAxisUp\n");
}


/*-----------------------------------------------------------------------*/
/**
 * RESUME
 *
 * Any command received by the IKBD will also resume the output if it was
 * paused by command 0x13, so this command is redundant.
 *
 * 0x11
 */
static void IKBD_Cmd_StartKeyboardTransfer()
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_StartKeyboardTransfer\n");
    Keyboard.PauseOutput = false;
}

/*-----------------------------------------------------------------------*/
/**
 * DISABLE MOUSE
 *
 * 0x12
 */
static void IKBD_Cmd_TurnMouseOff()
{
    KeyboardProcessor.MouseMode = AUTOMODE_OFF;
    bMouseDisabled = true;

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_TurnMouseOff\n");

    IKBD_CheckResetDisableBug();
}


/*-----------------------------------------------------------------------*/
/**
 * PAUSE OUTPUT
 *
 * 0x13
 */
static void IKBD_Cmd_StopKeyboardTransfer()
{
    if (bDuringResetCriticalTime) {
        /* Required for the loader of 'Just Bugging' by ACF */
        LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_StopKeyboardTransfer ignored during ikbd reset\n");
        return;
    }

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_StopKeyboardTransfer\n");
    Keyboard.PauseOutput = true;
}

/*-----------------------------------------------------------------------*/
/**
 * SET JOYSTICK EVENT REPORTING
 *
 * 0x14
 */
static void IKBD_Cmd_ReturnJoystickAuto()
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReturnJoystickAuto\n");

    KeyboardProcessor.JoystickMode = AUTOMODE_JOYSTICK;
    KeyboardProcessor.MouseMode = AUTOMODE_OFF;

    /* If mouse was also enabled within time of a reset (0x08 command) it isn't disabled now!
     * (used by the game Barbarian 1 by Psygnosis for example) */
    if ( bDuringResetCriticalTime && bMouseEnabledDuringReset ) {
        KeyboardProcessor.MouseMode = AUTOMODE_MOUSEREL;
        bBothMouseAndJoy = true;
        LOG_TRACE(TRACE_IKBD_ALL, "ikbd commands 0x08 and 0x14 received during reset,"
                                  " enabling joystick and mouse reporting at the same time\n" );
    }
    /* If mouse was disabled during the reset (0x12 command) it is enabled again
     * (used by the game Hammerfist for example) */
    else if ( bDuringResetCriticalTime && bMouseDisabled ) {
        KeyboardProcessor.MouseMode = AUTOMODE_MOUSEREL;
        bBothMouseAndJoy = true;
        LOG_TRACE(TRACE_IKBD_ALL, "ikbd commands 0x12 and 0x14 received during reset,"
                                  " enabling joystick and mouse reporting at the same time\n" );
    }

    /* This command resets the internally previously stored joystick states */
    KeyboardProcessor.Joy.PrevJoyData[JOYID_JOYSTICK0] = KeyboardProcessor.Joy.PrevJoyData[JOYID_JOYSTICK1] = 0;

    /* This is a hack for the STE Utopos (=> v1.50) and Falcon Double Bubble
     * 2000 games. They expect the joystick data to be sent within a certain
     * amount of time after this command, without checking the ACIA control
     * register first.
     */
    IKBD_GetJoystickData();
    IKBD_SendAutoJoysticks();
}

/*-----------------------------------------------------------------------*/
/**
 * SET JOYSTICK INTERROGATION MODE
 *
 * 0x15
 */
static void IKBD_Cmd_StopJoystick(void)
{
    KeyboardProcessor.JoystickMode = AUTOMODE_OFF;
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_StopJoystick\n");
}


/*-----------------------------------------------------------------------*/
/**
 * JOYSTICK INTERROGATE
 *
 * 0x16
 */
static void IKBD_Cmd_ReturnJoystick(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReturnJoystick\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 3 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xFD, IKBD_Delay_Random ( 7500, 10000 ) );
        IKBD_Cmd_Return_Byte (Joy_GetStickData(JOYID_JOYSTICK0));
        IKBD_Cmd_Return_Byte (Joy_GetStickData(JOYID_JOYSTICK1));
    }
}

/*-----------------------------------------------------------------------*/
/**
 * SET JOYSTICK MONITORING
 *
 * 0x17
 * rate      ; time between samples in hundredths of a second
 *   Returns: (in packets of two as long as in mode)
 *     %000000xy  where y is JOYSTICK1 Fire button
 *         and x is JOYSTICK0 Fire button
 *     %nnnnmmmm  where m is JOYSTICK1 state
 *         and n is JOYSTICK0 state
 *
 * TODO : we use a fixed 8 MHz clock to convert rate in 1/100th of sec into cycles.
 * This should be replaced by using MachineClocks.CPU_Freq.
 */
static void IKBD_Cmd_SetJoystickMonitoring(void)
{
    int     Rate;
    int     Cycles;

    Rate = (unsigned int)Keyboard.InputBuffer[1];

    KeyboardProcessor.JoystickMode = AUTOMODE_JOYSTICK_MONITORING;
    KeyboardProcessor.MouseMode = AUTOMODE_OFF;

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_SetJoystickMonitoring %d\n" , Rate );

    if ( Rate == 0 )
        Rate = 1;

    Cycles = 8021247 * Rate / 100;
    // FIXME: Implement this
    // CycInt_AddRelativeInterrupt ( Cycles, INT_CPU8_CYCLE, INTERRUPT_IKBD_AUTOSEND );

    Keyboard.AutoSendCycles = Cycles;
}

/*-----------------------------------------------------------------------*/
/**
 * SET FIRE BUTTON MONITORING
 *
 * 0x18
 *   Returns: (as long as in mode)
 *     %bbbbbbbb  ; state of the JOYSTICK1 fire button packed
 *           ; 8 bits per byte, the first sample if the MSB
 */
static void IKBD_Cmd_SetJoystickFireDuration(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_SetJoystickFireDuration (not implemented)\n");
}

/*-----------------------------------------------------------------------*/
/**
 * SET JOYSTICK KEYCODE MODE
 *
 * 0x19
 * RX        ; length of time (in tenths of seconds) until
 *         ; horizontal velocity breakpoint is reached
 * RY        ; length of time (in tenths of seconds) until
 *         ; vertical velocity breakpoint is reached
 * TX        ; length (in tenths of seconds) of joystick closure
 *         ; until horizontal cursor key is generated before RX
 *         ; has elapsed
 * TY        ; length (in tenths of seconds) of joystick closure
 *         ; until vertical cursor key is generated before RY
 *         ; has elapsed
 * VX        ; length (in tenths of seconds) of joystick closure
 *         ; until horizontal cursor keystrokes are generated after RX
 *         ; has elapsed
 * VY        ; length (in tenths of seconds) of joystick closure
 *         ; until vertical cursor keystrokes are generated after RY
 *         ; has elapsed
 */
static void IKBD_Cmd_SetCursorForJoystick(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_SetCursorForJoystick (not implemented)\n");
}

/*-----------------------------------------------------------------------*/
/**
 * DISABLE JOYSTICKS
 *
 * 0x1A
 */
static void IKBD_Cmd_DisableJoysticks(void)
{
    KeyboardProcessor.JoystickMode = AUTOMODE_OFF;
    bJoystickDisabled = true;

    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_DisableJoysticks\n");

    IKBD_CheckResetDisableBug();
}





/*-----------------------------------------------------------------------*/
/**
 * REPORT MOUSE BUTTON ACTION
 *
 * 0x87
 */
static void IKBD_Cmd_ReportMouseAction(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReportMouseAction\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 8 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xF6, IKBD_Delay_Random ( 7000, 7500 ) );
        IKBD_Cmd_Return_Byte (7);
        IKBD_Cmd_Return_Byte (KeyboardProcessor.Mouse.Action);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
    }
}


/*-----------------------------------------------------------------------*/
/**
 * REPORT MOUSE MODE
 *
 * 0x88 or 0x89 or 0x8A
 */
static void IKBD_Cmd_ReportMouseMode(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReportMouseMode\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 8 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xF6, IKBD_Delay_Random ( 7000, 7500 ) );
        switch (KeyboardProcessor.MouseMode) {
        case AUTOMODE_MOUSEREL:
            IKBD_Cmd_Return_Byte (8);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            break;
        case AUTOMODE_MOUSEABS:
            IKBD_Cmd_Return_Byte (9);
            IKBD_Cmd_Return_Byte (KeyboardProcessor.Abs.MaxX >> 8);
            IKBD_Cmd_Return_Byte (KeyboardProcessor.Abs.MaxX);
            IKBD_Cmd_Return_Byte (KeyboardProcessor.Abs.MaxY >> 8);
            IKBD_Cmd_Return_Byte (KeyboardProcessor.Abs.MaxY);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            break;
        case AUTOMODE_MOUSECURSOR:
            IKBD_Cmd_Return_Byte (10);
            IKBD_Cmd_Return_Byte (KeyboardProcessor.Mouse.KeyCodeDeltaX);
            IKBD_Cmd_Return_Byte (KeyboardProcessor.Mouse.KeyCodeDeltaY);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            break;
        default: ;
        }
    }
}


/*-----------------------------------------------------------------------*/
/**
 * REPORT MOUSE THRESHOLD
 *
 * 0x8B
 */
static void IKBD_Cmd_ReportMouseThreshold(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReportMouseThreshold\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 8 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xF6, IKBD_Delay_Random ( 7000, 7500 ) );
        IKBD_Cmd_Return_Byte (0x0B);
        IKBD_Cmd_Return_Byte (KeyboardProcessor.Mouse.XThreshold);
        IKBD_Cmd_Return_Byte (KeyboardProcessor.Mouse.YThreshold);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
    }
}


/*-----------------------------------------------------------------------*/
/**
 * REPORT MOUSE SCALE
 *
 * 0x8C
 */
static void IKBD_Cmd_ReportMouseScale(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReportMouseScale\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 8 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xF6, IKBD_Delay_Random ( 7000, 7500 ) );
        IKBD_Cmd_Return_Byte (0x0C);
        IKBD_Cmd_Return_Byte (KeyboardProcessor.Mouse.XScale);
        IKBD_Cmd_Return_Byte (KeyboardProcessor.Mouse.YScale);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
    }
}


/*-----------------------------------------------------------------------*/
/**
 * REPORT MOUSE VERTICAL COORDINATES
 *
 * 0x8F and 0x90
 */
static void IKBD_Cmd_ReportMouseVertical(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReportMouseVertical\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 8 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xF6, IKBD_Delay_Random ( 7000, 7500 ) );
        if (KeyboardProcessor.Mouse.YAxis == -1)
            IKBD_Cmd_Return_Byte (0x0F);
        else
            IKBD_Cmd_Return_Byte (0x10);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
    }
}


/*-----------------------------------------------------------------------*/
/**
 * REPORT MOUSE AVAILABILITY
 *
 * 0x92
 */
static void IKBD_Cmd_ReportMouseAvailability(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReportMouseAvailability\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 8 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xF6, IKBD_Delay_Random ( 7000, 7500 ) );
        if (KeyboardProcessor.MouseMode == AUTOMODE_OFF)
            IKBD_Cmd_Return_Byte (0x12);
        else
            IKBD_Cmd_Return_Byte (0x00);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
    }
}


/*-----------------------------------------------------------------------*/
/**
 * REPORT JOYSTICK MODE
 *
 * 0x94 or 0x95 or 0x99
 */
static void IKBD_Cmd_ReportJoystickMode(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReportJoystickMode\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 8 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xF6, IKBD_Delay_Random ( 7000, 7500 ) );
        switch (KeyboardProcessor.JoystickMode) {
        case AUTOMODE_JOYSTICK:
            IKBD_Cmd_Return_Byte (0x14);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            break;
        default:    /* TODO: Joystick keycodes mode not supported yet! */
            IKBD_Cmd_Return_Byte (0x15);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            IKBD_Cmd_Return_Byte (0);
            break;
        }
    }
}


/*-----------------------------------------------------------------------*/
/**
 * REPORT JOYSTICK AVAILABILITY
 *
 * 0x9A
 */
static void IKBD_Cmd_ReportJoystickAvailability(void)
{
    LOG_TRACE(TRACE_IKBD_CMDS, "IKBD_Cmd_ReportJoystickAvailability\n");

    if ( IKBD_OutputBuffer_CheckFreeCount ( 8 ) ) {
        IKBD_Cmd_Return_Byte_Delay ( 0xF6, IKBD_Delay_Random ( 7000, 7500 ) );
        if (KeyboardProcessor.JoystickMode == AUTOMODE_OFF)
            IKBD_Cmd_Return_Byte (0x1A);
        else
            IKBD_Cmd_Return_Byte (0x00);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
        IKBD_Cmd_Return_Byte (0);
    }
}

/************************************************************************/
/* End of the IKBD's commands emulation.				*/
/************************************************************************/
