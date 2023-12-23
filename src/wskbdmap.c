#include <sys/types.h>
#include <sys/time.h>
#include "wscons.h"
#include "input-event-codes.h"

static uint32_t wsUsbMap[] = {
	/* 0 */ KEY_RESERVED,
	/* 1 */ KEY_RESERVED,
	/* 2 */ KEY_RESERVED,
	/* 3 */ KEY_RESERVED,
	/* 4 */ KEY_A,
	/* 5 */ KEY_B,
	/* 6 */ KEY_C,
	/* 7 */ KEY_D,
	/* 8 */ KEY_E,
	/* 9 */ KEY_F,
	/* 10 */ KEY_G,
	/* 11 */ KEY_H,
	/* 12 */ KEY_I,
	/* 13 */ KEY_J,
	/* 14 */ KEY_K,
	/* 15 */ KEY_L,
	/* 16 */ KEY_M,
	/* 17 */ KEY_N,
	/* 18 */ KEY_O,
	/* 19 */ KEY_P,
	/* 20 */ KEY_Q,
	/* 21 */ KEY_R,
	/* 22 */ KEY_S,
	/* 23 */ KEY_T,
	/* 24 */ KEY_U,
	/* 25 */ KEY_V,
	/* 26 */ KEY_W,
	/* 27 */ KEY_X,
	/* 28 */ KEY_Y,
	/* 29 */ KEY_Z,
	/* 30 */ KEY_1,		/* 1 !*/
	/* 31 */ KEY_2,		/* 2 @ */
	/* 32 */ KEY_3,		/* 3 # */
	/* 33 */ KEY_4,		/* 4 $ */
	/* 34 */ KEY_5,		/* 5 % */
	/* 35 */ KEY_6,		/* 6 ^ */
	/* 36 */ KEY_7,		/* 7 & */
	/* 37 */ KEY_8,		/* 8 * */
	/* 38 */ KEY_9,		/* 9 ( */
	/* 39 */ KEY_0,		/* 0 ) */
	/* 40 */ KEY_ENTER,	/* Return  */
	/* 41 */ KEY_ESC,	/* Escape */
	/* 42 */ KEY_BACKSPACE,	/* Backspace Delete */
	/* 43 */ KEY_TAB,	/* Tab */
	/* 44 */ KEY_SPACE,	/* Space */
	/* 45 */ KEY_MINUS,	/* - _ */
	/* 46 */ KEY_EQUAL,	/* = + */
	/* 47 */ KEY_LEFTBRACE,	/* [ { */
	/* 48 */ KEY_RIGHTBRACE,	/* ] } */
	/* 49 */ KEY_BACKSLASH,	/* \ | */
	/* 50 */ KEY_BACKSLASH,    /* \ _ # ~ on some keyboards */
	/* 51 */ KEY_SEMICOLON,	/* ; : */
	/* 52 */ KEY_APOSTROPHE,	/* ' " */
	/* 53 */ KEY_GRAVE,	/* ` ~ */
	/* 54 */ KEY_COMMA,	/* , <  */
	/* 55 */ KEY_DOT,	/* . > */
	/* 56 */ KEY_SLASH,	/* / ? */
	/* 57 */ KEY_CAPSLOCK,	/* Caps Lock */
	/* 58 */ KEY_F1,		/* F1 */
	/* 59 */ KEY_F2,		/* F2 */
	/* 60 */ KEY_F3,		/* F3 */
	/* 61 */ KEY_F4,		/* F4 */
	/* 62 */ KEY_F5,		/* F5 */
	/* 63 */ KEY_F6,		/* F6 */
	/* 64 */ KEY_F7,		/* F7 */
	/* 65 */ KEY_F8,		/* F8 */
	/* 66 */ KEY_F9,		/* F9 */
	/* 67 */ KEY_F10,	/* F10 */
	/* 68 */ KEY_F11,	/* F11 */
	/* 69 */ KEY_F12,	/* F12 */
	/* 70 */ KEY_PRINT,	/* PrintScrn SysReq */
	/* 71 */ KEY_SCROLLLOCK,	/* Scroll Lock */
	/* 72 */ KEY_PAUSE,	/* Pause Break */
	/* 73 */ KEY_INSERT,	/* Insert XXX  Help on some Mac Keyboards */
	/* 74 */ KEY_HOME,	/* Home */
	/* 75 */ KEY_PAGEUP,	/* Page Up */
	/* 76 */ KEY_DELETE,	/* Delete */
	/* 77 */ KEY_END,	/* End */
	/* 78 */ KEY_PAGEDOWN,	/* Page Down */
	/* 79 */ KEY_RIGHT,	/* Right Arrow */
	/* 80 */ KEY_LEFT,	/* Left Arrow */
	/* 81 */ KEY_DOWN,	/* Down Arrow */
	/* 82 */ KEY_UP,		/* Up Arrow */
	/* 83 */ KEY_NUMLOCK,	/* Num Lock */
	/* 84 */ KEY_KPSLASH,	/* Keypad / */
	/* 85 */ KEY_KPASTERISK, /* Keypad * */
	/* 86 */ KEY_KPMINUS,	/* Keypad - */
	/* 87 */ KEY_KPPLUS,	/* Keypad + */
	/* 88 */ KEY_KPENTER,	/* Keypad Enter */
	/* 89 */ KEY_KP1,	/* Keypad 1 End */
	/* 90 */ KEY_KP2,	/* Keypad 2 Down */
	/* 91 */ KEY_KP3,	/* Keypad 3 Pg Down */
	/* 92 */ KEY_KP4,	/* Keypad 4 Left  */
	/* 93 */ KEY_KP5,	/* Keypad 5 */
	/* 94 */ KEY_KP6,	/* Keypad 6 */
	/* 95 */ KEY_KP7,	/* Keypad 7 Home */
	/* 96 */ KEY_KP8,	/* Keypad 8 Up */
	/* 97 */ KEY_KP9,	/* KEypad 9 Pg Up */
	/* 98 */ KEY_KP0,	/* Keypad 0 Ins */
	/* 99 */ KEY_KPDOT,	/* Keypad . Del */
	/* 100 */ KEY_102ND,	/* < > on some keyboards */
	/* 101 */ KEY_MENU,	/* Menu */
	/* 102 */ KEY_POWER,	/* sleep key on Sun USB */
	/* 103 */ KEY_KPEQUAL, /* Keypad = on Mac keyboards */
	/* 104 */ KEY_F13,
	/* 105 */ KEY_F14,
	/* 106 */ KEY_F15,
	/* 107 */ KEY_F16,
	/* 108 */ KEY_RESERVED,
	/* 109 */ KEY_POWER,
	/* 110 */ KEY_RESERVED,
	/* 111 */ KEY_RESERVED,
	/* 112 */ KEY_RESERVED,
	/* 113 */ KEY_RESERVED,
	/* 114 */ KEY_RESERVED,
	/* 115 */ KEY_RESERVED,
	/* 116 */ KEY_OPEN,	/* L7 */
	/* 117 */ KEY_HELP,
	/* 118 */ KEY_PROPS,	/* L3 */
	/* 119 */ KEY_FRONT,	/* L5 */
	/* 120 */ KEY_STOP,	/* L1 */
	/* 121 */ KEY_AGAIN,	/* L2 */
	/* 122 */ KEY_UNDO,	/* L4 */
	/* 123 */ KEY_CUT,	/* L10 */
	/* 124 */ KEY_COPY,	/* L6 */
	/* 125 */ KEY_PASTE,	/* L8 */
	/* 126 */ KEY_FIND,	/* L9 */
	/* 127 */ KEY_MUTE,
	/* 128 */ KEY_VOLUMEUP,
	/* 129 */ KEY_VOLUMEDOWN,
	/* 130 */ KEY_RESERVED,
	/* 131 */ KEY_RESERVED,
	/* 132 */ KEY_RESERVED,
	/* 133 */ KEY_RESERVED,
	/* 134 */ KEY_RESERVED,
	/* 135 */ KEY_RESERVED,	/* Japanese 106 kbd: '\_' */
	/* 136 */ KEY_RESERVED,	/* Japanese 106 kbd: Hiragana Katakana toggle */
	/* 137 */ KEY_YEN,	/* Japanese 106 kbd: '\|' */
	/* 138 */ KEY_RESERVED,	/* Japanese 106 kbd: Henkan */
	/* 139 */ KEY_RESERVED,	/* Japanese 106 kbd: Muhenkan */
	/* 140 */ KEY_RESERVED,
	/* 141 */ KEY_RESERVED,
	/* 142 */ KEY_RESERVED,
	/* 143 */ KEY_RESERVED,
	/* 144 */ KEY_RESERVED,	/* Korean 106 kbd: Hangul */
	/* 145 */ KEY_RESERVED,	/* Korean 106 kbd: Hangul Hanja */
	/* 146 */ KEY_RESERVED,
	/* 147 */ KEY_RESERVED,
	/* 148 */ KEY_RESERVED,
	/* 149 */ KEY_RESERVED,
	/* 150 */ KEY_RESERVED,
	/* 151 */ KEY_RESERVED,
	/* 152 */ KEY_RESERVED,
	/* 153 */ KEY_RESERVED,
	/* 154 */ KEY_RESERVED,
	/* 155 */ KEY_RESERVED,
	/* 156 */ KEY_RESERVED,
	/* 157 */ KEY_RESERVED,
	/* 158 */ KEY_RESERVED,
	/* 159 */ KEY_RESERVED,
	/* 160 */ KEY_RESERVED,
	/* 161 */ KEY_RESERVED,
	/* 162 */ KEY_RESERVED,
	/* 163 */ KEY_RESERVED,
	/* 164 */ KEY_RESERVED,
	/* 165 */ KEY_RESERVED,
	/* 166 */ KEY_RESERVED,
	/* 167 */ KEY_RESERVED,
	/* 168 */ KEY_RESERVED,
	/* 169 */ KEY_RESERVED,
	/* 170 */ KEY_RESERVED,
	/* 171 */ KEY_RESERVED,
	/* 172 */ KEY_RESERVED,
	/* 173 */ KEY_RESERVED,
	/* 174 */ KEY_RESERVED,
	/* 175 */ KEY_RESERVED,
	/* 176 */ KEY_RESERVED,
	/* 177 */ KEY_RESERVED,
	/* 178 */ KEY_RESERVED,
	/* 179 */ KEY_RESERVED,
	/* 180 */ KEY_RESERVED,
	/* 181 */ KEY_RESERVED,
	/* 182 */ KEY_RESERVED,
	/* 183 */ KEY_RESERVED,
	/* 184 */ KEY_RESERVED,
	/* 185 */ KEY_RESERVED,
	/* 186 */ KEY_RESERVED,
	/* 187 */ KEY_RESERVED,
	/* 188 */ KEY_RESERVED,
	/* 189 */ KEY_RESERVED,
	/* 190 */ KEY_RESERVED,
	/* 191 */ KEY_RESERVED,
	/* 192 */ KEY_RESERVED,
	/* 193 */ KEY_RESERVED,
	/* 194 */ KEY_RESERVED,
	/* 195 */ KEY_RESERVED,
	/* 196 */ KEY_RESERVED,
	/* 197 */ KEY_RESERVED,
	/* 198 */ KEY_RESERVED,
	/* 199 */ KEY_RESERVED,
	/* 200 */ KEY_RESERVED,
	/* 201 */ KEY_RESERVED,
	/* 202 */ KEY_RESERVED,
	/* 203 */ KEY_RESERVED,
	/* 204 */ KEY_RESERVED,
	/* 205 */ KEY_RESERVED,
	/* 206 */ KEY_RESERVED,
	/* 207 */ KEY_RESERVED,
	/* 208 */ KEY_RESERVED,
	/* 209 */ KEY_RESERVED,
	/* 210 */ KEY_RESERVED,
	/* 211 */ KEY_RESERVED,
	/* 212 */ KEY_RESERVED,
	/* 213 */ KEY_RESERVED,
	/* 214 */ KEY_RESERVED,
	/* 215 */ KEY_RESERVED,
	/* 216 */ KEY_RESERVED,
	/* 217 */ KEY_RESERVED,
	/* 218 */ KEY_RESERVED,
	/* 219 */ KEY_RESERVED,
	/* 220 */ KEY_RESERVED,
	/* 221 */ KEY_RESERVED,
	/* 222 */ KEY_RESERVED,
	/* 223 */ KEY_RESERVED,
	/* 224 */ KEY_LEFTCTRL,	/* Left Control */
	/* 225 */ KEY_LEFTSHIFT,	/* Left Shift */
	/* 226 */ KEY_LEFTALT,	/* Left Alt */
	/* 227 */ KEY_LEFTMETA,	/* Left Meta */
	/* 228 */ KEY_RIGHTCTRL,	/* Right Control */
	/* 229 */ KEY_RIGHTSHIFT,	/* Right Shift */
	/* 230 */ KEY_RIGHTALT,	/* Right Alt, AKA AltGr */
	/* 231 */ KEY_RIGHTMETA,	/* Right Meta */
};
#define WS_USB_MAP_SIZE (sizeof(wsUsbMap)/sizeof(*wsUsbMap))

