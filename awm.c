/* 
 * http://freaknet.org/alpt/src/alpt-wm
 * Sat Sep 22 19:33:00 CEST 2007
 *
 * This is my own window manager.
 * I cannot thank enough the suckless.org people. \o/ THANKS!!! \o/
 *
 * Here it is the original window manager:
 * http://www.suckless.org/wiki/dwm
 *
 * - AlpT (@freaknet.org)
 */
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

/* macros */
#define BUTTONMASK		(ButtonPressMask | ButtonReleaseMask)
#define CLEANMASK(mask)		(mask & ~(numlockmask | LockMask))
#define MOUSEMASK		(BUTTONMASK | PointerMotionMask)

/* enums */
enum { BarTop, BarBot, BarOff };			/* bar position */
enum { CurNormal, CurResize, CurMove, CurLast };	/* cursor */
enum { ColBorder, ColFG, ColBG, ColLast };		/* color */
enum { NetSupported, NetWMName, NetLast };		/* EWMH atoms */
enum { WMProtocols, WMDelete, WMName, WMState, WMLast };/* default atoms */

/* typedefs */
typedef struct Client Client;
struct Client {
	char name[256];
	int x, y, w, h;
	int rx, ry, rw, rh; /* revert geometry */
	int rex, rey, rew, reh; /* resized geometry */
	int px, py, pw, ph; /* pre-tile geometry */
	Bool isresized;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int minax, maxax, minay, maxay;
	long flags; 
	unsigned int border, oldborder;
	Bool isbanned, isfixed, ismax, isfloating, wasfloating;
	int vx, vy;
	Client *next;
	Client *prev;
	Client *snext;
	Window win;
};

typedef struct {
	int x, y, w, h;
	unsigned long norm[ColLast];
	unsigned long sel[ColLast];
	Drawable drawable;
	GC gc;
	struct {
		int ascent;
		int descent;
		int height;
		XFontSet set;
		XFontStruct *xfont;
	} font;
} DC; /* draw context */

typedef struct {
	unsigned long mod;
	KeySym keysym;
	void (*func)(const char *arg);
	const char *arg;
} Key;

/* forward declarations */
void arrange(void);
void attach(Client *c);
void attachstack(Client *c);
void ban(Client *c);
void buttonpress(XEvent *e);
void checkotherwm(void);
void cleanup(void);
void configure(Client *c);
void configurenotify(XEvent *e);
void configurerequest(XEvent *e);
void destroynotify(XEvent *e);
void detach(Client *c);
void detachstack(Client *c);
void drawbar(void);
void drawtext(const char *text, unsigned long col[ColLast]);
void *emallocz(unsigned int size);
void enternotify(XEvent *e);
void eprint(const char *errstr, ...);
void expose(XEvent *e);
void floating(void); /* default floating layout */
void focus(Client *c);
void focusnext(const char *arg);
void focusprev(const char *arg);
Client *getclient(Window w);
unsigned long getcolor(const char *colstr);
long getstate(Window w);
Bool gettextprop(Window w, Atom atom, char *text, unsigned int size);
void grabbuttons(Window w, Bool focused);
void initfont(const char *fontstr);
Bool isprotodel(Client *c);
Bool isvisible(Client *c);
void keypress(XEvent *e);
void killclient(const char *arg);
void leavenotify(XEvent *e);
void manage(Window w, XWindowAttributes *wa);
void mappingnotify(XEvent *e);
void maprequest(XEvent *e);
void movemouse(Client *c);
void propertynotify(XEvent *e);
void quit(const char *arg);
void snapit(Client *c, int *Nx, int *Ny);
void resize(Client *c, int x, int y, int w, int h, Bool sizehints);
void resizemouse(Client *c);
void restack(void);
void run(void);
void scan(void);
void setclientstate(Client *c, long state);
void setup(void);
void spawn(const char *arg);
unsigned int textnw(const char *text, unsigned int len);
unsigned int textw(const char *text);
void togglebar(const char *arg);
void togglefloating(const char *arg);
void togglemax(const char *arg);
void toggleview(const char *arg);
void revertpretilegeom(const char *arg);
void unban(Client *c);
void unmanage(Client *c);
void unmapnotify(XEvent *e);
void updatebarpos(void);
void updatesizehints(Client *c);
void updatetitle(Client *c);
void viewX(const char *arg);
void nextviewX(const char *arg);
void prevviewX(const char *arg);
void delviewX(const char *arg);
void insertviewX(const char *arg);
void viewY(const char *arg);
void nextviewY(const char *arg);
void prevviewY(const char *arg);
void delviewY(const char *arg);
void insertviewY(const char *arg);
void setwindowviewX(const char *arg);
void setwindowviewY(const char *arg);
void setwindownextviewX(const char *arg);
void setwindownextviewY(const char *arg);
void setwindowprevviewX(const char *arg);
void setwindowprevviewY(const char *arg);
void spawn_pmpterm(const char *arg);
void dumpwindowtitles(void);

void tile(const char *arg);
void toggletile(const char *arg);

int xerror(Display *dpy, XErrorEvent *ee);
int xerrordummy(Display *dsply, XErrorEvent *ee);
int xerrorstart(Display *dsply, XErrorEvent *ee);

/* variables */
char stext[256];
double mwfact;
int screen, sx, sy, sw, sh, wax, way, waw, wah;
int (*xerrorxlib)(Display *, XErrorEvent *);
unsigned int bh, bpos;
unsigned int blw = 0;
unsigned int numlockmask = 0;
void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[LeaveNotify] = leavenotify,
	[Expose] = expose,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
Atom wmatom[WMLast], netatom[NetLast];
Bool otherwm, readin;
Bool running = True;
Bool selscreen = True;
Client *clients = NULL;
Client *sel = NULL;
Client *stack = NULL;
Cursor cursor[CurLast];
Display *dpy;
DC dc = {0};
Window barwin, root;
Bool dont_spawn = False, last_tiled;

/* configuration, allows nested code to access above variables */
#include "config.h"

