/* lock.c --- handling the password dialog for locking-mode.
 * xscreensaver, Copyright (c) 1993-1998 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/* Athena locking code contributed by Jon A. Christopher <jac8782@tamu.edu> */
/* Copyright 1997, with the same permissions as above. */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef NO_LOCKING   /* whole file */

#include <X11/Intrinsic.h>
#include "xscreensaver.h"
#include "resources.h"

#ifdef HAVE_SYSLOG
# include <syslog.h>
#endif /* HAVE_SYSLOG */

#ifdef _VROOT_H_
ERROR!  You must not include vroot.h in this file.
#endif

#ifndef VMS
# include <pwd.h>
#else /* VMS */

extern char *getenv(const char *name);
extern int validate_user(char *name, char *password);

static Bool
vms_passwd_valid_p(char *pw, Bool verbose_p)
{
  return (validate_user (getenv("USER"), typed_passwd) == 1);
}
# undef passwd_valid_p
# define passwd_valid_p vms_passwd_valid_p

#endif /* VMS */


#undef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))

enum passwd_state { pw_read, pw_ok, pw_null, pw_fail, pw_cancel, pw_time };

struct passwd_dialog_data {

  enum passwd_state state;
  char typed_passwd [80];
  XtIntervalId timer;
  int i_beam;

  float ratio;
  Position x, y;
  Dimension width;
  Dimension height;
  Dimension border_width;

  char *heading_label;
  char *body_label;
  char *user_label;
  char *passwd_label;
  char *user_string;
  char *passwd_string;

  XFontStruct *heading_font;
  XFontStruct *body_font;
  XFontStruct *label_font;
  XFontStruct *passwd_font;

  Pixel foreground;
  Pixel background;
  Pixel passwd_foreground;
  Pixel passwd_background;
  Pixel logo_foreground;
  Pixel logo_background;
  Pixel shadow_top;
  Pixel shadow_bottom;

  Dimension logo_width;
  Dimension logo_height;
  Dimension thermo_width;
  Dimension internal_border;
  Dimension shadow_width;

  Dimension passwd_field_x, passwd_field_y;
  Dimension passwd_field_width, passwd_field_height;

  Dimension thermo_field_x, thermo_field_y;
  Dimension thermo_field_height;

  Pixmap save_under;
};


