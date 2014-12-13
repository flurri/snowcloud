/* nimbus.h - Define most functions and include the typical stuff
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <png.h>
// cairo
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

typedef struct _motifWmHints {
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
  long input_mode;
  unsigned long status;
} MotifWmHints;

//void send_systray_event(Display *disp, Window *win, long messagedata[4]);

void RemoveDecorations(Display*, Window);

int get_xevents(Display*,XEvent*);

void paint(cairo_surface_t*, int, int);

//XImage *getPNG(char *loc, size_t locsize);
typedef struct _screenshotLoc {
  int x;
  int y;
} ScreenshotLoc;

void setrect(cairo_surface_t *surface, ScreenshotLoc initial, ScreenshotLoc current);
