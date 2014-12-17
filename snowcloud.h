/*
 * snowcloud - A CloudApp interface for X11
 *
 * Copyright (c) 2014 Sarah[flurry]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

typedef struct {
  size_t length;
  size_t maxlen;
  unsigned char *data;
  unsigned char *cpos;
} pngstream;

void setrect(cairo_surface_t *surface, ScreenshotLoc initial, ScreenshotLoc current);
pngstream mkpngstream(Display *disp, int scr, Visual *vis, XImage *img, ScreenshotLoc initial, ScreenshotLoc current);
void test_write_file(pngstream png, char *fname);