void
make_passwd_window (saver_info *si)
{
  struct passwd *p = getpwuid (getuid ());
  XSetWindowAttributes attrs;
  unsigned long attrmask = 0;
  Screen *screen = si->default_screen->screen;
  passwd_dialog_data *pw = (passwd_dialog_data *) calloc (1, sizeof(*pw));
  Colormap cmap = DefaultColormapOfScreen (screen);
  char *f;

  pw->ratio = 1.0;

  pw->heading_label = get_string_resource ("passwd.heading.label",
					   "Dialog.Label.Label");
  pw->body_label = get_string_resource ("passwd.body.label",
					"Dialog.Label.Label");
  pw->user_label = get_string_resource ("passwd.user.label",
					"Dialog.Label.Label");
  pw->passwd_label = get_string_resource ("passwd.passwd.label",
					  "Dialog.Label.Label");

  if (!pw->heading_label)
    pw->heading_label = strdup("ERROR: REESOURCES NOT INSTALLED CORRECTLY");
  if (!pw->body_label)
    pw->body_label = strdup("ERROR: REESOURCES NOT INSTALLED CORRECTLY");
  if (!pw->user_label) pw->user_label = strdup("ERROR");
  if (!pw->passwd_label) pw->passwd_label = strdup("ERROR");

  /* Put the version number in the label. */
  {
    char *s = (char *) malloc (strlen(pw->heading_label) + 20);
    sprintf(s, pw->heading_label, si->version);
    free (pw->heading_label);
    pw->heading_label = s;
  }

  pw->user_string = (p->pw_name ? p->pw_name : "???");
  pw->passwd_string = strdup("");

  f = get_string_resource ("passwd.headingFont", "Dialog.Font");
  pw->heading_font = XLoadQueryFont (si->dpy, (f ? f : "fixed"));
  if (!pw->heading_font) pw->heading_font = XLoadQueryFont (si->dpy, "fixed");
  if (f) free (f);

  f = get_string_resource("passwd.bodyFont", "Dialog.Font");
  pw->body_font = XLoadQueryFont (si->dpy, (f ? f : "fixed"));
  if (!pw->body_font) pw->body_font = XLoadQueryFont (si->dpy, "fixed");
  if (f) free (f);

  f = get_string_resource("passwd.labelFont", "Dialog.Font");
  pw->label_font = XLoadQueryFont (si->dpy, (f ? f : "fixed"));
  if (!pw->label_font) pw->label_font = XLoadQueryFont (si->dpy, "fixed");
  if (f) free (f);

  f = get_string_resource("passwd.passwdFont", "Dialog.Font");
  pw->passwd_font = XLoadQueryFont (si->dpy, (f ? f : "fixed"));
  if (!pw->passwd_font) pw->passwd_font = XLoadQueryFont (si->dpy, "fixed");
  if (f) free (f);

  pw->foreground = get_pixel_resource ("passwd.foreground",
				       "Dialog.Foreground",
				       si->dpy, cmap);
  pw->background = get_pixel_resource ("passwd.background",
				       "Dialog.Background",
				       si->dpy, cmap);

  if (pw->foreground == pw->background)
    {
      /* Make sure the error messages show up. */
      pw->foreground = BlackPixelOfScreen (screen);
      pw->background = WhitePixelOfScreen (screen);
    }

  pw->passwd_foreground = get_pixel_resource ("passwd.text.foreground",
					      "Dialog.Text.Foreground",
					      si->dpy, cmap);
  pw->passwd_background = get_pixel_resource ("passwd.text.background",
					      "Dialog.Text.Background",
					      si->dpy, cmap);
  pw->logo_foreground = get_pixel_resource ("passwd.logo.foreground",
					    "Dialog.Logo.Foreground",
					    si->dpy, cmap);
  pw->logo_background = get_pixel_resource ("passwd.logo.background",
					    "Dialog.Logo.Background",
					    si->dpy, cmap);
  pw->shadow_top = get_pixel_resource ("passwd.topShadowColor",
				       "Dialog.Foreground",
				       si->dpy, cmap);
  pw->shadow_bottom = get_pixel_resource ("passwd.bottomShadowColor",
					  "Dialog.Background",
					  si->dpy, cmap);

  pw->logo_width = get_integer_resource ("passwd.logo.width",
					 "Dialog.Logo.Width");
  pw->logo_height = get_integer_resource ("passwd.logo.height",
					  "Dialog.Logo.Height");
  pw->thermo_width = get_integer_resource ("passwd.thermometer.width",
					   "Dialog.Thermometer.Width");
  pw->internal_border = get_integer_resource ("passwd.internalBorderWidth",
					      "Dialog.InternalBorderWidth");
  pw->shadow_width = get_integer_resource ("passwd.shadowThickness",
					   "Dialog.ShadowThickness");

  if (pw->logo_width == 0)  pw->logo_width = 150;
  if (pw->logo_height == 0) pw->logo_height = 150;
  if (pw->internal_border == 0) pw->internal_border = 15;
  if (pw->shadow_width == 0) pw->shadow_width = 4;
  if (pw->thermo_width == 0) pw->thermo_width = pw->shadow_width;

  {
    int direction, ascent, descent;
    XCharStruct overall;

    pw->width = 0;
    pw->height = 0;

    /* Measure the heading_label. */
    XTextExtents (pw->heading_font,
		  pw->heading_label, strlen(pw->heading_label),
		  &direction, &ascent, &descent, &overall);
    if (overall.width > pw->width) pw->width = overall.width;
    pw->height += ascent + descent;

    /* Measure the body_label. */
    XTextExtents (pw->body_font,
		  pw->body_label, strlen(pw->body_label),
		  &direction, &ascent, &descent, &overall);
    if (overall.width > pw->width) pw->width = overall.width;
    pw->height += ascent + descent;

    {
      Dimension w2 = 0, w3 = 0;
      Dimension h2 = 0, h3 = 0;
      const char *passwd_string = "MMMMMMMMMMMM";

      /* Measure the user_label. */
      XTextExtents (pw->label_font,
		    pw->user_label, strlen(pw->user_label),
		    &direction, &ascent, &descent, &overall);
      if (overall.width > w2)  w2 = overall.width;
      h2 += ascent + descent;

      /* Measure the passwd_label. */
      XTextExtents (pw->label_font,
		    pw->passwd_label, strlen(pw->passwd_label),
		    &direction, &ascent, &descent, &overall);
      if (overall.width > w2)  w2 = overall.width;
      h2 += ascent + descent;

      /* Measure the user_string. */
      XTextExtents (pw->passwd_font,
		    pw->user_string, strlen(pw->user_string),
		    &direction, &ascent, &descent, &overall);
      overall.width += (pw->shadow_width * 4);
      ascent += (pw->shadow_width * 4);
      if (overall.width > w3)  w3 = overall.width;
      h3 += ascent + descent;

      /* Measure the (maximally-sized, dummy) passwd_string. */
      XTextExtents (pw->passwd_font,
		    passwd_string, strlen(passwd_string),
		    &direction, &ascent, &descent, &overall);
      overall.width += (pw->shadow_width * 4);
      ascent += (pw->shadow_width * 4);
      if (overall.width > w3)  w3 = overall.width;
      h3 += ascent + descent;

      w2 = w2 + w3 + (pw->shadow_width * 2);
      h2 = MAX (h2, h3);

      if (w2 > pw->width)  pw->width  = w2;
      pw->height += h2;
    }

    pw->width  += (pw->internal_border * 2);
    pw->height += (pw->internal_border * 4);

    pw->width += pw->thermo_width + (pw->shadow_width * 3);

    if (pw->logo_height > pw->height)
      pw->height = pw->logo_height;
    else if (pw->height > pw->logo_height)
      pw->logo_height = pw->height;

    pw->logo_width = pw->logo_height;

    pw->width += pw->logo_width;
  }

  attrmask |= CWOverrideRedirect; attrs.override_redirect = True;
  attrmask |= CWEventMask; attrs.event_mask = ExposureMask|KeyPressMask;

  {
    Dimension w = WidthOfScreen(screen);
    Dimension h = HeightOfScreen(screen);
    if (si->prefs.debug_p) w /= 2;
    pw->x = ((w + pw->width) / 2) - pw->width;
    pw->y = ((h + pw->height) / 2) - pw->height;
    if (pw->x < 0) pw->x = 0;
    if (pw->y < 0) pw->y = 0;
  }

  pw->border_width = get_integer_resource ("passwd.borderWidth",
                                           "Dialog.BorderWidth");

  si->passwd_dialog =
    XCreateWindow (si->dpy,
		   RootWindowOfScreen(screen),
		   pw->x, pw->y, pw->width, pw->height, pw->border_width,
		   DefaultDepthOfScreen (screen), InputOutput,
		   DefaultVisualOfScreen(screen),
		   attrmask, &attrs);
  XSetWindowBackground (si->dpy, si->passwd_dialog, pw->background);


  /* Before mapping the window, save the bits that are underneath the
     rectangle the window will occlude.  When we lower the window, we
     restore these bits.  This works, because the running screenhack
     has already been sent SIGSTOP, so we know nothing else is drawing
     right now! */
  {
    XGCValues gcv;
    GC gc;
    pw->save_under = XCreatePixmap (si->dpy,
                                    si->default_screen->screensaver_window,
                                    pw->width + (pw->border_width*2) + 1,
                                    pw->height + (pw->border_width*2) + 1,
                                    si->default_screen->current_depth);
    gcv.function = GXcopy;
    gc = XCreateGC (si->dpy, pw->save_under, GCFunction, &gcv);
    XCopyArea (si->dpy, si->default_screen->screensaver_window,
               pw->save_under, gc,
               pw->x - pw->border_width, pw->y - pw->border_width,
               pw->width + (pw->border_width*2) + 1,
               pw->height + (pw->border_width*2) + 1,
               0, 0);
    XFreeGC (si->dpy, gc);
  }

  XMapRaised (si->dpy, si->passwd_dialog);
  XSync (si->dpy, False);

  si->pw_data = pw;

  draw_passwd_window (si);
  XSync (si->dpy, False);
}