/* V space */
int selvx, selvy;
int nvx, nvy[MAX_NVX]; /* greatest Vx/Vy in space */
int lastselvy[MAX_NVX];


/**
 * Preemptive Terminal 
 */
#define PMPTERM_TITLE	 	"__pmpterm"
#define PREEMPTIVE_TERM 	XTERM_EXEC " -title \"" PMPTERM_TITLE "\""
Client *pmpterm=0;
int pmpterm_spawned=0;

void spawn_pmpterm(const char *arg)
{
	if(!pmpterm) {
		if(!pmpterm_spawned)
			spawn(PREEMPTIVE_TERM);
		return;
	}

	pmpterm->vx=selvx;
	pmpterm->vy=selvy;
	strcpy(pmpterm->name, "terminal");
	unban(pmpterm);
	resize(pmpterm, pmpterm->x, pmpterm->y, pmpterm->w, pmpterm->h, True);
	dumpwindowtitles();
	if(!pmpterm_spawned) {
		spawn(PREEMPTIVE_TERM);
		pmpterm_spawned=1;
	}
}
/**/


/* functions*/
void
arrange(void) {
	Client *c;

	for(c = clients; c; c = c->next)
		if(isvisible(c))
			unban(c);
		else
			ban(c);
	floating();
	focus(NULL);
	restack();
}

void
attach(Client *c) {
	if(clients)
		clients->prev = c;
	c->next = clients;
	clients = c;
}

void
attachstack(Client *c) {
	c->snext = stack;
	stack = c;
}

void
ban(Client *c) {
	if(c->isbanned)
		return;
	XMoveWindow(dpy, c->win, c->x + 2 * sw, c->y);
	c->isbanned = True;
}

void
buttonpress(XEvent *e) {
	Client *c=0;
	XButtonPressedEvent *ev = &e->xbutton;


	if(!(c = getclient(ev->window)))
		c=sel;
	if(c) {
		if(CLEANMASK(ev->state) == MODKEY && ev->button == Button3 && !c->isfixed) {
			resizemouse(sel);
			return;
		}
		
		focus(c);
		if(CLEANMASK(ev->state) == MODKEY && ev->button == Button1) {
			restack();
			movemouse(c);
		}
	}
}

void
checkotherwm(void) {
	otherwm = False;
	XSetErrorHandler(xerrorstart);

	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, root, SubstructureRedirectMask);
	XSync(dpy, False);
	if(otherwm)
		eprint("dwm: another window manager is already running\n");
	XSync(dpy, False);
	XSetErrorHandler(NULL);
	xerrorxlib = XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void) {
	close(STDIN_FILENO);
	while(stack) {
		unban(stack);
		unmanage(stack);
	}
	if(dc.font.set)
		XFreeFontSet(dpy, dc.font.set);
	else
		XFreeFont(dpy, dc.font.xfont);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XFreePixmap(dpy, dc.drawable);
	XFreeGC(dpy, dc.gc);
	XDestroyWindow(dpy, barwin);
	XFreeCursor(dpy, cursor[CurNormal]);
	XFreeCursor(dpy, cursor[CurResize]);
	XFreeCursor(dpy, cursor[CurMove]);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XSync(dpy, False);
}

void
configure(Client *c) {
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->border;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e) {
	XConfigureEvent *ev = &e->xconfigure;

	if(ev->window == root && (ev->width != sw || ev->height != sh)) {
		sw = ev->width;
		sh = ev->height;
		XFreePixmap(dpy, dc.drawable);
		dc.drawable = XCreatePixmap(dpy, root, sw, bh, DefaultDepth(dpy, screen));
		XResizeWindow(dpy, barwin, sw, bh);
		updatebarpos();
		arrange();
	}
}