static uint32_t wsXtMap[] = {
	/* 0 */ KEY_RESERVED,
	/* 1 */ KEY_ESC,
	/* 2 */ KEY_1,
	/* 3 */ KEY_2,
	/* 4 */ KEY_3,
	/* 5 */ KEY_4,
	/* 6 */ KEY_5,
	/* 7 */ KEY_6,
	/* 8 */ KEY_7,
	/* 9 */ KEY_8,
	/* 10 */ KEY_9,
	/* 11 */ KEY_0,
	/* 12 */ KEY_MINUS,
	/* 13 */ KEY_EQUAL,
	/* 14 */ KEY_BACKSPACE,
	/* 15 */ KEY_TAB,
	/* 16 */ KEY_Q,
	/* 17 */ KEY_W,
	/* 18 */ KEY_E,
	/* 19 */ KEY_R,
	/* 20 */ KEY_T,
	/* 21 */ KEY_Y,
	/* 22 */ KEY_U,
	/* 23 */ KEY_I,
	/* 24 */ KEY_O,
	/* 25 */ KEY_P,
	/* 26 */ KEY_LEFTBRACE,
	/* 27 */ KEY_RIGHTBRACE,
	/* 28 */ KEY_ENTER,
	/* 29 */ KEY_LEFTCTRL,
	/* 30 */ KEY_A,
	/* 31 */ KEY_S,
	/* 32 */ KEY_D,
	/* 33 */ KEY_F,
	/* 34 */ KEY_G,
	/* 35 */ KEY_H,
	/* 36 */ KEY_J,
	/* 37 */ KEY_K,
	/* 38 */ KEY_L,
	/* 39 */ KEY_SEMICOLON,
	/* 40 */ KEY_APOSTROPHE,
	/* 41 */ KEY_GRAVE,
	/* 42 */ KEY_LEFTSHIFT,
	/* 43 */ KEY_BACKSLASH,
	/* 44 */ KEY_Z,
	/* 45 */ KEY_X,
	/* 46 */ KEY_C,
	/* 47 */ KEY_V,
	/* 48 */ KEY_B,
	/* 49 */ KEY_N,
	/* 50 */ KEY_M,
	/* 51 */ KEY_COMMA,
	/* 52 */ KEY_DOT,
	/* 53 */ KEY_SLASH,
	/* 54 */ KEY_RIGHTSHIFT,
	/* 55 */ KEY_KPASTERISK,
	/* 56 */ KEY_LEFTALT,
	/* 57 */ KEY_SPACE,
	/* 58 */ KEY_CAPSLOCK,
	/* 59 */ KEY_F1,
	/* 60 */ KEY_F2,
	/* 61 */ KEY_F3,
	/* 62 */ KEY_F4,
	/* 63 */ KEY_F5,
	/* 64 */ KEY_F6,
	/* 65 */ KEY_F7,
	/* 66 */ KEY_F8,
	/* 67 */ KEY_F9,
	/* 68 */ KEY_F10,
	/* 69 */ KEY_NUMLOCK,
	/* 70 */ KEY_SCROLLLOCK,
	/* 71 */ KEY_KP7,
	/* 72 */ KEY_KP8,
	/* 73 */ KEY_KP9,
	/* 74 */ KEY_KPMINUS,
	/* 75 */ KEY_KP4,
	/* 76 */ KEY_KP5,
	/* 77 */ KEY_KP6,
	/* 78 */ KEY_KPPLUS,
	/* 79 */ KEY_KP1,
	/* 80 */ KEY_KP2,
	/* 81 */ KEY_KP3,
	/* 82 */ KEY_KP0,
	/* 83 */ KEY_KPDOT,
	/* 84 */ KEY_RESERVED,

	/* 85 */ KEY_ZENKAKUHANKAKU,
	/* 86 */ KEY_102ND,	/* backslash on uk, < on german */
	/* 87 */ KEY_F11,
	/* 88 */ KEY_F12,
	/* 89 */ KEY_RESERVED,
	/* 90 */ KEY_RESERVED,
	/* 91 */ KEY_RESERVED,
	/* 92 */ KEY_RESERVED,
	/* 93 */ KEY_RESERVED,
	/* 94 */ KEY_RESERVED,
	/* 95 */ KEY_RESERVED,
	/* 96 */ KEY_RESERVED,
	/* 97 */ KEY_RESERVED,
	/* 98 */ KEY_RESERVED,
	/* 99 */ KEY_RESERVED,
	/* 100 */ KEY_RESERVED,
	/* 101 */ KEY_RESERVED,
	/* 102 */ KEY_RESERVED,
	/* 103 */ KEY_RESERVED,
	/* 104 */ KEY_RESERVED,
	/* 105 */ KEY_RESERVED,
	/* 106 */ KEY_RESERVED,
	/* 107 */ KEY_RESERVED,
	/* 108 */ KEY_RESERVED,
	/* 109 */ KEY_RESERVED,
	/* 110 */ KEY_RESERVED,
	/* 111 */ KEY_BRIGHTNESSUP,
	/* 112 */ KEY_BRIGHTNESSDOWN,
	/* 113 */ KEY_RESERVED,
	/* 114 */ KEY_RESERVED,
	/* 115 */ KEY_RESERVED,
	/* 116 */ KEY_RESERVED,
	/* 117 */ KEY_RESERVED,
	/* 118 */ KEY_RESERVED,
	/* 119 */ KEY_RESERVED,
	/* 120 */ KEY_RESERVED,
	/* 121 */ KEY_RESERVED,
	/* 122 */ KEY_RESERVED,
	/* 123 */ KEY_RESERVED,
	/* 124 */ KEY_RESERVED,
	/* 125 */ KEY_RESERVED,
	/* 126 */ KEY_RESERVED,
	/* 127 */ KEY_PAUSE,
	/* 128 */ KEY_RESERVED,
	/* 129 */ KEY_RESERVED,
	/* 130 */ KEY_RESERVED,
	/* 131 */ KEY_RESERVED,
	/* 132 */ KEY_RESERVED,
	/* 133 */ KEY_RESERVED,
	/* 134 */ KEY_RESERVED,
	/* 135 */ KEY_RESERVED,
	/* 136 */ KEY_RESERVED,
	/* 137 */ KEY_RESERVED,
	/* 138 */ KEY_RESERVED,
	/* 139 */ KEY_RESERVED,
	/* 140 */ KEY_RESERVED,
	/* 141 */ KEY_RESERVED,
	/* 142 */ KEY_RESERVED,
	/* 143 */ KEY_RESERVED,
	/* 144 */ KEY_PREVIOUSSONG,
	/* 145 */ KEY_RESERVED,
	/* 146 */ KEY_RESERVED,
	/* 147 */ KEY_RESERVED,
	/* 148 */ KEY_RESERVED,
	/* 149 */ KEY_RESERVED,
	/* 150 */ KEY_RESERVED,
	/* 151 */ KEY_RESERVED,
	/* 152 */ KEY_RESERVED,
	/* 153 */ KEY_NEXTSONG,
	/* 154 */ KEY_RESERVED,
	/* 155 */ KEY_RESERVED,
	/* 156 */ KEY_KPENTER,
	/* 157 */ KEY_RIGHTCTRL,
	/* 158 */ KEY_RESERVED,
	/* 159 */ KEY_RESERVED,
	/* 160 */ KEY_MUTE,
	/* 161 */ KEY_RESERVED,
	/* 162 */ KEY_PLAYPAUSE,
	/* 163 */ KEY_RESERVED,
	/* 164 */ KEY_RESERVED,
	/* 165 */ KEY_RESERVED,
	/* 166 */ KEY_RESERVED,
	/* 167 */ KEY_RESERVED,
	/* 168 */ KEY_RESERVED,
	/* 169 */ KEY_RESERVED,
	/* 170 */ KEY_PRINT,
	/* 171 */ KEY_RESERVED,
	/* 172 */ KEY_RESERVED,
	/* 173 */ KEY_RESERVED,
	/* 174 */ KEY_VOLUMEDOWN,
	/* 175 */ KEY_RESERVED,
	/* 176 */ KEY_VOLUMEUP,
	/* 177 */ KEY_RESERVED,
	/* 178 */ KEY_RESERVED,
	/* 179 */ KEY_RESERVED,
	/* 180 */ KEY_RESERVED,
	/* 181 */ KEY_KPSLASH,
	/* 182 */ KEY_RESERVED,
	/* 183 */ KEY_PRINT,
	/* 184 */ KEY_RIGHTALT,
	/* 185 */ KEY_RESERVED,
	/* 186 */ KEY_RESERVED,
	/* 187 */ KEY_RESERVED,
	/* 188 */ KEY_RESERVED,
	/* 189 */ KEY_RESERVED,
	/* 190 */ KEY_RESERVED,
	/* 191 */ KEY_RESERVED,
	/* 192 */ KEY_RESERVED,
	/* 193 */ KEY_RESERVED,
	/* 194 */ KEY_RESERVED,
	/* 195 */ KEY_RESERVED,
	/* 196 */ KEY_RESERVED,
	/* 197 */ KEY_RESERVED,
	/* 198 */ KEY_RESERVED,
	/* 199 */ KEY_HOME,
	/* 200 */ KEY_UP,
	/* 201 */ KEY_PAGEUP,
	/* 202 */ KEY_RESERVED,
	/* 203 */ KEY_LEFT,
	/* 204 */ KEY_RESERVED,
	/* 205 */ KEY_RIGHT,
	/* 206 */ KEY_RESERVED,
	/* 207 */ KEY_END,
	/* 208 */ KEY_DOWN,
	/* 209 */ KEY_PAGEDOWN,
	/* 210 */ KEY_INSERT,
	/* 211 */ KEY_DELETE,
	/* 212 */ KEY_RESERVED,
	/* 213 */ KEY_RESERVED,
	/* 214 */ KEY_RESERVED,
	/* 215 */ KEY_RESERVED,
	/* 216 */ KEY_RESERVED,
	/* 217 */ KEY_RESERVED,
	/* 218 */ KEY_RESERVED,
	/* 219 */ KEY_LEFTMETA,
	/* 220 */ KEY_RIGHTMETA,
	/* 221 */ KEY_MENU,
	/* 222 */ KEY_RESERVED,
	/* 223 */ KEY_RESERVED,
	/* 224 */ KEY_RESERVED,
	/* 225 */ KEY_RESERVED,
	/* 226 */ KEY_RESERVED,
	/* 227 */ KEY_RESERVED,
	/* 228 */ KEY_RESERVED,
	/* 229 */ KEY_RESERVED,
	/* 230 */ KEY_RESERVED,
	/* 231 */ KEY_RESERVED,
	/* 232 */ KEY_RESERVED,
	/* 233 */ KEY_RESERVED,
	/* 234 */ KEY_RESERVED,
	/* 235 */ KEY_RESERVED,
	/* 236 */ KEY_RESERVED,
	/* 237 */ KEY_SETUP,
};
#define WS_XT_MAP_SIZE (sizeof(wsXtMap)/sizeof(*wsXtMap))

uint32_t
wskey_transcode(int wskbd_type, int wskey)
{
	switch (wskbd_type) {
	case WSKBD_TYPE_PC_XT:
	case WSKBD_TYPE_PC_AT:
		if (wskey < 0 || wskey >= (int)WS_XT_MAP_SIZE)
			return KEY_UNKNOWN;
		return wsXtMap[wskey];
	case WSKBD_TYPE_USB:
		if (wskey < 0 || wskey >= (int)WS_USB_MAP_SIZE)
			return KEY_UNKNOWN;
		return wsUsbMap[wskey];
	default:
		return KEY_RESERVED;
	}
}