void
draw_passwd_window (saver_info *si)
{
  passwd_dialog_data *pw = si->pw_data;
  XGCValues gcv;
  GC gc1, gc2;
  int spacing, height;
  int x1, x2, x3, y1, y2;
  int sw;
  int tb_height;

  height = (pw->heading_font->ascent + pw->heading_font->descent +
	    pw->body_font->ascent + pw->body_font->descent +
	    (2 * MAX ((pw->label_font->ascent + pw->label_font->descent),
		      (pw->passwd_font->ascent + pw->passwd_font->descent +
		       (pw->shadow_width * 4)))));
  spacing = ((pw->height - (2 * pw->shadow_width) -
	      pw->internal_border - height)) / 8;
  if (spacing < 0) spacing = 0;

  gcv.foreground = pw->foreground;
  gc1 = XCreateGC (si->dpy, si->passwd_dialog, GCForeground, &gcv);
  gc2 = XCreateGC (si->dpy, si->passwd_dialog, GCForeground, &gcv);
  x1 = pw->logo_width + pw->thermo_width + (pw->shadow_width * 3);
  x3 = pw->width - (pw->shadow_width * 2);
  y1 = (pw->shadow_width * 2) + spacing + spacing;

  /* top heading
   */
  XSetFont (si->dpy, gc1, pw->heading_font->fid);
  sw = string_width (pw->heading_font, pw->heading_label);
  x2 = (x1 + ((x3 - x1 - sw) / 2));
  y1 += spacing + pw->heading_font->ascent + pw->heading_font->descent;
  XDrawString (si->dpy, si->passwd_dialog, gc1, x2, y1,
	       pw->heading_label, strlen(pw->heading_label));

  /* text below top heading
   */
  XSetFont (si->dpy, gc1, pw->body_font->fid);
  y1 += spacing + pw->body_font->ascent + pw->body_font->descent;
  sw = string_width (pw->body_font, pw->body_label);
  x2 = (x1 + ((x3 - x1 - sw) / 2));
  XDrawString (si->dpy, si->passwd_dialog, gc1, x2, y1,
	       pw->body_label, strlen(pw->body_label));


  tb_height = (pw->passwd_font->ascent + pw->passwd_font->descent +
	       (pw->shadow_width * 4));

  /* the "User:" prompt
   */
  y1 += spacing;
  y2 = y1;
  XSetForeground (si->dpy, gc1, pw->foreground);
  XSetFont (si->dpy, gc1, pw->label_font->fid);
  y1 += (spacing + tb_height);
  x2 = (x1 + pw->internal_border +
	MAX(string_width (pw->label_font, pw->user_label),
	    string_width (pw->label_font, pw->passwd_label)));
  XDrawString (si->dpy, si->passwd_dialog, gc1,
	       x2 - string_width (pw->label_font, pw->user_label),
	       y1,
	       pw->user_label, strlen(pw->user_label));

  /* the "Password:" prompt
   */
  y1 += (spacing + tb_height);
  XDrawString (si->dpy, si->passwd_dialog, gc1,
	       x2 - string_width (pw->label_font, pw->passwd_label),
	       y1,
	       pw->passwd_label, strlen(pw->passwd_label));


  XSetForeground (si->dpy, gc2, pw->passwd_background);

  /* the "user name" text field
   */
  y1 = y2;
  XSetForeground (si->dpy, gc1, pw->passwd_foreground);
  XSetFont (si->dpy, gc1, pw->passwd_font->fid);
  y1 += (spacing + tb_height);
  x2 += (pw->shadow_width * 4);

  pw->passwd_field_width = x3 - x2 - pw->internal_border;
  pw->passwd_field_height = (pw->passwd_font->ascent +
			     pw->passwd_font->descent +
			     pw->shadow_width);

  XFillRectangle (si->dpy, si->passwd_dialog, gc2,
		  x2 - pw->shadow_width,
		  y1 - (pw->passwd_font->ascent + pw->passwd_font->descent),
		  pw->passwd_field_width, pw->passwd_field_height);
  XDrawString (si->dpy, si->passwd_dialog, gc1, x2, y1,
	       pw->user_string, strlen(pw->user_string));

  /* the "password" text field
   */
  y1 += (spacing + tb_height);

  pw->passwd_field_x = x2 - pw->shadow_width;
  pw->passwd_field_y = y1 - (pw->passwd_font->ascent +
			     pw->passwd_font->descent);

  /* The shadow around the text fields
   */
  y1 = y2;
  y1 += (spacing + (pw->shadow_width * 3));
  x1 = x2 - (pw->shadow_width * 2);
  x2 = pw->passwd_field_width + (pw->shadow_width * 2);
  y2 = pw->passwd_field_height + (pw->shadow_width * 2);

  draw_shaded_rectangle (si->dpy, si->passwd_dialog,
			 x1, y1, x2, y2,
			 pw->shadow_width,
			 pw->shadow_bottom, pw->shadow_top);

  y1 += (spacing + pw->passwd_font->ascent + pw->passwd_font->descent +
	 (pw->shadow_width * 4));
  draw_shaded_rectangle (si->dpy, si->passwd_dialog,
			 x1, y1, x2, y2,
			 pw->shadow_width,
			 pw->shadow_bottom, pw->shadow_top);

  /* the logo
   */
  XSetForeground (si->dpy, gc1, pw->logo_foreground);
  XSetForeground (si->dpy, gc2, pw->logo_background);

  x1 = pw->shadow_width * 3;
  y1 = pw->shadow_width * 3;
  x2 = pw->logo_width - (pw->shadow_width * 6);
  y2 = pw->logo_height - (pw->shadow_width * 6);

  XFillRectangle (si->dpy, si->passwd_dialog, gc2, x1, y1, x2, y2);
  skull (si->dpy, si->passwd_dialog, gc1, gc2,
	 x1 + pw->shadow_width, y1 + pw->shadow_width,
	 x2 - (pw->shadow_width * 2), y2 - (pw->shadow_width * 2));

  /* The thermometer
   */
  pw->thermo_field_x = pw->logo_width + pw->shadow_width;
  pw->thermo_field_y = pw->shadow_width * 3;
  pw->thermo_field_height = pw->height - (pw->shadow_width * 6);

  /* Solid border inside the logo box. */
  XSetForeground (si->dpy, gc1, pw->foreground);
  XDrawRectangle (si->dpy, si->passwd_dialog, gc1, x1, y1, x2-1, y2-1);

  /* The shadow around the logo
   */
  draw_shaded_rectangle (si->dpy, si->passwd_dialog,
			 pw->shadow_width * 2,
			 pw->shadow_width * 2,
			 pw->logo_width - (pw->shadow_width * 4),
			 pw->logo_height - (pw->shadow_width * 4),
			 pw->shadow_width,
			 pw->shadow_bottom, pw->shadow_top);

  /* The shadow around the thermometer
   */
  draw_shaded_rectangle (si->dpy, si->passwd_dialog,
			 pw->logo_width,
			 pw->shadow_width * 2,
			 pw->thermo_width + (pw->shadow_width * 2),
			 pw->height - (pw->shadow_width * 4),
			 pw->shadow_width,
			 pw->shadow_bottom, pw->shadow_top);

  /* Solid border inside the thermometer. */
  XSetForeground (si->dpy, gc1, pw->foreground);
  XDrawRectangle (si->dpy, si->passwd_dialog, gc1, 
		  pw->logo_width + pw->shadow_width,
		  pw->shadow_width * 3,
		  pw->thermo_width - 1,
		  pw->height - (pw->shadow_width * 6) - 1);

  /* The shadow around the whole window
   */
  draw_shaded_rectangle (si->dpy, si->passwd_dialog,
			 0, 0, pw->width, pw->height, pw->shadow_width,
			 pw->shadow_top, pw->shadow_bottom);

  XFreeGC (si->dpy, gc1);
  XFreeGC (si->dpy, gc2);

  update_passwd_window (si, pw->passwd_string, pw->ratio);
}


