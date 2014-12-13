#include "snowcloud.h"
#include <math.h>

void RemoveDecorations(Display *disp, Window win) {
  Atom prop;
  MotifWmHints hints;

  memset(&hints, 0, sizeof(hints));
  
  hints.flags = 2; // MWM_HINTS_DECORATIONS
  hints.decorations = 0;
  
  prop = XInternAtom(disp, "_MOTIF_WM_HINTS", True);
  XChangeProperty(disp, win, prop, prop, 32, PropModeReplace, (unsigned char *)&hints, sizeof(MotifWmHints)/sizeof(long));
}

int get_xevents(Display *disp, XEvent *e) {
  if (!e) return; // nothing we can do

  XNextEvent(disp, e);

  return 1;
}

void paint(cairo_surface_t *surface, int width, int height) {
  cairo_t *cs;

  cs = cairo_create(surface);

  cairo_rectangle(cs, 0.0, 0.0, width, height);
  cairo_set_operator(cs, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cs);
  cairo_set_source_rgba(cs, 0.0, 0.0, 0.0, 0.3);
  cairo_set_operator(cs, CAIRO_OPERATOR_OVER);
  cairo_paint(cs);

  cairo_destroy(cs);

  return;
}

void setrect(cairo_surface_t *surface, ScreenshotLoc initial, ScreenshotLoc current) {
  cairo_t *cs;

  int width, height;

  int initx, inity;

  width = (current.x - initial.x);
  initx = initial.x;

  if (width < 0) {
    width = -1*width;
    initx = current.x;
  }

  height = (current.y - initial.y);
  inity = initial.y;

  if (height < 0) {
    height = -1*height;
    inity = current.y;
  }
  
  cs = cairo_create(surface);

  printf(
	 "InitX: %d :: InitY:  %d\n"
	 "CurrX: %d :: CurrY:  %d\n"
	 "Width: %d :: Height: %d\n",
	 initx, inity, current.x, current.y, width, height);
  
  cairo_rectangle(cs, initx, inity, width, height);
  cairo_set_source_rgba(cs, 0.0, 0.0, 0.0, 0.0);
  cairo_set_operator(cs, CAIRO_OPERATOR_SOURCE);
  cairo_fill(cs);

  cairo_destroy(cs);

  return;
}