void
configurerequest(XEvent *e) {
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if((c = getclient(ev->window))) {
		c->ismax = False;
		if(ev->value_mask & CWBorderWidth)
			c->border = ev->border_width;
		if(ev->value_mask & CWX)
			c->x = ev->x;
		if(ev->value_mask & CWY)
			c->y = ev->y;
		if(ev->value_mask & CWWidth)
			c->w = ev->width;
		if(ev->value_mask & CWHeight)
			c->h = ev->height;
		if((c->x + c->w) > sw && c->isfloating)
			c->x = sw / 2 - c->w / 2; /* center in x direction */
		if((c->y + c->h) > sh && c->isfloating)
			c->y = sh / 2 - c->h / 2; /* center in y direction */
		if((ev->value_mask & (CWX | CWY))
				&& !(ev->value_mask & (CWWidth | CWHeight)))
			configure(c);
		if(isvisible(c))
			XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	}
	else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

void
destroynotify(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if((c = getclient(ev->window)))
		unmanage(c);
}

void
detach(Client *c) {
	if(c->prev)
		c->prev->next = c->next;
	if(c->next)
		c->next->prev = c->prev;
	if(c == clients)
		clients = c->next;
	c->next = c->prev = NULL;
}

void
detachstack(Client *c) {
	Client **tc;

	for(tc=&stack; *tc && *tc != c; tc=&(*tc)->snext);
	*tc = c->snext;
}

void
drawbar(void) {
	int x;
	char buf[32];

	dc.x = dc.y = 0;

	if(bpos == BarOff)
		return;

	sprintf(buf, "X %d, Y %d", selvx+1, selvy+1);
	dc.w = textw(buf);
	drawtext(buf, dc.sel);
	dc.x += dc.w;
	dc.w = blw;
	x = dc.x + dc.w;
	dc.w = textw(stext);
	dc.x = sw - dc.w;
	if(dc.x < x) {
		dc.x = x;
		dc.w = sw - x;
	}
	drawtext(stext, dc.sel);
	if((dc.w = dc.x - x) > bh) {
		dc.x = x;
		if(sel)
			drawtext(sel->name, dc.sel);
		else
			drawtext(NULL, dc.norm);
	}
	XCopyArea(dpy, dc.drawable, barwin, dc.gc, 0, 0, sw, bh, 0, 0);
	XSync(dpy, False);
}

void
drawtext(const char *text, unsigned long col[ColLast]) {
	int x, y, w, h;
	static char buf[256];
	unsigned int len, olen;
	XRectangle r = { dc.x, dc.y, dc.w, dc.h };

	XSetForeground(dpy, dc.gc, col[ColBG]);
	XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	if(!text)
		return;
	w = 0;
	olen = len = strlen(text);
	if(len >= sizeof buf)
		len = sizeof buf - 1;
	memcpy(buf, text, len);
	buf[len] = 0;
	h = dc.font.ascent + dc.font.descent;
	y = dc.y + (dc.h / 2) - (h / 2) + dc.font.ascent;
	x = dc.x + (h / 2);
	/* shorten text if necessary */
	while(len && (w = textnw(buf, len)) > dc.w - h)
		buf[--len] = 0;
	if(len < olen) {
		if(len > 1)
			buf[len - 1] = '.';
		if(len > 2)
			buf[len - 2] = '.';
		if(len > 3)
			buf[len - 3] = '.';
	}
	if(w > dc.w)
		return; /* too long */
	XSetForeground(dpy, dc.gc, col[ColFG]);
	if(dc.font.set)
		XmbDrawString(dpy, dc.drawable, dc.font.set, dc.gc, x, y, buf, len);
	else
		XDrawString(dpy, dc.drawable, dc.gc, x, y, buf, len);
}

void *
emallocz(unsigned int size) {
	void *res = calloc(1, size);

	if(!res)
		eprint("fatal: could not malloc() %u bytes\n", size);
	return res;
}

void
eprint(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
expose(XEvent *e) {
	XExposeEvent *ev = &e->xexpose;

	if(ev->count == 0) {
		if(barwin == ev->window)
			drawbar();
	}
}

void
floating(void) { /* default floating layout */
	Client *c;
	int at_least_one=0;
	for(c = clients; c; c = c->next)
		if(isvisible(c)) {
			resize(c, c->x, c->y, c->w, c->h, True);
			at_least_one=1;
		}
	if(!at_least_one && !dont_spawn)
		spawn_pmpterm(0);
}


/* function taken from the original dwm.c, but modified a bit for the
 * toggletile() function */
void tile(const char *arg) {
	unsigned int i, n, nx, ny, nw, nh, mw, th;
	Client *c;

	for(n = 0, c = clients; c; c = c->next)
		if(isvisible(c))
			n++;

	/* window geoms */
	mw = (n == 1) ? waw : mwfact * waw;
	th = (n > 1) ? wah / (n - 1) : 0;
	if(n > 1 && th < bh)
		th = wah;

	nx = wax;
	ny = way;
	for(i = 0, c = sel ? sel : clients; 
	    c && i < n; 
	    c = !c->snext ? stack : c->snext) {
		if(!isvisible(c))
			continue;
//		c->ismax = False;
		if(i == 0) { /* master */
			nw = mw - 2 * c->border;
			nh = wah - 2 * c->border;
		}
		else {  /* tile window */
			if(i == 1) {
				ny = way;
				nx += mw;
			}
			nw = waw - mw - 2 * c->border;
			if(i + 1 == n) /* remainder */
				nh = (way + wah) - ny - 2 * c->border;
			else
				nh = th - 2 * c->border;
		}
		c->px=c->x; c->py=c->y; c->pw=c->w; c->ph=c->h;
		resize(c, nx, ny, nw, nh, False);
		if(n > 1 && th != wah)
			ny += nh + 2 * c->border;
		i++;
	}
}

void
focus(Client *c) {
	if((!c && selscreen) || (c && !isvisible(c)))
		for(c = stack; c && !isvisible(c); c = c->snext);
	if(sel && sel != c) {
		grabbuttons(sel->win, False);
		XSetWindowBorder(dpy, sel->win, dc.norm[ColBorder]);
	}
	if(c) {
		detachstack(c);
		attachstack(c);
		grabbuttons(c->win, True);
	}
	sel = c;
	drawbar();
	if(!selscreen)
		return;
	if(c) {
		XSetWindowBorder(dpy, c->win, dc.sel[ColBorder]);
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
	} else
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
}

void
focusnext(const char *arg) {
	Client *c;

	if(!sel)
		return;
	for(c = sel->next; c && !isvisible(c); c = c->next);
	if(!c)
		for(c = clients; c && !isvisible(c); c = c->next);
	if(c) {
		focus(c);
		restack();
	}
}

void
focusprev(const char *arg) {
	Client *c;

	if(!sel)
		return;

	for(c = sel->prev; c && !isvisible(c); c = c->prev);
	if(!c) {
		for(c = clients; c && c->next; c = c->next);
		for(; c && !isvisible(c); c = c->prev);
	}
	if(c) {
		focus(c);
		restack();
	}
}

void focusswitch(const char *arg) {
	Client *c;
	if(!stack)
		return;
	for(c = stack->snext; c && !isvisible(c); c = c->snext);
	if(c) {
		focus(c);
		restack();
	}
}

void toggletile(const char *arg){
	if(!last_tiled)
		tile(0);
	else {
		revertpretilegeom(0);
		floating();
	}
	last_tiled=!last_tiled;
	focus(sel);
	restack();
}

Client *
getclient(Window w) {
	Client *c;

	for(c = clients; c && c->win != w; c = c->next);
	return c;
}

unsigned long
getcolor(const char *colstr) {
	Colormap cmap = DefaultColormap(dpy, screen);
	XColor color;

	if(!XAllocNamedColor(dpy, cmap, colstr, &color, &color))
		eprint("error, cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

long
getstate(Window w) {
	int format, status;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	status = XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
			&real, &format, &n, &extra, (unsigned char **)&p);
	if(status != Success)
		return -1;
	if(n != 0)
		result = *p;
	XFree(p);
	return result;
}

Bool
gettextprop(Window w, Atom atom, char *text, unsigned int size) {
	char **list = NULL;
	int n;
	XTextProperty name;

	if(!text || size == 0)
		return False;
	text[0] = '\0';
	XGetTextProperty(dpy, w, &name, atom);
	if(!name.nitems)
		return False;
	if(name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if(XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success
		&& n > 0 && *list)
		{
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return True;
}

void
grabbuttons(Window w, Bool focused) {
	XUngrabButton(dpy, AnyButton, AnyModifier, w);

	if(focused) {
		XGrabButton(dpy, Button1, MODKEY, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button1, MODKEY | LockMask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button1, MODKEY | numlockmask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button1, MODKEY | numlockmask | LockMask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);

		XGrabButton(dpy, Button2, MODKEY, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button2, MODKEY | LockMask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button2, MODKEY | numlockmask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button2, MODKEY | numlockmask | LockMask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);

		XGrabButton(dpy, Button3, MODKEY, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button3, MODKEY | LockMask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button3, MODKEY | numlockmask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
		XGrabButton(dpy, Button3, MODKEY | numlockmask | LockMask, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
	}
	else
		XGrabButton(dpy, AnyButton, AnyModifier, w, False, BUTTONMASK,
				GrabModeAsync, GrabModeSync, None, None);
}

void
initfont(const char *fontstr) {
	char *def, **missing;
	int i, n;

	missing = NULL;
	if(dc.font.set)
		XFreeFontSet(dpy, dc.font.set);
	dc.font.set = XCreateFontSet(dpy, fontstr, &missing, &n, &def);
	if(missing) {
		while(n--)
			fprintf(stderr, "dwm: missing fontset: %s\n", missing[n]);
		XFreeStringList(missing);
	}
	if(dc.font.set) {
		XFontSetExtents *font_extents;
		XFontStruct **xfonts;
		char **font_names;
		dc.font.ascent = dc.font.descent = 0;
		font_extents = XExtentsOfFontSet(dc.font.set);
		n = XFontsOfFontSet(dc.font.set, &xfonts, &font_names);
		for(i = 0, dc.font.ascent = 0, dc.font.descent = 0; i < n; i++) {
			if(dc.font.ascent < (*xfonts)->ascent)
				dc.font.ascent = (*xfonts)->ascent;
			if(dc.font.descent < (*xfonts)->descent)
				dc.font.descent = (*xfonts)->descent;
			xfonts++;
		}
	}
	else {
		if(dc.font.xfont)
			XFreeFont(dpy, dc.font.xfont);
		dc.font.xfont = NULL;
		if(!(dc.font.xfont = XLoadQueryFont(dpy, fontstr))
		&& !(dc.font.xfont = XLoadQueryFont(dpy, "fixed")))
			eprint("error, cannot load font: '%s'\n", fontstr);
		dc.font.ascent = dc.font.xfont->ascent;
		dc.font.descent = dc.font.xfont->descent;
	}
	dc.font.height = dc.font.ascent + dc.font.descent;
}

Bool
isprotodel(Client *c) {
	int i, n;
	Atom *protocols;
	Bool ret = False;

	if(XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		for(i = 0; !ret && i < n; i++)
			if(protocols[i] == wmatom[WMDelete])
				ret = True;
		XFree(protocols);
	}
	return ret;
}

Bool
isvisible(Client *c) {
	return c->vx == selvx && c->vy == selvy;
}

void
keypress(XEvent *e) {
	KEYS
	unsigned int len = sizeof keys / sizeof keys[0];
	unsigned int i;
	KeyCode code;
	KeySym keysym;
	XKeyEvent *ev;

	if(!e) { /* grabkeys */
		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for(i = 0; i < len; i++) {
			code = XKeysymToKeycode(dpy, keys[i].keysym);
			XGrabKey(dpy, code, keys[i].mod, root, True,
					GrabModeAsync, GrabModeAsync);
			XGrabKey(dpy, code, keys[i].mod | LockMask, root, True,
					GrabModeAsync, GrabModeAsync);
			XGrabKey(dpy, code, keys[i].mod | numlockmask, root, True,
					GrabModeAsync, GrabModeAsync);
			XGrabKey(dpy, code, keys[i].mod | numlockmask | LockMask, root, True,
					GrabModeAsync, GrabModeAsync);
		}
		return;
	}
	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for(i = 0; i < len; i++)
		if(keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state))
		{
			if(keys[i].func)
				keys[i].func(keys[i].arg);
		}
}

void
killclient(const char *arg) {
	XEvent ev;

	if(!sel)
		return;
	if(isprotodel(sel)) {
		ev.type = ClientMessage;
		ev.xclient.window = sel->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = wmatom[WMDelete];
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, sel->win, False, NoEventMask, &ev);
	}
	else
		XKillClient(dpy, sel->win);
}

void
enternotify(XEvent *e) {
	XCrossingEvent *ev = &e->xcrossing;

	if(ev->mode != NotifyNormal)
		return;
	if(ev->window == root) {
		grabbuttons(root, True);
		selscreen = True;
	} 
	else {
		if(ev->detail != NotifyVirtual) {
			XUngrabButton(dpy, AnyButton, AnyModifier, root);
			XSync(dpy, False);
		}
	}
}

void
leavenotify(XEvent *e) {
	XCrossingEvent *ev = &e->xcrossing;

	if((ev->window == root) && !ev->same_screen) {
		selscreen = False;
		focus(NULL);
	}
}

void
manage(Window w, XWindowAttributes *wa) {
	Client *c, *t = NULL;
	Window trans;
	Status rettrans;
	XWindowChanges wc;

	c = emallocz(sizeof(Client));
	c->isresized=False;
	c->win = w;
	if(sel) {
		c->x = sel->x+32;
		c->y = sel->y+32;
	} else {
		c->x=sx+sw/4;
		c->y=sy+sh/4;
	}
	c->w = wa->width;
	c->h = wa->height;
	c->oldborder = wa->border_width;
	if(c->w == sw && c->h == sh) {
		c->x = sx;
		c->y = sy;
		c->border = wa->border_width;
	} else {
		if(c->x + c->w + 2 * c->border > wax + waw)
			c->x = wax + waw - c->w - 2 * c->border;
		if(c->y + c->h + 2 * c->border > way + wah)
			c->y = way + wah - c->h - 2 * c->border;
		if(c->x < wax)
			c->x = wax;
		if(c->y < way)
			c->y = way;
		c->border = BORDERPX;
	}
	wc.border_width = c->border;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, dc.norm[ColBorder]);
	configure(c); /* propagates border_width, if size doesn't change */
	updatesizehints(c);
	XSelectInput(dpy, w,
		StructureNotifyMask | PropertyChangeMask | EnterWindowMask);
	grabbuttons(c->win, True);
	updatetitle(c);
	if((rettrans = XGetTransientForHint(dpy, w, &trans) == Success))
		for(t = clients; t && t->win != trans; t = t->next);
	if(t) {
		c->vx=t->vx;
		c->vy=t->vy;
	}

	c->vx=selvx;
	c->vy=selvy;
	if(!c->isfloating)
		c->isfloating = (rettrans == Success) || c->isfixed;
	attach(c);
	attachstack(c);
	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h); /* some windows require this */
	ban(c);
	XMapWindow(dpy, c->win);
	setclientstate(c, NormalState);

	last_tiled=False;

	/** Preemptive terminal */
	if(!strcmp(c->name, PMPTERM_TITLE)) {
		/* It's just a preemptive terminal. Hide it */
		c->vx=c->vy=-1;
		pmpterm=c;
		c->x=sx+sw/4;
		c->y=sy+sh/4;
		pmpterm_spawned=0;
	}/**/

	arrange();
}

void
mappingnotify(XEvent *e) {
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if(ev->request == MappingKeyboard)
		keypress(NULL);
}

void
maprequest(XEvent *e) {
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if(!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if(wa.override_redirect)
		return;
	if(!getclient(ev->window))
		manage(ev->window, &wa);
}

void
movemouse(Client *c) {
	int x1, y1, ocx, ocy, di, nx, ny;
	unsigned int dui;
	Window dummy;
	XEvent ev;

	ocx = nx = c->x;
	ocy = ny = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, cursor[CurMove], CurrentTime) != GrabSuccess)
		return;
	c->ismax = False;
	XQueryPointer(dpy, root, &dummy, &dummy, &x1, &y1, &di, &di, &dui);
	for(;;) {
		XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ButtonRelease:
			XUngrabPointer(dpy, CurrentTime);
			if(c->isresized) {
				XSync(dpy, False);
				resize(c, c->rex, c->rey, c->rew, c->reh, False);
				c->isresized=False;
			}
			return;
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			XSync(dpy, False);
			nx = ocx + (ev.xmotion.x - x1);
			ny = ocy + (ev.xmotion.y - y1);
			snapit(c, &nx, &ny);
			c->isresized=True;
			c->rex=nx; c->rey=ny;
			c->rew=c->w; c->reh=c->h;
			break;
		}
	}
}

void
propertynotify(XEvent *e) {
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if(ev->state == PropertyDelete)
		return; /* ignore */
	if((c = getclient(ev->window))) {
		switch (ev->atom) {
			default: break;
			case XA_WM_TRANSIENT_FOR:
				XGetTransientForHint(dpy, c->win, &trans);
				if(!c->isfloating && (c->isfloating = (getclient(trans) != NULL)))
					arrange();
				break;
			case XA_WM_NORMAL_HINTS:
				updatesizehints(c);
				break;
		}
		if(ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if(c == sel)
				drawbar();
		}
	}
}

void
quit(const char *arg) {
	readin = running = False;
}

void
resize(Client *c, int x, int y, int w, int h, Bool sizehints) {
	double dx, dy, max, min, ratio;
	XWindowChanges wc; 

	if(sizehints) {
		if(c->minay > 0 && c->maxay > 0 && (h - c->baseh) > 0 && (w - c->basew) > 0) {
			dx = (double)(w - c->basew);
			dy = (double)(h - c->baseh);
			min = (double)(c->minax) / (double)(c->minay);
			max = (double)(c->maxax) / (double)(c->maxay);
			ratio = dx / dy;
			if(max > 0 && min > 0 && ratio > 0) {
				if(ratio < min) {
					dy = (dx * min + dy) / (min * min + 1);
					dx = dy * min;
					w = (int)dx + c->basew;
					h = (int)dy + c->baseh;
				}
				else if(ratio > max) {
					dy = (dx * min + dy) / (max * max + 1);
					dx = dy * min;
					w = (int)dx + c->basew;
					h = (int)dy + c->baseh;
				}
			}
		}
		if(c->minw && w < c->minw)
			w = c->minw;
		if(c->minh && h < c->minh)
			h = c->minh;
		if(c->maxw && w > c->maxw)
			w = c->maxw;
		if(c->maxh && h > c->maxh)
			h = c->maxh;
		if(c->incw)
			w -= (w - c->basew) % c->incw;
		if(c->inch)
			h -= (h - c->baseh) % c->inch;
	}
	if(w <= 0 || h <= 0)
		return;
	/* offscreen appearance fixes */
	if(x > sw)
		x = sw - w - 2 * c->border;
	if(y > sh)
		y = sh - h - 2 * c->border;
	if(x + w + 2 * c->border < sx)
		x = sx;
	if(y + h + 2 * c->border < sy)
		y = sy;
	if(c->x != x || c->y != y || c->w != w || c->h != h) {
		c->x = wc.x = x;
		c->y = wc.y = y;
		c->w = wc.width = w;
		c->h = wc.height = h;
		wc.border_width = c->border;
		XConfigureWindow(dpy, c->win, CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wc);
		configure(c);
		XSync(dpy, False);
	}
}
void
snapit(Client *c, int *Nx, int *Ny){
	int nx=*Nx, ny=*Ny;

	if(abs(wax + nx) < SNAP)
		nx = wax;
	else if(abs((wax + waw) - (nx + c->w + 2 * c->border)) < SNAP)
		nx = wax + waw - c->w - 2 * c->border;
	if(abs(way - ny) < SNAP)
		ny = way;
	else if(abs((way + wah) - (ny + c->h + 2 * c->border)) < SNAP)
		ny = way + wah - c->h - 2 * c->border;

	*Nx=nx; *Ny=ny;
}

void
setresizemousepos(int Cx, int Cy, int mx, int my, int *Nx, int *Ny, int *Nw, int *Nh) {
	int nx=*Nx,ny=*Ny,nw=*Nw,nh=*Nh;

	if(my > ny && my < ny+nh) { /* inside the horizontal strip */

		if(mx > nx && mx < nx+nw) { /* inside the window */
			if(mx < Cx) { /* left from center */
				nw=nx+nw-mx;
				nx=mx;
			} else if(mx > Cx) { /* right from center */
				nw=mx-nx;
			}
			if(my < Cy) { /* down from center */
				nh=ny+nh-my;
				ny=my;
			} else if(my > Cy) { /* right from center */
				nh=my-ny;
			}
		} else { /* outside the win */
			if(mx > nx+nw) {
				/* outside, right */
				nw=mx-nx;
			} else if(mx < nx) { /* outside, left */
				nw+=nx-mx;
				nx=mx;
			}
		}
	} else if(mx > nx && mx < nx+nw) { /* inside the vertical strip, outside the win */
		if(my > ny+nh) {
			/* outside, down */
			nh=my-ny;
		} else if(my < ny) { /* outside, left */
			nh+=ny-my;
			ny=my;
		}
	} else { /* in an angle */
		if(my > ny+nh) {
			nh=my-ny;
		} else if(my < ny) {
			nh+=ny-my;
			ny=my;
		}
		if(mx > nx+nw) {
			nw=mx-nx;
		} else if(mx < nx) {
			nw+=nx-mx;
			nx=mx;
		}
	}

	*Nx=nx;*Ny=ny;*Nw=nw;*Nh=nh;
}

void
resizemouse(Client *c) {
	int di, nw, nh, nx, ny, mx, my, Cx, Cy;
	unsigned int dui;
	Window dummy;
	XEvent ev;

	nx = c->x;
	ny = c->y;
	nw = c->w;
	nh = c->h;

	XQueryPointer(dpy, root, &dummy, &dummy, &mx, &my, &di, &di, &dui);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	XSync(dpy, False);

	snapit(c, &mx, &my);

	/* Find the center of the window */
	Cx=(nx+nx+nw)/2;
	Cy=(ny+ny+nh)/2;

	/** X **/
	setresizemousepos(Cx,Cy,  mx,my,  &nx, &ny,  &nw, &nh);
	resize(c, nx, ny, nw, nh, False);
}

void
restack(void) {
	XEvent ev;

	drawbar();
	if(!sel)
		return;
	XRaiseWindow(dpy, sel->win);

	XSync(dpy, False);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void) {
	char *p;
	int r, xfd;
	fd_set rd;
	XEvent ev;

	/* main event loop */
	XSync(dpy, False);
	xfd = ConnectionNumber(dpy);
	readin = True;
	while(running) {
		FD_ZERO(&rd);
		if(readin)
			FD_SET(STDIN_FILENO, &rd);
		FD_SET(xfd, &rd);
		if(select(xfd + 1, &rd, NULL, NULL, NULL) == -1) {
			if(errno == EINTR)
				continue;
			eprint("select failed\n");
		}
		if(FD_ISSET(STDIN_FILENO, &rd)) {
			switch(r = read(STDIN_FILENO, stext, sizeof stext - 1)) {
			case -1:
				strncpy(stext, strerror(errno), sizeof stext - 1);
				stext[sizeof stext - 1] = '\0';
				readin = False;
				break;
			case 0:
				strncpy(stext, "EOF", 4);
				readin = False;
				break;
			default:
				for(stext[r] = '\0', p = stext + strlen(stext) - 1; p >= stext && *p == '\n'; *p-- = '\0');
				for(; p >= stext && *p != '\n'; --p);
				if(p > stext)
					strncpy(stext, p + 1, sizeof stext);
			}
			drawbar();
		}
		while(XPending(dpy)) {
			XNextEvent(dpy, &ev);
			if(handler[ev.type])
				(handler[ev.type])(&ev); /* call handler */
		}
	}
}

void
scan(void) {
	unsigned int i, num;
	Window *wins, d1, d2;
	XWindowAttributes wa;

	wins = NULL;
	if(XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for(i = 0; i < num; i++) {
			if(!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if(wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for(i = 0; i < num; i++) { /* now the transients */
			if(!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if(XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
	}
	if(wins)
		XFree(wins);
	arrange();
}

void
setclientstate(Client *c, long state) {
	long data[] = {state, None};

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
			PropModeReplace, (unsigned char *)data, 2);
}

void
setup(void) {
	int d;
	unsigned int i, j, mask;
	Window w;
	XModifierKeymap *modmap;
	XSetWindowAttributes wa;

	/* init atoms */
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMName] = XInternAtom(dpy, "WM_NAME", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
			PropModeReplace, (unsigned char *) netatom, NetLast);

	/* init cursors */
	cursor[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(dpy, XC_fleur);

	/* init geometry */
	sx = sy = 0;
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);

	/* init modifier map */
	modmap = XGetModifierMapping(dpy);
	for(i = 0; i < 8; i++)
		for(j = 0; j < modmap->max_keypermod; j++) {
			if(modmap->modifiermap[i * modmap->max_keypermod + j]
			== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
		}
	XFreeModifiermap(modmap);

	/* select for events */
	wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
		| EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
	wa.cursor = cursor[CurNormal];
	XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);

	/* grab keys */
	keypress(NULL);

	/* init V */
	selvx=selvy=0;
	memset(lastselvy, 0, sizeof(*lastselvy)*MAX_NVX);
	memset(nvy, 0, sizeof(*nvy)*MAX_NVX);
	nvx=nvy[selvx]=0;

	/* init appearance */
	dc.norm[ColBorder] = getcolor(NORMBORDERCOLOR);
	dc.norm[ColBG] = getcolor(NORMBGCOLOR);
	dc.norm[ColFG] = getcolor(NORMFGCOLOR);
	dc.sel[ColBorder] = getcolor(SELBORDERCOLOR);
	dc.sel[ColBG] = getcolor(SELBGCOLOR);
	dc.sel[ColFG] = getcolor(SELFGCOLOR);
	initfont(FONT);
	dc.h = bh = dc.font.height + 2;

	/* init layouts */
	mwfact = MWFACT;

	/* init bar */
	bpos = BARPOS;
	wa.override_redirect = 1;
	wa.background_pixmap = ParentRelative;
	wa.event_mask = ButtonPressMask | ExposureMask;
	barwin = XCreateWindow(dpy, root, sx, sy, sw, bh, 0,
			DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
			CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
	XDefineCursor(dpy, barwin, cursor[CurNormal]);
	updatebarpos();
	XMapRaised(dpy, barwin);
	strcpy(stext, "AlpT-wm");
	dc.drawable = XCreatePixmap(dpy, root, sw, bh, DefaultDepth(dpy, screen));
	dc.gc = XCreateGC(dpy, root, 0, 0);
	XSetLineAttributes(dpy, dc.gc, 1, LineSolid, CapButt, JoinMiter);
	if(!dc.font.set)
		XSetFont(dpy, dc.gc, dc.font.xfont->fid);

	/* multihead support */
	selscreen = XQueryPointer(dpy, root, &w, &w, &d, &d, &d, &d, &mask);

	/* first execs */
	for(i = 0; initexecs[i]; i++)
		spawn(initexecs[i]);
}

void
spawn(const char *arg) {
	static char *shell = NULL;

	if(!shell && !(shell = getenv("SHELL")))
		shell = "/bin/sh";
	if(!arg)
		return;
	/* The double-fork construct avoids zombie processes and keeps the code
	 * clean from stupid signal handlers. */
	if(fork() == 0) {
		if(fork() == 0) {
			if(dpy)
				close(ConnectionNumber(dpy));
			setsid();
			execl(shell, shell, "-c", arg, (char *)NULL);
			fprintf(stderr, "dwm: execl '%s -c %s'", shell, arg);
			perror(" failed");
		}
		exit(0);
	}
	wait(0);
}

unsigned int
textnw(const char *text, unsigned int len) {
	XRectangle r;

	if(dc.font.set) {
		XmbTextExtents(dc.font.set, text, len, NULL, &r);
		return r.width;
	}
	return XTextWidth(dc.font.xfont, text, len);
}

unsigned int
textw(const char *text) {
	return textnw(text, strlen(text)) + dc.font.height;
}

void
togglebar(const char *arg) {
	if(bpos == BarOff)
		bpos = (BARPOS == BarOff) ? BarTop : BARPOS;
	else
		bpos = BarOff;
	updatebarpos();
	arrange();
}

void
togglefloating(const char *arg) {
	if(!sel)
		return;
	sel->isfloating = !sel->isfloating;
	if(sel->isfloating)
		resize(sel, sel->x, sel->y, sel->w, sel->h, True);
//	arrange();
}

void
togglemax(const char *arg) {
	XEvent ev;

	if(!sel || sel->isfixed)
		return;
	if((sel->ismax = !sel->ismax)) {
		sel->wasfloating = True;
		sel->rx = sel->x;
		sel->ry = sel->y;
		sel->rw = sel->w;
		sel->rh = sel->h;
		//resize(sel, wax, way, waw - 2 * sel->border, wah - 2 * sel->border, True);
		resize(sel, sx, sy, sw, sh, True);
	}
	else {
		resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, True);
		if(!sel->wasfloating)
			togglefloating(NULL);
	}
	drawbar();
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void revertpretilegeom(const char *arg) {
	Client *c;
	for(c=clients; c; c=c->next) 
		if(isvisible(c))
			resize(c, c->px, c->py, c->pw, c->ph, False);
}

void
unban(Client *c) {
	if(!c->isbanned)
		return;
	XMoveWindow(dpy, c->win, c->x, c->y);
	c->isbanned = False;
}

void
unmanage(Client *c) {
	XWindowChanges wc;
	
	if(pmpterm==c)
		pmpterm=0;

	wc.border_width = c->oldborder;
	/* The server grab construct avoids race conditions. */
	XGrabServer(dpy);
	XSetErrorHandler(xerrordummy);
	XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
	detach(c);
	detachstack(c);
	if(sel == c)
		focus(NULL);
	XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
	setclientstate(c, WithdrawnState);
	free(c);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XUngrabServer(dpy);
	arrange();
}

void
unmapnotify(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if((c = getclient(ev->window)))
		unmanage(c);
}

void
updatebarpos(void) {
	XEvent ev;

	wax = sx;
	way = sy;
	wah = sh;
	waw = sw;
	switch(bpos) {
	default:
		wah -= bh;
		way += bh;
		XMoveWindow(dpy, barwin, sx, sy);
		break;
	case BarBot:
		wah -= bh;
		XMoveWindow(dpy, barwin, sx, sy + wah);
		break;
	case BarOff:
		XMoveWindow(dpy, barwin, sx, sy - bh);
		break;
	}
	XSync(dpy, False);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
updatesizehints(Client *c) {
	long msize;
	XSizeHints size;

	if(!XGetWMNormalHints(dpy, c->win, &size, &msize) || !size.flags)
		size.flags = PSize;
	c->flags = size.flags;
	if(c->flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	}
	else if(c->flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	}
	else
		c->basew = c->baseh = 0;
	if(c->flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	}
	else
		c->incw = c->inch = 0;
	if(c->flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	}
	else
		c->maxw = c->maxh = 0;
	if(c->flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	}
	else if(c->flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	}
	else
		c->minw = c->minh = 0;
	if(c->flags & PAspect) {
		c->minax = size.min_aspect.x;
		c->maxax = size.max_aspect.x;
		c->minay = size.min_aspect.y;
		c->maxay = size.max_aspect.y;
	}
	else
		c->minax = c->maxax = c->minay = c->maxay = 0;
	c->isfixed = (c->maxw && c->minw && c->maxh && c->minh
			&& c->maxw == c->minw && c->maxh == c->minh);
}

void
updatetitle(Client *c) {
	if(!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, wmatom[WMName], c->name, sizeof c->name);
	dumpwindowtitles();
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.  */
int
xerror(Display *dpy, XErrorEvent *ee) {
	if(ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable)
	|| (ee->request_code == X_KillClient && ee->error_code == BadValue))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dsply, XErrorEvent *ee) {
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dsply, XErrorEvent *ee) {
	otherwm = True;
	return -1;
}

void _viewX(int x) {
	x=x<0?0:x;

	if(x > nvx && nvx+1 >= MAX_NVX)
		/* no more space */
		return;

	if(x > nvx) {
		/* new space */
		x=++nvx;
		nvy[nvx]=0;
	}

	selvx=x;
	selvy=lastselvy[selvx];

	arrange();
}
void viewX(const char *arg) { _viewX(atoi(arg)); }

void _viewY(int y) {
	selvy=y<0 ? 0 : y;
	if(selvy > nvy[selvx]) {
		/* new space */
		selvy=++nvy[selvx];
	}
	lastselvy[selvx]=selvy;
	arrange();
}
void viewY(const char *arg) { _viewY(atoi(arg)); }

void insertviewX(const char *arg) {
	Client *c;

	/* shift all the next windows */
	for(c=clients; c; c=c->next)
		if(c->vx > selvx)
			c->vx++;
	
	_viewX(++selvx);
}

void insertviewY(const char *arg) {
	Client *c;

	/* shift all the next windows */
	for(c=clients; c; c=c->next)
		if(c->vy > selvy)
			c->vy++;
	
	_viewY(++selvy);
}

void nextviewX(const char *arg) { _viewX(selvx+1); }
void nextviewY(const char *arg) { _viewY(selvy+1); }
void prevviewX(const char *arg) { _viewX(selvx-1); }
void prevviewY(const char *arg) { _viewY(selvy-1); }

void delviewX(const char *arg) {
	Client *c;

	if(nvx < 0)
		return;

	for(c = clients; c; c = c->next) {
		if(c->vx == selvx) {
			sel=c;
			killclient(NULL);
		} else if(c->vx > selvx)
			c->vx--;
	}

	lastselvy[nvx]=nvy[nvx]=0;
	nvx--;
	if(selvx == nvx+1)
		prevviewX(NULL);
	arrange();
}

void delviewY(const char *arg) {
	Client *c;
	
	if(nvy[selvx] < 0)
		return;

	for(c = clients; c; c = c->next) {
		if(c->vx != selvx)
			continue;
		if(c->vy == selvy) {
			sel=c;
			killclient(NULL);
		} else if(c->vy > selvy)
			c->vy--;
	}
	
	nvy[selvx]--;
	if(selvy == nvy[selvx]+1) {
		if(nvy[selvx] < 0)
			/* No more Y on the current X. Delete it */
			delviewX(0);
		else
			prevviewY(NULL);
	}

	arrange();
}

void setwindowviewX(const char *arg) {
	Client *oldsel;
	if(!sel)
		return;

	dont_spawn=True;
	oldsel=sel;
	viewX(arg);
	sel=oldsel;
	if(sel && sel->vx != selvx) {
		sel->vx=selvx;
		sel->vy=selvy;
		arrange();
		focus(oldsel);
		restack();
	}
	dont_spawn=False;
}

void setwindowviewY(const char *arg) {
	Client *oldsel;
	if(!sel)
		return;

	dont_spawn=True;
	oldsel=sel;
	viewY(arg);
	sel=oldsel;
	if(sel && sel->vy != selvy) {
		sel->vx=selvx;
		sel->vy=selvy;
		arrange();
		focus(oldsel);
		restack();
	}
	dont_spawn=False;
}

void setwindownextviewX(const char *arg) {
	char buf[32];
	sprintf(buf, "%d", selvx+1);
	setwindowviewX((const char *)buf);
}
void setwindowprevviewX(const char *arg) {
	char buf[32];
	sprintf(buf, "%d", selvx-1);
	setwindowviewX((const char *)buf);
}
void setwindownextviewY(const char *arg) {
	char buf[32];
	sprintf(buf, "%d", selvy+1);
	setwindowviewY((const char *)buf);
}
void setwindowprevviewY(const char *arg) {
	char buf[32];
	sprintf(buf, "%d", selvy-1);
	setwindowviewY((const char *)buf);
}

void
dumpwindowtitles(void) {
	FILE *fd;
	Client *c;

	if(!(fd=fopen(PROC_DWM, "w")))
		eprint("Cannot open %s\n", PROC_DWM);

	for(c = clients; c; c = c->next)
		if(c->vx >= 0 && c->vy >= 0)
			fprintf(fd, "%d,%d\t%s\n", c->vx+1,c->vy+1, c->name);

	fclose(fd);
}

int
main(int argc, char *argv[]) {
	if(argc == 2 && !strcmp("-v", argv[1]))
		eprint("awm-"VERSION" based on\n\tdwm, © 2006-2007 A. R. Garbe, S. van Dijk, J. Salmi, P. Hruby, S. Nagy\n");
	else if(argc != 1)
		eprint("usage: dwm [-v]\n");

	setlocale(LC_CTYPE, "");
	if(!(dpy = XOpenDisplay(0)))
		eprint("dwm: cannot open display\n");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	checkotherwm();
	setup();
	drawbar();
	scan();
	run();
	cleanup();

	XCloseDisplay(dpy);
	return 0;
}