void
update_passwd_window (saver_info *si, const char *printed_passwd, float ratio)
{
  passwd_dialog_data *pw = si->pw_data;
  XGCValues gcv;
  GC gc1, gc2;
  int x, y;

  pw->ratio = ratio;
  gcv.foreground = pw->passwd_foreground;
  gcv.font = pw->passwd_font->fid;
  gc1 = XCreateGC (si->dpy, si->passwd_dialog, GCForeground|GCFont, &gcv);
  gcv.foreground = pw->passwd_background;
  gc2 = XCreateGC (si->dpy, si->passwd_dialog, GCForeground, &gcv);

  if (printed_passwd)
    {
      char *s = strdup (printed_passwd);
      if (pw->passwd_string) free (pw->passwd_string);
      pw->passwd_string = s;
    }

  /* the "password" text field
   */
  XFillRectangle (si->dpy, si->passwd_dialog, gc2,
		  pw->passwd_field_x, pw->passwd_field_y,
		  pw->passwd_field_width, pw->passwd_field_height);
  XDrawString (si->dpy, si->passwd_dialog, gc1,
	       pw->passwd_field_x + pw->shadow_width,
	       pw->passwd_field_y + (pw->passwd_font->ascent +
				     pw->passwd_font->descent),
	       pw->passwd_string, strlen(pw->passwd_string));

  /* The I-beam
   */
  if (pw->i_beam != 0)
    {
      x = (pw->passwd_field_x + pw->shadow_width +
	   string_width (pw->passwd_font, pw->passwd_string));
      y = pw->passwd_field_y + pw->shadow_width;
      XDrawLine (si->dpy, si->passwd_dialog, gc1, 
		 x, y, x, y + pw->passwd_font->ascent);
    }

  pw->i_beam = (pw->i_beam + 1) % 4;


  /* the thermometer
   */
  y = pw->thermo_field_height * (1.0 - pw->ratio);
  if (y > 0)
    {
      XFillRectangle (si->dpy, si->passwd_dialog, gc2,
		      pw->thermo_field_x + 1,
		      pw->thermo_field_y + 1,
		      pw->thermo_width-2,
		      y);
      XSetForeground (si->dpy, gc1, pw->logo_foreground);
      XFillRectangle (si->dpy, si->passwd_dialog, gc1,
		      pw->thermo_field_x + 1,
		      pw->thermo_field_y + 1 + y,
		      pw->thermo_width-2,
		      MAX (0, pw->thermo_field_height - y - 2));
    }

  XFreeGC (si->dpy, gc1);
  XFreeGC (si->dpy, gc2);
  XSync (si->dpy, False);
}


