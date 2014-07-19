/* See LICENSE file for copyright and license details. */

/* commands executed at init */
const char *initexecs[] = { 
	"exec xsetroot -mod 0 0",
//	"exec xsetbg /path/to/my/cool/image",
	NULL
};

#define XTERM_EXEC		"exec rxterm"

#define PROC_DWM		"/tmp/proc_awm"

/* appearance */
#define MAX_NVX 		64
#define BARPOS			BarOff /* BarTop, BarBot, BarOff */
#define BORDERPX		1
#define FONT			"-misc-fixed-medium-r-normal--20-*-*-*-*-*-*-1"
#define NORMBORDERCOLOR		"#000"
#define SELBORDERCOLOR		"#333"
#define NORMBGCOLOR		"#000"
#define NORMFGCOLOR		"#fff"
#define SELBGCOLOR		"#555"
#define SELFGCOLOR		"#fff"

#define RESIZEHINTS		True	/* False - respect size hints in tiled resizals */
#define MWFACT			0.6	/* master width factor [0.1 .. 0.9] */
#define SNAP			32	/* snap pixel */

/* key definitions */
#define MODKEY			Mod1Mask
#define CTRLKEY			ControlMask
#define SHIFTKEY		ShiftMask

#define KEYS \
Key keys[] = { \
	/* modifier			key		function	argument */ \
	{ MODKEY|CTRLKEY|SHIFTKEY,	XK_q,		quit,		NULL }, \
\
	{ CTRLKEY|MODKEY,	XK_n,           spawn,  XTERM_EXEC " -e /home/alpt/scripts/nihil" }, \
	{ CTRLKEY|MODKEY,	XK_bracketright,spawn,  XTERM_EXEC }, \
	{ CTRLKEY|MODKEY,	XK_h,		spawn,  XTERM_EXEC }, \
	{ CTRLKEY|MODKEY,	XK_f,		spawn,	"exec firefox" }, \
	{ CTRLKEY|MODKEY,	XK_m,		spawn,	XTERM_EXEC " -e ssh -t nihil mutt -Z" }, \
	{ CTRLKEY|MODKEY,	XK_b,		togglebar,	NULL }, \
	{ MODKEY,		XK_less, 	focusnext,	NULL }, \
	{ MODKEY,		XK_slash,	focusprev,	NULL }, \
	{ MODKEY,		XK_Tab,		focusswitch,	NULL }, \
	{ CTRLKEY|MODKEY,	XK_Tab,		toggletile,	NULL }, \
	{ MODKEY,			XK_Up,		togglemax,	NULL }, \
	{ MODKEY,			XK_c,		killclient,	NULL }, \
\
	{ MODKEY|CTRLKEY,		XK_Left,	prevviewY,	NULL }, \
	{ MODKEY|CTRLKEY,		XK_Right,	nextviewY,	NULL }, \
\
	{ MODKEY|SHIFTKEY,		XK_Down,	prevviewX,	NULL }, \
	{ MODKEY|SHIFTKEY,		XK_Up,		nextviewX,	NULL }, \
\
	{ MODKEY|CTRLKEY,		XK_d,		delviewY,	NULL }, \
	{ MODKEY|SHIFTKEY,		XK_d,		delviewX,	NULL }, \
\
	{ MODKEY|CTRLKEY,		XK_Return,	insertviewY,	NULL }, \
	{ MODKEY|SHIFTKEY,		XK_Return,	insertviewX,	NULL }, \
\
	{ MODKEY,			XK_F1,		viewY,		"0" }, \
	{ MODKEY,			XK_F2,		viewY,		"1" }, \
	{ MODKEY,			XK_F3,		viewY,		"2" }, \
	{ MODKEY,			XK_F4,		viewY,		"3" }, \
	{ MODKEY,			XK_F5,		viewY,		"4" }, \
	{ MODKEY,			XK_F6,		viewY,		"5" }, \
	{ MODKEY,			XK_F7,		viewY,		"6" }, \
	{ MODKEY,			XK_F8,		viewY,		"7" }, \
	{ MODKEY,			XK_F9,		viewY,		"8" }, \
	{ MODKEY,			XK_F10,		viewY,		"9" }, \
	{ MODKEY,			XK_F11,		viewY,		"10" }, \
	{ MODKEY,			XK_F12,		viewY,		"11" }, \
	\
	{ MODKEY|SHIFTKEY,		XK_F1,		viewX,		"0" }, \
	{ MODKEY|SHIFTKEY,		XK_F2,		viewX,		"1" }, \
	{ MODKEY|SHIFTKEY,		XK_F3,		viewX,		"2" }, \
	{ MODKEY|SHIFTKEY,		XK_F4,		viewX,		"3" }, \
	{ MODKEY|SHIFTKEY,		XK_F5,		viewX,		"4" }, \
	{ MODKEY|SHIFTKEY,		XK_F6,		viewX,		"5" }, \
	{ MODKEY|SHIFTKEY,		XK_F7,		viewX,		"6" }, \
	{ MODKEY|SHIFTKEY,		XK_F8,		viewX,		"7" }, \
	{ MODKEY|SHIFTKEY,		XK_F9,		viewX,		"8" }, \
	{ MODKEY|SHIFTKEY,		XK_F10,		viewX,		"9" }, \
	{ MODKEY|SHIFTKEY,		XK_F11,		viewX,		"10" }, \
	{ MODKEY|SHIFTKEY,		XK_F12,		viewX,		"11" }, \
\
\
	{ MODKEY|SHIFTKEY|CTRLKEY,		XK_Up,		setwindownextviewX,	NULL }, \
	{ MODKEY|SHIFTKEY|CTRLKEY,		XK_Down,	setwindowprevviewX,	NULL }, \
	{ MODKEY|SHIFTKEY|CTRLKEY,		XK_Left,	setwindowprevviewY,	NULL }, \
	{ MODKEY|SHIFTKEY|CTRLKEY,		XK_Right,	setwindownextviewY,	NULL }, \
\
	{ MODKEY|SHIFTKEY,		XK_1,		setwindowviewX,		"0" }, \
	{ MODKEY|SHIFTKEY,		XK_2,		setwindowviewX,		"1" }, \
	{ MODKEY|SHIFTKEY,		XK_3,		setwindowviewX,		"2" }, \
	{ MODKEY|SHIFTKEY,		XK_4,		setwindowviewX,		"3" }, \
	{ MODKEY|SHIFTKEY,		XK_5,		setwindowviewX,		"4" }, \
	{ MODKEY|SHIFTKEY,		XK_6,		setwindowviewX,		"5" }, \
	{ MODKEY|SHIFTKEY,		XK_7,		setwindowviewX,		"6" }, \
	{ MODKEY|SHIFTKEY,		XK_8,		setwindowviewX,		"7" }, \
	{ MODKEY|SHIFTKEY,		XK_9,		setwindowviewX,		"8" }, \
	{ MODKEY|SHIFTKEY,		XK_0,		setwindowviewX,		"9" }, \
\
	{ MODKEY|CTRLKEY,		XK_1,		setwindowviewY,		"0" }, \
	{ MODKEY|CTRLKEY,		XK_2,		setwindowviewY,		"1" }, \
	{ MODKEY|CTRLKEY,		XK_3,		setwindowviewY,		"2" }, \
	{ MODKEY|CTRLKEY,		XK_4,		setwindowviewY,		"3" }, \
	{ MODKEY|CTRLKEY,		XK_5,		setwindowviewY,		"4" }, \
	{ MODKEY|CTRLKEY,		XK_6,		setwindowviewY,		"5" }, \
	{ MODKEY|CTRLKEY,		XK_7,		setwindowviewY,		"6" }, \
	{ MODKEY|CTRLKEY,		XK_8,		setwindowviewY,		"7" }, \
	{ MODKEY|CTRLKEY,		XK_9,		setwindowviewY,		"8" }, \
	{ MODKEY|CTRLKEY,		XK_0,		setwindowviewY,		"9" }, \
};
