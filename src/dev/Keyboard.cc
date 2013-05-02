/******************************************************************************
 Copyright 2012-2013 Martin Karsten

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
#include "mach/APIC.h"
#include "mach/Machine.h"
#include "dev/Keyboard.h"

// keyboard encoder ------------------------------------------
enum KYBRD_ENCODER_IO {
	KYBRD_ENC_INPUT_BUF              = 0x60,
	KYBRD_ENC_CMD_REG                = 0x60
};

enum KYBRD_ENC_CMDS {
	KYBRD_ENC_CMD_SET_LED            = 0xED,
	KYBRD_ENC_CMD_ECHO               = 0xEE,
	KYBRD_ENC_CMD_SCAN_CODE_SET      = 0xF0,
	KYBRD_ENC_CMD_ID                 = 0xF2,
	KYBRD_ENC_CMD_AUTODELAY          = 0xF3,
	KYBRD_ENC_CMD_ENABLE             = 0xF4,
	KYBRD_ENC_CMD_RESETWAIT          = 0xF5,
	KYBRD_ENC_CMD_RESETSCAN          = 0xF6,
	KYBRD_ENC_CMD_ALL_AUTO           = 0xF7,
	KYBRD_ENC_CMD_ALL_MAKEBREAK      = 0xF8,
	KYBRD_ENC_CMD_ALL_MAKEONLY       = 0xF9,
	KYBRD_ENC_CMD_ALL_MAKEBREAK_AUTO = 0xFA,
	KYBRD_ENC_CMD_SINGLE_AUTOREPEAT  = 0xFB,
	KYBRD_ENC_CMD_SINGLE_MAKEBREAK   = 0xFC,
	KYBRD_ENC_CMD_SINGLE_BREAKONLY   = 0xFD,
	KYBRD_ENC_CMD_RESEND             = 0xFE,
	KYBRD_ENC_CMD_RESET              = 0xFF
};

// keyboard controller ---------------------------------------
enum KYBRD_CTRL_IO {
	KYBRD_CTRL_STATS_REG             = 0x64,
	KYBRD_CTRL_CMD_REG               = 0x64
};

enum KYBRD_CTRL_STATS_MASK {
	KYBRD_CTRL_STATS_MASK_OUT_BUF    = 0x01, //00000001
	KYBRD_CTRL_STATS_MASK_IN_BUF     = 0x02, //00000010
	KYBRD_CTRL_STATS_MASK_SYSTEM     = 0x04, //00000100
	KYBRD_CTRL_STATS_MASK_CMD_DATA   = 0x08, //00001000
	KYBRD_CTRL_STATS_MASK_LOCKED     = 0x10, //00010000
	KYBRD_CTRL_STATS_MASK_AUX_BUF    = 0x20, //00100000
	KYBRD_CTRL_STATS_MASK_TIMEOUT    = 0x40, //01000000
	KYBRD_CTRL_STATS_MASK_PARITY     = 0x80  //10000000
};

enum KYBRD_CTRL_CMDS {
	KYBRD_CTRL_CMD_READ              = 0x20,
	KYBRD_CTRL_CMD_WRITE             = 0x60,
	KYBRD_CTRL_CMD_SELF_TEST         = 0xAA,
	KYBRD_CTRL_CMD_INTERFACE_TEST    = 0xAB,
	KYBRD_CTRL_CMD_DISABLE           = 0xAD,
	KYBRD_CTRL_CMD_ENABLE            = 0xAE,
	KYBRD_CTRL_CMD_READ_IN_PORT      = 0xC0,
	KYBRD_CTRL_CMD_READ_OUT_PORT     = 0xD0,
	KYBRD_CTRL_CMD_WRITE_OUT_PORT    = 0xD1,
	KYBRD_CTRL_CMD_READ_TEST_INPUTS  = 0xE0,
	KYBRD_CTRL_CMD_SYSTEM_RESET      = 0xFE,
	KYBRD_CTRL_CMD_MOUSE_DISABLE     = 0xA7,
	KYBRD_CTRL_CMD_MOUSE_ENABLE      = 0xA8,
	KYBRD_CTRL_CMD_MOUSE_PORT_TEST   = 0xA9,
	KYBRD_CTRL_CMD_MOUSE_WRITE       = 0xD4
};

// scan error codes ------------------------------------------
enum KYBRD_ERROR {
	KYBRD_ERR_BUF_OVERRUN            = 0x00,
	KYBRD_ERR_ID_RET                 = 0x83AB,
	KYBRD_ERR_BAT                    = 0xAA,
	KYBRD_ERR_ECHO_RET               = 0xEE,
	KYBRD_ERR_ACK                    = 0xFA,
	KYBRD_ERR_BAT_FAILED             = 0xFC,
	KYBRD_ERR_DIAG_FAILED            = 0xFD,
	KYBRD_ERR_RESEND_CMD             = 0xFE,
	KYBRD_ERR_KEY                    = 0xFF
};

enum : Keyboard::KeyCode {
  // Alphanumeric keys ////////////////
	KEY_SPACE             = ' ',
	KEY_0                 = '0',
	KEY_1                 = '1',
	KEY_2                 = '2',
	KEY_3                 = '3',
	KEY_4                 = '4',
	KEY_5                 = '5',
	KEY_6                 = '6',
	KEY_7                 = '7',
	KEY_8                 = '8',
	KEY_9                 = '9',

	KEY_A                 = 'a',
	KEY_B                 = 'b',
	KEY_C                 = 'c',
	KEY_D                 = 'd',
	KEY_E                 = 'e',
	KEY_F                 = 'f',
	KEY_G                 = 'g',
	KEY_H                 = 'h',
	KEY_I                 = 'i',
	KEY_J                 = 'j',
	KEY_K                 = 'k',
	KEY_L                 = 'l',
	KEY_M                 = 'm',
	KEY_N                 = 'n',
	KEY_O                 = 'o',
	KEY_P                 = 'p',
	KEY_Q                 = 'q',
	KEY_R                 = 'r',
	KEY_S                 = 's',
	KEY_T                 = 't',
	KEY_U                 = 'u',
	KEY_V                 = 'v',
	KEY_W                 = 'w',
	KEY_X                 = 'x',
	KEY_Y                 = 'y',
	KEY_Z                 = 'z',

	KEY_RETURN            = '\r',
	KEY_ESCAPE            = 0x1001,
	KEY_BACKSPACE         = '\b',

	// Arrow keys ////////////////////////
	KEY_UP                = 0x1100,
	KEY_DOWN              = 0x1101,
	KEY_LEFT              = 0x1102,
	KEY_RIGHT             = 0x1103,

	// Function keys /////////////////////
	KEY_F1                = 0x1201,
	KEY_F2                = 0x1202,
	KEY_F3                = 0x1203,
	KEY_F4                = 0x1204,
	KEY_F5                = 0x1205,
	KEY_F6                = 0x1206,
	KEY_F7                = 0x1207,
	KEY_F8                = 0x1208,
	KEY_F9                = 0x1209,
	KEY_F10               = 0x120a,
	KEY_F11               = 0x120b,
	KEY_F12               = 0x120b,
	KEY_F13               = 0x120c,
	KEY_F14               = 0x120d,
	KEY_F15               = 0x120e,

	KEY_DOT               = '.',
	KEY_COMMA             = ',',
	KEY_COLON             = ':',
	KEY_SEMICOLON         = ';',
	KEY_SLASH             = '/',
	KEY_BACKSLASH         = '\\',
	KEY_PLUS              = '+',
	KEY_MINUS             = '-',
	KEY_ASTERISK          = '*',
	KEY_EXCLAMATION       = '!',
	KEY_QUESTION          = '?',
	KEY_QUOTEDOUBLE       = '\"',
	KEY_QUOTE             = '\'',
	KEY_EQUAL             = '=',
	KEY_HASH              = '#',
	KEY_PERCENT           = '%',
	KEY_AMPERSAND         = '&',
	KEY_UNDERSCORE        = '_',
	KEY_LEFTPARENTHESIS   = '(',
	KEY_RIGHTPARENTHESIS  = ')',
	KEY_LEFTBRACKET       = '[',
	KEY_RIGHTBRACKET      = ']',
	KEY_LEFTCURL          = '{',
	KEY_RIGHTCURL         = '}',
	KEY_DOLLAR            = '$',
	KEY_POUND             = 156,
	KEY_EURO              = 128,
	KEY_LESS              = '<',
	KEY_GREATER           = '>',
	KEY_BAR               = '|',
	KEY_GRAVE             = '`',
	KEY_TILDE             = '~',
	KEY_AT                = '@',
	KEY_CARRET            = '^',

	// Numeric keypad //////////////////////
	KEY_KP_0              = '0',
	KEY_KP_1              = '1',
	KEY_KP_2              = '2',
	KEY_KP_3              = '3',
	KEY_KP_4              = '4',
	KEY_KP_5              = '5',
	KEY_KP_6              = '6',
	KEY_KP_7              = '7',
	KEY_KP_8              = '8',
	KEY_KP_9              = '9',
	KEY_KP_PLUS           = '+',
	KEY_KP_MINUS          = '-',
	KEY_KP_DECIMAL        = '.',
	KEY_KP_DIVIDE         = '/',
	KEY_KP_ASTERISK       = '*',
	KEY_KP_NUMLOCK        = 0x300f,
	KEY_KP_ENTER          = 0x3010,

	KEY_TAB               = 0x4000,
	KEY_CAPSLOCK          = 0x4001,

	// Modify keys ////////////////////////////
  KEY_LSHIFT            = 0x4002,
	KEY_LCTRL             = 0x4003,
	KEY_LALT              = 0x4004,
	KEY_LWIN              = 0x4005,
	KEY_RSHIFT            = 0x4006,
	KEY_RCTRL             = 0x4007,
	KEY_RALT              = 0x4008,
	KEY_RWIN              = 0x4009,

	KEY_INSERT            = 0x400a,
	KEY_DELETE            = 0x400b,
	KEY_HOME              = 0x400c,
	KEY_END               = 0x400d,
	KEY_PAGEUP            = 0x400e,
	KEY_PAGEDOWN          = 0x400f,
	KEY_SCROLLLOCK        = 0x4010,
	KEY_PAUSE             = 0x4011,

	KEY_UNKNOWN,
	KEY_NUMKEYCODES
};

// xt scan code set; index is make code
static const int keyTable[] = {
	// key scancode
	KEY_UNKNOWN, //0x00
	KEY_ESCAPE, //0x01
	KEY_1, //0x02
	KEY_2, //0x03
	KEY_3, //0x04
	KEY_4, //0x05
	KEY_5, //0x06
	KEY_6, //0x07
	KEY_7, //0x08
	KEY_8, //0x09
	KEY_9, //0x0a
	KEY_0, //0x0b
	KEY_MINUS, //0x0c
	KEY_EQUAL, //0x0d
	KEY_BACKSPACE, //0x0e
	KEY_TAB, //0x0f
	KEY_Q, //0x10
	KEY_W, //0x11
	KEY_E, //0x12
	KEY_R, //0x13
	KEY_T, //0x14
	KEY_Y, //0x15
	KEY_U, //0x16
	KEY_I, //0x17
	KEY_O, //0x18
	KEY_P, //0x19
	KEY_LEFTBRACKET, //0x1a
	KEY_RIGHTBRACKET, //0x1b
	KEY_RETURN, //0x1c
	KEY_LCTRL, //0x1d
	KEY_A, //0x1e
	KEY_S, //0x1f
	KEY_D, //0x20
	KEY_F, //0x21
	KEY_G, //0x22
	KEY_H, //0x23
	KEY_J, //0x24
	KEY_K, //0x25
	KEY_L, //0x26
	KEY_SEMICOLON, //0x27
	KEY_QUOTE, //0x28
	KEY_GRAVE, //0x29
	KEY_LSHIFT, //0x2a
	KEY_BACKSLASH, //0x2b
	KEY_Z, //0x2c
	KEY_X, //0x2d
	KEY_C, //0x2e
	KEY_V, //0x2f
	KEY_B, //0x30
	KEY_N, //0x31
	KEY_M, //0x32
	KEY_COMMA, //0x33
	KEY_DOT, //0x34
	KEY_SLASH, //0x35
	KEY_RSHIFT, //0x36
	KEY_KP_ASTERISK, //0x37
	KEY_RALT, //0x38
	KEY_SPACE, //0x39
	KEY_CAPSLOCK, //0x3a
	KEY_F1, //0x3b
	KEY_F2, //0x3c
	KEY_F3, //0x3d
	KEY_F4, //0x3e
	KEY_F5, //0x3f
	KEY_F6, //0x40
	KEY_F7, //0x41
	KEY_F8, //0x42
	KEY_F9, //0x43
	KEY_F10, //0x44
	KEY_KP_NUMLOCK, //0x45
	KEY_SCROLLLOCK, //0x46
	KEY_HOME, //0x47
	KEY_KP_8, //0x48	//keypad up arrow
	KEY_KP_9, //0x49  //keypad page up
	KEY_KP_MINUS, //0x4a
	KEY_KP_4, //0x4b
	KEY_KP_5, //0x4c
	KEY_KP_6, //0x4d
	KEY_KP_PLUS, //0x4e //keypad page down
	KEY_KP_1, //0x4f
	KEY_KP_2, //0x50	//keypad down arrow
	KEY_KP_3, //0x51	//keypad page down
	KEY_KP_0, //0x52	//keypad insert key
	KEY_KP_DECIMAL, //0x53	//keypad delete key
	KEY_UNKNOWN, //0x54
	KEY_UNKNOWN, //0x55
	KEY_UNKNOWN, //0x56
	KEY_F11, //0x57
	KEY_F12 //0x58
};

// invalid scan code. Used to indicate the last scan code is not to be reused
static const int INVALID_SCANCODE = 0;

static int scanCodeSet = 2;

// translation table from AT to XT scan codes
static const uint8_t scanCodeTable[0x80] = {
	0xff,0x43,0x41,0x3f,0x3d,0x3b,0x3c,0x58,0x64,0x44,0x42,0x40,0x3e,0x0f,0x29,0x59,
	0x65,0x38,0x2a,0x70,0x1d,0x10,0x02,0x5a,0x66,0x71,0x2c,0x1f,0x1e,0x11,0x03,0x5b,
	0x67,0x2e,0x2d,0x20,0x12,0x05,0x04,0x5c,0x68,0x39,0x2f,0x21,0x14,0x13,0x06,0x5d,
	0x69,0x31,0x30,0x23,0x22,0x15,0x07,0x5e,0x6a,0x72,0x32,0x24,0x16,0x08,0x09,0x5f,
	0x6b,0x33,0x25,0x17,0x18,0x0b,0x0a,0x60,0x6c,0x34,0x35,0x26,0x27,0x19,0x0c,0x61,
	0x6d,0x73,0x28,0x74,0x1a,0x0d,0x62,0x6e,0x3a,0x36,0x1c,0x1b,0x75,0x2b,0x63,0x76,
	0x55,0x56,0x77,0x78,0x79,0x7a,0x0e,0x7b,0x7c,0x4f,0x7d,0x4b,0x47,0x7e,0x7f,0x6f,
	0x52,0x53,0x50,0x4c,0x4d,0x48,0x01,0x45,0x57,0x4e,0x51,0x4a,0x37,0x49,0x46,0x54,
};

// X11 keycodes (see xmodmap -pk) -> not used currently
static const uint8_t x11Code[2][0x80] = {
{   0,   9,  10,  11,  12,  13,  14,  15, 
   16,  17,  18,  19,  20,  21,  22,  23, 
   24,  25,  26,  27,  28,  29,  30,  31, 
   32,  33,  34,  35,  36,  37,  38,  39, 
   40,  41,  42,  43,  44,  45,  46,  47, 
   48,  49,  50,  51,  52,  53,  54,  55, 
   56,  57,  58,  59,  60,  61,  62,  63, 
   64,  65,  66,  67,  68,  69,  70,  71, 
   72,  73,  74,  75,  76,  77,  78,  79, 
   80,  81,  82,  83,  84,  85,  86,  87, 
   88,  89,  90,  91,   0,   0,   0,  95, 
   96,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0 },
{   0,  75,   0,  71,  69,  67,  68,  96, 
    0,  76,  74,  72,  70,  23,  49,   0, 
    0,  64,  50,   0,  37,  24,  10,   0, 
    0,   0,  52,  39,  38,  25,  11,   0, 
    0,  54,  53,  40,  26,  13,  12,   0, 
    0,  65,  55,  41,  28,  27,  14,   0, 
    0,  57,  56,  43,  42,  29,  15,   0, 
    0,   0,  58,  44,  30,  16,  17,   0, 
    0,  59,  45,  31,  32,  19,  18,   0, 
    0,  60,  61,  46,  47,  33,  20,   0, 
    0,   0,  48,   0,  34,  21,   0,   0, 
   66,  62,  36,  35,   0,  51,   0,   0,
    0,   0,  22,   0,   0,  87,   0,  83,
   79,   0,   0,   0,  90,  91,  88,  84,
   85,  80,   9,  77,  95,  86,  89,  82,
   63,  81,  78,   0,   0,   0,   0,  73 }
};

inline uint8_t Keyboard::ctrl_read_status() {
	return in8(KYBRD_CTRL_STATS_REG); // read status from keyboard controller
}

inline bool Keyboard::read_buffer() {
	return (ctrl_read_status() & KYBRD_CTRL_STATS_MASK_OUT_BUF);
}

inline bool Keyboard::write_buffer() {
	return (ctrl_read_status() & KYBRD_CTRL_STATS_MASK_IN_BUF);
}

inline void Keyboard::ctrl_send_cmd(uint8_t cmd) {
	while (write_buffer());
	out8(KYBRD_CTRL_CMD_REG, cmd);   // send command byte to keyboard controller
}

inline uint8_t Keyboard::enc_read_buf() {
	while (!read_buffer());
	return in8(KYBRD_ENC_INPUT_BUF); // read command byte from encoder
}

inline void Keyboard::enc_send_cmd(uint8_t cmd) {
	while (write_buffer());
	out8(KYBRD_ENC_CMD_REG, cmd);    // send command byte to encoder
}

inline void Keyboard::drain_buffer() {
	while (read_buffer()) {
		uint8_t data = enc_read_buf(); // clear keyboard buffer
		DBG::out(DBG::Devices, ' ', FmtHex(data));
	}
}

inline bool Keyboard::self_test() {
	ctrl_send_cmd(KYBRD_CTRL_CMD_SELF_TEST); // send command
	return (enc_read_buf() == 0x55);
}

// sets LEDs
inline void Keyboard::set_leds() {
	enc_send_cmd(KYBRD_ENC_CMD_SET_LED);
	enc_send_cmd(led);
	acks += 2;
}

void Keyboard::init() {
	// TODO: initialize USB controllers, disable USB legacy -> not done yet
	// TODO: determine PS/2 presence -> in ACPI.cc, but not supported in qemu/bochs

	uint8_t data, config;
	DBG::out(DBG::Devices, "PS/2 ports:");
	if (!self_test()) return;
	DBG::out(DBG::Devices, " self-test");

	ctrl_send_cmd(KYBRD_CTRL_CMD_READ);            // ask for configuration byte
	config = enc_read_buf();                       // read configuration byte
	DBG::out(DBG::Devices, ' ', FmtHex(config));

	if (!(config & (1 << 4)) || !(config & (1 << 5))) {
		config &= ~(1 << 0);                           // disable keyboard interrupts
		config &= ~(1 << 1);                           // disable mouse interrupts
		config &= ~(1 << 6);                           // disable XT translation
		ctrl_send_cmd(KYBRD_CTRL_CMD_WRITE);           // announce configuration byte
		enc_send_cmd(config);                          // write back configuration byte

		enc_send_cmd(KYBRD_ENC_CMD_RESETWAIT);         // stop keyboard scanning
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_ACK, FmtHex(data) );
		ctrl_send_cmd(KYBRD_CTRL_CMD_MOUSE_WRITE);     // send next byte to mouse
		enc_send_cmd(KYBRD_ENC_CMD_RESETWAIT);         // stop mouse scanning
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_ACK, FmtHex(data) );
		drain_buffer();
	}

	if (!(config & (1 << 4))) {                     // test for keyboard
		DBG::out(DBG::Devices, " keyboard");
		ctrl_send_cmd(KYBRD_CTRL_CMD_INTERFACE_TEST); // test keyboard
		data = enc_read_buf();
		KASSERT( data == 0x00, FmtHex(data));
		enc_send_cmd(KYBRD_ENC_CMD_RESET);            // reset keyboard
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_ACK, FmtHex(data));
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_BAT, FmtHex(data));
		DBG::out(DBG::Devices, "/reset");

		enc_send_cmd(KYBRD_ENC_CMD_SCAN_CODE_SET);    // get/set scancode set
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_ACK, FmtHex(data));
		enc_send_cmd(0x02);                           // set scancode set 2 (AT)
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_ACK, FmtHex(data));
		DBG::out(DBG::Devices, "/scan");
		enc_send_cmd(KYBRD_ENC_CMD_SCAN_CODE_SET);    // get/set scancode set
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_ACK, FmtHex(data));
		enc_send_cmd(0x00);                           // get scancode set
		data = enc_read_buf();
		DBG::out(DBG::Devices, '/', FmtHex(data));
		if (data == KYBRD_ERR_ACK) {
			if (read_buffer()) data = enc_read_buf();   // should use timeout
			else data = 0x02;
		}
		DBG::out(DBG::Devices, '/', FmtHex(data));
		switch (data) {
			case 0x01: case 0x43: scanCodeSet = 1; break;          // XT scancode set
			case 0x02: case 0x41: scanCodeSet = 2; break;          // AT scancode set
			case 0x03: case 0x3F: scanCodeSet = 3; break;          // PS/2 scancode set
			default: scanCodeSet = 2; break;
		}

		enc_send_cmd(KYBRD_ENC_CMD_ENABLE);            // enable keyboard scanning
		data = enc_read_buf();
		if (data == KYBRD_ERR_ACK) {
			config |= (1 << 0);                          // enable interrupts (below)
			DBG::out(DBG::Devices, " A");
		}
	}

	if (!(config & (1 << 5))) {                      // test for mouse
		DBG::out(DBG::Devices, " mouse");
		ctrl_send_cmd(KYBRD_CTRL_CMD_MOUSE_PORT_TEST); // test mouse
		data = enc_read_buf();
		KASSERT( data == 0x00, FmtHex(data) );
		ctrl_send_cmd(KYBRD_CTRL_CMD_MOUSE_WRITE);     // send next byte to moouse
		enc_send_cmd(KYBRD_ENC_CMD_RESET);             // reset mouse
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_ACK, FmtHex(data) );
		data = enc_read_buf();
		KASSERT( data == KYBRD_ERR_BAT, FmtHex(data) );

		ctrl_send_cmd(KYBRD_CTRL_CMD_MOUSE_WRITE);     // send next byte to mouse
		enc_send_cmd(KYBRD_ENC_CMD_ENABLE);            // enable mouse scanning
		data = enc_read_buf();
		if (data == KYBRD_ERR_ACK) {
			config |= (1 << 1);                          // enable interrupts (below)
			DBG::out(DBG::Devices, " A");
		}
	}

	DBG::outln(DBG::Devices, ' ', FmtHex(config), ' ', FmtHex(scanCodeSet));
	ctrl_send_cmd(KYBRD_CTRL_CMD_WRITE);             // write back configuration byte
	enc_send_cmd(config);

	if (config & (1 << 0)) Machine::staticEnableIRQ(PIC::Keyboard, 0x21);
	if (config & (1 << 1)) Machine::staticEnableIRQ(PIC::Mouse, 0x2c);
}

void Keyboard::staticInterruptHandler() {
	// read scan code while the output buffer is full (scan code is in it)
	while (read_buffer()) {
		KeyCode code = enc_read_buf();          // read the scan code
		KASSERT(acks >= 0, "unexpected ACK from keyboard");
		switch (code) {                     // watch for errors
			case 0xE0: case 0xE1:       extended =   true; continue;
			case KYBRD_ERR_ACK:         acks -= 1;         continue;
			case KYBRD_ERR_BAT_FAILED:  bat_res =   false; continue;
			case KYBRD_ERR_DIAG_FAILED: diag_res =  false; continue;
			case KYBRD_ERR_RESEND_CMD:  resend_res = true; continue;
		}

		if unlikely(scanCodeSet == 1) {
			if (code & 0x80) {                // check for break code
				is_break = true;
				code &= ~0x80;
			}
		} else {
			if (code == 0xf0) {               // check for break code
				is_break = true;
	continue;
			} else {
				KASSERT(code >= 0 && code < 0x80, code);
				code = scanCodeTable[code];     // translate to XT scancode set
			}
		}
		KeyCode key = keyTable[code];       // get key code from XT set

#if KEYBOARD_RAW
		kbq.appendFromISR(key);
		continue;
#endif

		if (is_break) {                     // break code
			is_break = false;
			switch (key) {                    // special keys released?
				case KEY_LCTRL:  case KEY_RCTRL:  ctrl =  false; break;
				case KEY_LSHIFT: case KEY_RSHIFT: shift = false; break;
				case KEY_LALT:   case KEY_RALT:   alt =   false; break;
			}
		} else {                            // make code
			scancode = code;
			switch (key) {                    // special keys pressed?
				case KEY_LCTRL:  case KEY_RCTRL:  ctrl =  true; break;
				case KEY_LSHIFT: case KEY_RSHIFT: shift = true; break;
				case KEY_LALT:   case KEY_RALT:   alt =   true; break;
				case KEY_SCROLLLOCK: led ^= 1; set_leds(); break;
				case KEY_KP_NUMLOCK: led ^= 2; set_leds(); break;
				case KEY_CAPSLOCK:   led ^= 4; set_leds(); break;
				default:
					if ((shift || get_capslock()) && key >= 'a' && key <= 'z') key -= 32;
					else if (shift) switch (key) {
						case '0': key = KEY_RIGHTPARENTHESIS; break;
						case '1': key = KEY_EXCLAMATION; break;
						case '2': key = KEY_AT; break;
						case '3': key = KEY_EXCLAMATION; break;
						case '4': key = KEY_HASH; break;
						case '5': key = KEY_PERCENT; break;
						case '6': key = KEY_CARRET; break;
						case '7': key = KEY_AMPERSAND; break;
						case '8': key = KEY_ASTERISK; break;
						case '9': key = KEY_LEFTPARENTHESIS; break;
						case KEY_COMMA:        key = KEY_LESS; break;
						case KEY_DOT:          key = KEY_GREATER; break;
						case KEY_SLASH:        key = KEY_QUESTION; break;
						case KEY_SEMICOLON:    key = KEY_COLON; break;
						case KEY_QUOTE:        key = KEY_QUOTEDOUBLE; break;
						case KEY_LEFTBRACKET:  key = KEY_LEFTCURL; break;
						case KEY_RIGHTBRACKET: key = KEY_RIGHTCURL; break;
						case KEY_GRAVE:        key = KEY_TILDE; break;
						case KEY_MINUS:        key = KEY_UNDERSCORE; break;
						case KEY_PLUS:         key = KEY_EQUAL; break;
						case KEY_BACKSLASH:    key = KEY_BAR; break;
					} else if (ctrl && alt && extended && key == KEY_KP_DECIMAL) Reboot();
					kbq.appendFromISR(key); // add key to key buffer!
					break;
			}
		}
		extended = false;
	}
}

// TODO: handle TAB, ENTER, etc.
void Keyboard::disable() {
	ctrl_send_cmd(KYBRD_CTRL_CMD_DISABLE);
	disabled = true;
}

void Keyboard::enable() {
	ctrl_send_cmd(KYBRD_CTRL_CMD_ENABLE);
	disabled = false;
}

// system reset: 11111110 to output port sets reset system line low
void Keyboard::reset_system() {
	ctrl_send_cmd(KYBRD_CTRL_CMD_WRITE_OUT_PORT);
	enc_send_cmd(KYBRD_ENC_CMD_RESEND);
}