void
destroy_passwd_window (saver_info *si)
{
  passwd_dialog_data *pw = si->pw_data;
  Screen *screen = si->default_screen->screen;
  Colormap cmap = DefaultColormapOfScreen (screen);
  Pixel black = BlackPixelOfScreen (screen);
  Pixel white = WhitePixelOfScreen (screen);

  if (pw->timer)
    XtRemoveTimeOut (pw->timer);

  if (si->passwd_dialog)
    {
      XDestroyWindow (si->dpy, si->passwd_dialog);
      si->passwd_dialog = 0;
    }
  
  if (pw->save_under)
    {
      XGCValues gcv;
      GC gc;
      gcv.function = GXcopy;
      gc = XCreateGC (si->dpy, si->default_screen->screensaver_window,
                      GCFunction, &gcv);
      XCopyArea (si->dpy, pw->save_under,
                 si->default_screen->screensaver_window, gc,
                 0, 0,
                 pw->width + (pw->border_width*2) + 1,
                 pw->height + (pw->border_width*2) + 1,
                 pw->x - pw->border_width, pw->y - pw->border_width);
      XFreePixmap (si->dpy, pw->save_under);
      pw->save_under = 0;
      XFreeGC (si->dpy, gc);
    }

  if (pw->heading_label) free (pw->heading_label);
  if (pw->body_label)    free (pw->body_label);
  if (pw->user_label)    free (pw->user_label);
  if (pw->passwd_label)  free (pw->passwd_label);

  if (pw->heading_font) XFreeFont (si->dpy, pw->heading_font);
  if (pw->body_font)    XFreeFont (si->dpy, pw->body_font);
  if (pw->label_font)   XFreeFont (si->dpy, pw->label_font);
  if (pw->passwd_font)  XFreeFont (si->dpy, pw->passwd_font);

  if (pw->foreground != black && pw->foreground != white)
    XFreeColors (si->dpy, cmap, &pw->foreground, 1, 0L);
  if (pw->background != black && pw->background != white)
    XFreeColors (si->dpy, cmap, &pw->background, 1, 0L);
  if (pw->passwd_foreground != black && pw->passwd_foreground != white)
    XFreeColors (si->dpy, cmap, &pw->passwd_foreground, 1, 0L);
  if (pw->passwd_background != black && pw->passwd_background != white)
    XFreeColors (si->dpy, cmap, &pw->passwd_background, 1, 0L);
  if (pw->logo_foreground != black && pw->logo_foreground != white)
    XFreeColors (si->dpy, cmap, &pw->logo_foreground, 1, 0L);
  if (pw->logo_background != black && pw->logo_background != white)
    XFreeColors (si->dpy, cmap, &pw->logo_background, 1, 0L);
  if (pw->shadow_top != black && pw->shadow_top != white)
    XFreeColors (si->dpy, cmap, &pw->shadow_top, 1, 0L);
  if (pw->shadow_bottom != black && pw->shadow_bottom != white)
    XFreeColors (si->dpy, cmap, &pw->shadow_bottom, 1, 0L);

  memset (pw, 0, sizeof(*pw));
  free (pw);

  si->pw_data = 0;
}


