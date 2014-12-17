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
#include <math.h>

cairo_status_t _write_png(void *closure, const unsigned char *data, unsigned int length);

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

  cairo_set_source_rgba(cs, 1.0, 1.0, 1.0, 0.7);
  
  cairo_select_font_face(cs, "sans-serif",
			 CAIRO_FONT_SLANT_NORMAL,
			 CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(cs, 11);

  cairo_move_to(cs, width-250, height-5);
  cairo_show_text(cs, "Select the region or press a key to cancel.");
  
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

  cairo_rectangle(cs, initx-1, inity-1, width+2, height+2);
  cairo_set_source_rgba(cs, 0.0, 0.0, 0.0, 0.7);
  cairo_set_operator(cs, CAIRO_OPERATOR_SOURCE);
  cairo_fill(cs);
  
  cairo_rectangle(cs, initx, inity, width, height);
  cairo_set_source_rgba(cs, 0.0, 0.0, 0.0, 0.0);
  cairo_set_operator(cs, CAIRO_OPERATOR_SOURCE);
  cairo_fill(cs);

  cairo_destroy(cs);

  return;
}

pngstream mkpngstream(Display *disp, int scr, Visual *vis, XImage *img, ScreenshotLoc initial, ScreenshotLoc current) {
  int width, height, stride;
  cairo_surface_t *cs;
  unsigned char *tmpblob = NULL, r, g, b;
  pngstream pngblob;
  unsigned long pixel;
  int x, y;
  cairo_status_t res;
  pngstream bad;
  bad.length = 0;
  bad.data = NULL;
  
  // error checking
  if (!img) {
    printf("[FATAL] XImage is NULL!\n");
    return bad;
  }
  
  if ((width = current.x-initial.x) < 0) {
    printf("[FATAL] width is negative!\n");
    return bad;
  }

  if ((height = current.y-initial.y) < 0) {
    printf("[FATAL] height is negative!\n");
    return bad;
  }

  stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width);

  tmpblob = (unsigned char *)malloc(stride * height);

  for (y=0;y<height;y++) {
    for (x=0;x<width;x++) {
      pixel = XGetPixel(img, x, y);
      r = (img->red_mask & pixel) >> 16;
      g = (img->green_mask & pixel) >> 8;
      b = (img->blue_mask & pixel);

      tmpblob[y * stride + x * 4] = b;
      tmpblob[y * stride + x * 4 + 1] = g;
      tmpblob[y * stride + x * 4 + 2] = r;
    }
  }

  cs = cairo_image_surface_create_for_data(tmpblob,
					   CAIRO_FORMAT_RGB24,
					   width,
					   height,
					   stride);

  pngblob.length = 0;
  pngblob.data = (unsigned char *)malloc(sizeof(unsigned char) * 5120000);
  pngblob.maxlen = 5120000;
  pngblob.cpos = pngblob.data;
  
  if ((res = cairo_surface_write_to_png_stream(cs, _write_png, &pngblob)) != CAIRO_STATUS_SUCCESS) {
    printf("cairo_surface_write_to_png_stream failed: %d", res);
    cairo_surface_destroy(cs);
    free(tmpblob);
    return bad;
  }

  printf("Still going?");

  cairo_surface_destroy(cs);
  free(tmpblob);

  return pngblob;
}

void test_write_file(pngstream png, char *fname) {
  FILE *pngf;

  printf("test_write_file\n");
  
  if (!fname) {
    printf("[FATAL] Filename is NULL!\n");
    return;
  }

  if (!(pngf = fopen(fname, "w"))) {
    printf("[FATAL] Could not open file %s!\n", fname);
    return;
  }
  fwrite(png.data, png.length, 1, pngf);
  fclose(pngf);
}

cairo_status_t _write_png(void *closure, const unsigned char *data, unsigned int length) {
  unsigned int i;
  pngstream *png = (pngstream *)closure;

  if (!png->data) {
    return CAIRO_STATUS_WRITE_ERROR;
  }


  if ((png->length + length) > png->maxlen) {
    return CAIRO_STATUS_WRITE_ERROR;
  }

  memcpy(png->cpos, data, length);
  
  png->cpos += length;
  png->length += length;
  return CAIRO_STATUS_SUCCESS;
}
