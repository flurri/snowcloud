#include "nimbus_xcb.h"

int main(int argc, char **argv) {
  xcb_connection_t *conn;
  const xcb_setup_t *setup;
  xcb_depth_iterator_t depth_iter;
  xcb_visualtype_iterator_t visual_iter;
  xcb_screen_t *scr;
  xcb_intern_atom_cookie_t *EWMHCookie;
  xcb_screen_iterator_t iter;
  xcb_generic_error_t *xcberr;
  xcb_window_t selector;
  xcb_ewmh_connection_t EWMH;
  xcb_generic_event_t *event;
  xcb_visualtype_t *visual_type = NULL;
  xcb_gcontext_t gfxregion;
  cairo_surface_t *cs;
  uint32_t gfxval[1];
  int err;

  conn = xcb_connect(NULL, NULL);
  
  if ((err = xcb_connection_has_error(conn))) {
    fprintf(stderr, "[FATAL] Could not connect X11 display (err.%d)", err);
    return 1;
  }
  
  setup = xcb_get_setup(conn);
  
  iter = xcb_setup_roots_iterator(setup);

  scr = iter.data;

  EWMHCookie = xcb_ewmh_init_atoms(conn, &EWMH);

  if (!xcb_ewmh_init_atoms_replies(&EWMH, EWMHCookie, NULL)) {
    fprintf(stderr, "Could not get hints for fullscreenify.\n");
    return 2;
  }

  if (scr) {
    depth_iter = xcb_screen_allowed_depths_iterator(scr);
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
      visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
      for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
	
	if (scr->root_visual == visual_iter.data->visual_id) {
	  visual_type = visual_iter.data;
	  break;
	}
      }
    }
  }

  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, selector, EWMH._NET_WM_STATE, XCB_ATOM,
		      32, 1, &(EWMH._NET_WM_STATE_FULLSCREEN));

  fprintf(stderr, "\n[INFO] Your resolution: %dx%d\n", scr->width_in_pixels, scr->height_in_pixels);
  
  selector = xcb_generate_id(conn);

  gfxregion = xcb_generate_id(conn);
  
  xcb_create_window(conn,
		    24,
		    selector,
		    scr->root,
		    0,
		    0,
		    scr->width_in_pixels,
		    scr->height_in_pixels,
		    0,
		    XCB_WINDOW_CLASS_INPUT_OUTPUT,
		    scr->root_visual,
		    0, NULL);
  
  xcb_map_window(conn, selector);

  gfxval[0] = scr->black_pixel;
  
  xcb_create_gc(conn,
		gfxregion,
		scr->root,
		XCB_GC_FOREGROUND,
		gfxval);

  xcb_flush(conn);

  cs = cairo_xcb_surface_create(conn, gfxregion, visual_type, scr->width_in_pixels, scr->height_in_pixels);
  while ((event = xcb_wait_for_event(conn))) {
    switch (event->response_type & ~0x80) {
    case XCB_EXPOSE:
      paint(cs, scr->width_in_pixels, scr->height_in_pixels);
      break;
    default:
      break;
    }
    free(event);
  }

  xcb_disconnect(conn);

  return 0;
}