/* Interactions
 */

static void
passwd_animate_timer (XtPointer closure, XtIntervalId *id)
{
  saver_info *si = (saver_info *) closure;
  int tick = 166;
  passwd_dialog_data *pw = si->pw_data;

  if (!pw) return;

  pw->ratio -= (1.0 / ((double) si->prefs.passwd_timeout / (double) tick));
  if (pw->ratio < 0)
    {
      pw->ratio = 0;
      if (pw->state == pw_read)
	pw->state = pw_time;
    }

  update_passwd_window (si, 0, pw->ratio);

  if (pw->state == pw_read)
    pw->timer = XtAppAddTimeOut (si->app, tick, passwd_animate_timer,
				 (XtPointer) si);
  else
    pw->timer = 0;

  idle_timer ((XtPointer) si, id);
}


static void
handle_passwd_key (saver_info *si, XKeyEvent *event)
{
  saver_preferences *p = &si->prefs;
  passwd_dialog_data *pw = si->pw_data;
  int pw_size = sizeof (pw->typed_passwd) - 1;
  char *typed_passwd = pw->typed_passwd;
  char s[2];
  char *stars = 0;
  int i;
  int size = XLookupString (event, s, 1, 0, 0);

  if (size != 1) return;

  s[1] = 0;

  switch (*s)
    {
    case '\010': case '\177':				/* Backspace */
      if (!*typed_passwd)
	XBell (si->dpy, 0);
      else
	typed_passwd [strlen(typed_passwd)-1] = 0;
      break;

    case '\025': case '\030':				/* Erase line */
      memset (typed_passwd, 0, pw_size);
      break;

    case '\012': case '\015':				/* Enter */
      if (pw->state != pw_read)
	;  /* already done? */
      else if (typed_passwd[0] == 0)
	pw->state = pw_null;
      else
        {
          update_passwd_window (si, "Checking...", pw->ratio);
          XSync (si->dpy, False);
          if (passwd_valid_p (typed_passwd, p->verbose_p))
            pw->state = pw_ok;
          else
            pw->state = pw_fail;
          update_passwd_window (si, "", pw->ratio);
        }
      break;

    default:
      i = strlen (typed_passwd);
      if (i >= pw_size-1)
	XBell (si->dpy, 0);
      else
	{
	  typed_passwd [i] = *s;
	  typed_passwd [i+1] = 0;
	}
      break;
    }

  i = strlen(typed_passwd);
  stars = (char *) malloc(i+1);
  memset (stars, '*', i);
  stars[i] = 0;
  update_passwd_window (si, stars, pw->ratio);
  free (stars);
}


