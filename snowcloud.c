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
#include "snowcloud.h"

int main(int argc, char *argv[]) {
  Display *disp;
  int scr, prepare, tmp;
  Window selector, rootwin;
  XEvent event;
  cairo_surface_t *canvas;
  Atom setfull[2];
  XWindowAttributes scrattr;
  XSetWindowAttributes winattr;
  XVisualInfo vis;
  ScreenshotLoc initloc, endloc;
  pngstream pngstr;
  XImage *ximg;
  
  // Get the user's display
  if (!(disp = XOpenDisplay(NULL))) {
    printf("Could not get display!\n");
  }

  // Get screen, root
  scr = DefaultScreen(disp);
  rootwin = RootWindow(disp, scr);
		   
  XMatchVisualInfo(disp, scr, 32, TrueColor, &vis);
  
  // Get screen stats for width/height
  XGetWindowAttributes(disp, rootwin, &scrattr);

  // Set window attributes
  winattr.colormap = XCreateColormap(disp, rootwin, vis.visual, AllocNone);
  winattr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask;
  winattr.background_pixmap = None;
  winattr.border_pixel = 0;
  // Make the selector window
  selector = XCreateWindow(disp,
			   rootwin,
			   0, 0,
			   scrattr.width, scrattr.height,
			   0,
			   vis.depth,
			   InputOutput,
			   vis.visual,
			   CWColormap | CWEventMask | CWBackPixmap | CWBorderPixel,
			   &winattr); 
  XStoreName(disp, selector, "cloudselect");
  // We need key release (any key == cancel), and button for hold/release
  //XSelectInput(disp, selector, ExposureMask|ButtonPressMask|ButtonReleaseMask|KeyReleaseMask);
  XMapWindow(disp, selector);

  // set our window to fullscreen
  setfull[0] = XInternAtom(disp, "_NET_WM_STATE_FULLSCREEN", False);
  setfull[1] = None;
  
  XChangeProperty(disp,
		  selector,
		  XInternAtom(disp, "_NET_WM_STATE", False),
		  XA_ATOM,
		  32,
		  PropModeReplace,
		  (unsigned char *)setfull,
		  1);

  RemoveDecorations(disp, selector);  
  
  // make a cairo surface to paint on
  canvas = cairo_xlib_surface_create(disp, selector, vis.visual, scrattr.width, scrattr.height);

  initloc.x = initloc.y = -1;

  prepare = 0;
  
  while (get_xevents(disp, &event)) {
    if (event.type == Expose) {
      paint(canvas, scrattr.width, scrattr.height);
    } else if (event.type == ButtonPress) {
      if (event.xbutton.button == Button1) {
	initloc.x = event.xbutton.x;
	initloc.y = event.xbutton.y;
      } else {
	printf("[INFO] Operation cancelled.\n");
	break;
      }
    } else if (event.type == ButtonRelease) { // We have completed selecting!
      if (event.xbutton.button == Button1) {
	endloc.x = event.xbutton.x;
	endloc.y = event.xbutton.y;

	// Negative resolutions are bad and make no sense anyway
	if (endloc.x < initloc.x) {
	  tmp = endloc.x;
	  endloc.x = initloc.x;
	  initloc.x = tmp;
	}
	if (endloc.y < initloc.y) {
	  tmp = endloc.y;
	  endloc.y = initloc.y;
	  initloc.y = tmp;
	}

	// Debug printf()s
	printf("Initial co-ordinates: (%d, %d)\n", initloc.x, initloc.y);
	printf("Final co-ordinates:   (%d, %d)\n", endloc.x, endloc.y);
	printf("Dimensions:           %dx%d\n", endloc.y-initloc.y, endloc.x-initloc.x);

	// Get selection as XImage
	ximg = XGetImage(disp, rootwin, initloc.x, initloc.y, (endloc.x-initloc.x), (endloc.y-initloc.y), XAllPlanes(), ZPixmap);

	// Pass XImage to mkpngstream
	pngstr = mkpngstream(disp, DefaultScreen(disp), vis.visual, ximg, initloc, endloc); // Make PNG stream to send to CloudApp
	if (!pngstr.data) {
	  printf("Making PNG stream failed...\n");
	  if (ximg) {
	    XFree(ximg); // XFree doesn't like NULL pointers
	  }
	} else {
	  test_write_file(pngstr, "test.png");
	}
	break;
      } else { // ... Wrong button!
	printf("[INFO Operation cancelled.\n");
	break;
      }
    } else if (event.type == MotionNotify) { // We only track Button1Motion, so...
      endloc.x = event.xmotion.x;
      endloc.y = event.xmotion.y;
      printf("[INFO] endloc.x: %d\nendloc.y: %d\n", endloc.x, endloc.y);
      printf("[INFO] initloc.x: %d\ninitloc.y: %d\n", initloc.x, initloc.y);
      paint(canvas, scrattr.width, scrattr.height);
      setrect(canvas, initloc, endloc);
    } else if (event.type == KeyRelease) {
      if (prepare == 1) break;
    }
    else if (event.type == KeyPress) {
      prepare = 1;
    }
  }

  cairo_surface_destroy(canvas);

  XCloseDisplay(disp);
  return 0;
}