static void
passwd_event_loop (saver_info *si)
{
  saver_preferences *p = &si->prefs;
  char *msg = 0;
  XEvent event;
  passwd_animate_timer ((XtPointer) si, 0);

  while (si->pw_data && si->pw_data->state == pw_read)
    {
      XtAppNextEvent (si->app, &event);
      if (event.xany.window == si->passwd_dialog && event.xany.type == Expose)
	draw_passwd_window (si);
      else if (event.xany.type == KeyPress)
	handle_passwd_key (si, &event.xkey);
      else
	XtDispatchEvent (&event);
    }

  switch (si->pw_data->state)
    {
    case pw_ok:   msg = 0; break;
    case pw_null: msg = ""; break;
    case pw_time: msg = "Timed out!"; break;
    default:      msg = "Sorry!"; break;
    }

  if (si->pw_data->state == pw_fail)
    si->unlock_failures++;

  if (p->verbose_p)
    switch (si->pw_data->state)
      {
      case pw_ok:
	fprintf (stderr, "%s: password correct.\n", blurb()); break;
      case pw_fail:
	fprintf (stderr, "%s: password incorrect!\n", blurb()); break;
      case pw_null:
      case pw_cancel:
	fprintf (stderr, "%s: password entry cancelled.\n", blurb()); break;
      case pw_time:
	fprintf (stderr, "%s: password entry timed out.\n", blurb()); break;
      default: break;
      }

#ifdef HAVE_SYSLOG
  if (si->pw_data->state == pw_fail)
    {
      /* If they typed a password (as opposed to just hitting return) and
	 the password was invalid, log it.
      */
      struct passwd *pw = getpwuid (getuid ());
      char *d = DisplayString (si->dpy);
      char *u = (pw->pw_name ? pw->pw_name : "???");
      int opt = 0;
      int fac = 0;

# ifdef LOG_PID
      opt = LOG_PID;
# endif

# if defined(LOG_AUTHPRIV)
      fac = LOG_AUTHPRIV;
# elif defined(LOG_AUTH)
      fac = LOG_AUTH;
# else
      fac = LOG_DAEMON;
# endif

      if (!d) d = "";
      openlog (progname, opt, fac);
      syslog (LOG_NOTICE, "FAILED LOGIN %d ON DISPLAY \"%s\", FOR \"%s\"",
	      si->unlock_failures, d, u);
      closelog ();
    }
#endif /* HAVE_SYSLOG */

  if (si->pw_data->state == pw_fail)
    XBell (si->dpy, False);

  if (si->pw_data->state == pw_ok && si->unlock_failures != 0)
    {
      if (si->unlock_failures == 1)
	fprintf (real_stderr,
		 "%s: WARNING: 1 failed attempt to unlock the screen.\n",
		 blurb());
      else
	fprintf (real_stderr,
		 "%s: WARNING: %d failed attempts to unlock the screen.\n",
		 blurb(), si->unlock_failures);
      fflush (real_stderr);

      si->unlock_failures = 0;
    }

  if (msg)
    {
      si->pw_data->i_beam = 0;
      update_passwd_window (si, msg, 0.0);
      XSync (si->dpy, False);
      sleep (1);

      /* Swallow all pending KeyPress/KeyRelease events. */
      {
	XEvent e;
	while (XCheckMaskEvent (si->dpy, KeyPressMask|KeyReleaseMask, &e))
	  ;
      }
    }
}


Bool
unlock_p (saver_info *si)
{
  saver_preferences *p = &si->prefs;
  Screen *screen = si->default_screen->screen;
  Colormap cmap = DefaultColormapOfScreen (screen);
  Bool status;

  if (p->verbose_p)
    fprintf (stderr, "%s: prompting for password.\n", blurb());

  if (si->pw_data || si->passwd_dialog)
    destroy_passwd_window (si);

  make_passwd_window (si);
  if (cmap) XInstallColormap (si->dpy, cmap);

  passwd_event_loop (si);

  status = (si->pw_data->state == pw_ok);
  destroy_passwd_window (si);

  cmap = si->default_screen->cmap;
  if (cmap) XInstallColormap (si->dpy, cmap);

  return status;
}

#endif /* !NO_LOCKING -- whole file */
