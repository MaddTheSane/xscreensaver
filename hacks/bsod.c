/* xscreensaver, Copyright (c) 1998 Jamie Zawinski <jwz@netscape.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * Blue Screen of Death: the finest in personal computer emulation.
 * Concept cribbed from Stephen Martin <smartin@mks.com>;
 * this version written by jwz, 4-Jun-98.
 *
 *   TODO:
 *      -  Should have a "macsbug" mode.
 *      -  Should simulate a Unix kernel panic and reboot.
 *      -  Making various boot noises would be fun, too.
 *      -  Maybe scatter some random bits across the screen,
 *         to simulate corruption of video ram?
 *      -  Should randomize the various hex numbers printed.
 */

#include "screenhack.h"
#include <stdio.h>
#include <X11/Xutil.h>

#ifdef HAVE_XPM
# include <X11/xpm.h>
# include "images/amiga.xpm"
#endif

#include "images/mac.xbm"


static void
draw_string (Display *dpy, Window window, GC gc, XGCValues *gcv,
	     XFontStruct *font, int win_width, int win_height,
	     const char *string, int delay)
{
  int x, y;
  int width = 0, height = 0, cw = 0;
  int char_width, line_height;

  const char *s = string;
  const char *se = string;

  /* This pretty much assumes fixed-width fonts */
  char_width = (font->per_char
		? font->per_char['n'-font->min_char_or_byte2].width
		: font->min_bounds.width);
  line_height = font->ascent + font->descent + 1;

  while (1)
    {
      if (*s == '\n' || !*s)
	{
	  height++;
	  if (cw > width) width = cw;
	  cw = 0;
	  if (!*s) break;
	}
      else
	cw++;
      s++;
    }

  x = (win_width - (width * char_width)) / 2;
  y = (win_height - (height * line_height)) / 2;

  if (x < char_width) x = char_width;
  if (y < line_height) y = line_height;

  se = s = string;
  while (1)
    {
      if (*s == '\n' || !*s)
	{
	  int off = 0;
	  Bool flip = False;

	  if (*se == '@' || *se == '_')
	    {
	      if (*se == '@') flip = True;
	      se++;
	      off = (char_width * (width - (s - se))) / 2;
	    }

	  if (flip)
	    {
	      XSetForeground(dpy, gc, gcv->background);
	      XSetBackground(dpy, gc, gcv->foreground);
	    }

	  if (s != se)
	    XDrawImageString(dpy, window, gc, x+off, y+font->ascent, se, s-se);

	  if (flip)
	    {
	      XSetForeground(dpy, gc, gcv->foreground);
	      XSetBackground(dpy, gc, gcv->background);
	    }

	  se = s;
	  y += line_height;
	  if (!*s) break;
	  se = s+1;

	  if (delay)
	    {
	      XSync(dpy, False);
	      usleep(delay);
	    }
	}
      s++;
    }
}


static Pixmap
double_pixmap(Display *dpy, GC gc, Visual *visual, int depth, Pixmap pixmap,
	     int pix_w, int pix_h)
{
  int x, y;
  Pixmap p2 = XCreatePixmap(dpy, pixmap, pix_w*2, pix_h*2, depth);
  XImage *i1 = XGetImage(dpy, pixmap, 0, 0, pix_w, pix_h, ~0L, ZPixmap);
  XImage *i2 = XCreateImage(dpy, visual, depth, ZPixmap, 0, 0,
			    pix_w*2, pix_h*2, 8, 0);
  i2->data = (unsigned char *) calloc(i2->height, i2->bytes_per_line);
  for (y = 0; y < pix_h; y++)
    for (x = 0; x < pix_w; x++)
      {
	unsigned long p = XGetPixel(i1, x, y);
	XPutPixel(i2, x*2,   y*2,   p);
	XPutPixel(i2, x*2+1, y*2,   p);
	XPutPixel(i2, x*2,   y*2+1, p);
	XPutPixel(i2, x*2+1, y*2+1, p);
      }
  free(i1->data); i1->data = 0;
  XDestroyImage(i1);
  XPutImage(dpy, p2, gc, i2, 0, 0, 0, 0, i2->width, i2->height);
  free(i2->data); i2->data = 0;
  XDestroyImage(i2);
  XFreePixmap(dpy, pixmap);
  return p2;
}


/* Sleep for N seconds and return False.  But if a key or mouse event is
   seen, discard all pending key or mouse events, and return True.
 */
static Bool
bsod_sleep(Display *dpy, int seconds)
{
  XEvent event;
  int q = seconds * 4;
  int mask = KeyPressMask|ButtonPressMask;
  while (q > 0)
    {
      XSync(dpy, False);
      if (XCheckMaskEvent(dpy, mask, &event))
	{
	  while (XCheckMaskEvent(dpy, mask, &event))
	    ;
	  return True;
	}
      q--;
      usleep(250000);
    }
  return False; 
}


static void
windows (Display *dpy, Window window, int delay, Bool w95p)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  char *fontname;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc;

  const char *w95 =
    ("@Windows\n"
     "A fatal exception 0E has occured at F0AD:42494C4C\n"
     "the current application will be terminated.\n"
     "\n"
     "* Press any key to terminate the current application.\n"
     "* Press CTRL+ALT+DELETE again to restart your computer.\n"
     "  You will lose any unsaved information in all applications.\n"
     "\n"
     "\n"
     "_Press any key to continue");

  const char *wnt =
    ("*** STOP: 0x0000001E (0x80000003,0x80106fc0,0x8025ea21,0xfd6829e8)\n"
   "Unhandled Kernel exception c0000047 from fa8418b4 (8025ea21,fd6829e8)\n"
   "\n"
   "Dll Base Date Stamp - Name             Dll Base Date Stamp - Name\n"
   "80100000 2be154c9 - ntoskrnl.exe       80400000 2bc153b0 - hal.dll\n"
   "80258000 2bd49628 - ncrc710.sys        8025c000 2bd49688 - SCSIPORT.SYS \n"
   "80267000 2bd49683 - scsidisk.sys       802a6000 2bd496b9 - Fastfat.sys\n"
   "fa800000 2bd49666 - Floppy.SYS         fa810000 2bd496db - Hpfs_Rec.SYS\n"
   "fa820000 2bd49676 - Null.SYS           fa830000 2bd4965a - Beep.SYS\n"
   "fa840000 2bdaab00 - i8042prt.SYS       fa850000 2bd5a020 - SERMOUSE.SYS\n"
   "fa860000 2bd4966f - kbdclass.SYS       fa870000 2bd49671 - MOUCLASS.SYS\n"
   "fa880000 2bd9c0be - Videoprt.SYS       fa890000 2bd49638 - NCC1701E.SYS\n"
   "fa8a0000 2bd4a4ce - Vga.SYS            fa8b0000 2bd496d0 - Msfs.SYS\n"
   "fa8c0000 2bd496c3 - Npfs.SYS           fa8e0000 2bd496c9 - Ntfs.SYS\n"
   "fa940000 2bd496df - NDIS.SYS           fa930000 2bd49707 - wdlan.sys\n"
   "fa970000 2bd49712 - TDI.SYS            fa950000 2bd5a7fb - nbf.sys\n"
   "fa980000 2bd72406 - streams.sys        fa9b0000 2bd4975f - ubnb.sys\n"
   "fa9c0000 2bd5bfd7 - usbser.sys         fa9d0000 2bd4971d - netbios.sys\n"
   "fa9e0000 2bd49678 - Parallel.sys       fa9f0000 2bd4969f - serial.SYS\n"
   "faa00000 2bd49739 - mup.sys            faa40000 2bd4971f - SMBTRSUP.SYS\n"
   "faa10000 2bd6f2a2 - srv.sys            faa50000 2bd4971a - afd.sys\n"
   "faa60000 2bd6fd80 - rdr.sys            faaa0000 2bd49735 - bowser.sys\n"
   "\n"
   "Address dword dump Dll Base                                      - Name\n"
   "801afc20 80106fc0 80106fc0 00000000 00000000 80149905 : "
     "fa840000 - i8042prt.SYS\n"
   "801afc24 80149905 80149905 ff8e6b8c 80129c2c ff8e6b94 : "
     "8025c000 - SCSIPORT.SYS\n"
   "801afc2c 80129c2c 80129c2c ff8e6b94 00000000 ff8e6b94 : "
     "80100000 - ntoskrnl.exe\n"
   "801afc34 801240f2 80124f02 ff8e6df4 ff8e6f60 ff8e6c58 : "
     "80100000 - ntoskrnl.exe\n"
   "801afc54 80124f16 80124f16 ff8e6f60 ff8e6c3c 8015ac7e : "
     "80100000 - ntoskrnl.exe\n"
   "801afc64 8015ac7e 8015ac7e ff8e6df4 ff8e6f60 ff8e6c58 : "
     "80100000 - ntoskrnl.exe\n"
   "801afc70 80129bda 80129bda 00000000 80088000 80106fc0 : "
     "80100000 - ntoskrnl.exe\n"
   "\n"
   "Kernel Debugger Using: COM2 (Port 0x2f8, Baud Rate 19200)\n"
   "Restart and set the recovery options in the system control panel\n"
   "or the /CRASHDEBUG system start option. If this message reappears,\n"
   "contact your system administrator or technical support group."
     );

  XGetWindowAttributes (dpy, window, &xgwa);

  fontname = get_string_resource ((xgwa.height > 600
				   ? (w95p
				      ? "windows95.font2"
				      : "windowsNT.font2")
				   : (w95p
				      ? "windows95.font"
				      : "windowsNT.font")),
				  "Windows.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource((w95p
				       ? "windows95.foreground"
				       : "windowsNT.foreground"),
				      "Windows.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource((w95p
				       ? "windows95.background"
				       : "windowsNT.background"),
				      "Windows.Background",
				      dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  if (w95p)
    draw_string(dpy, window, gc, &gcv, font, xgwa.width, xgwa.height, w95, 0);
  else
    draw_string(dpy, window, gc, &gcv, font, 10, 10, wnt, 750);

  XFreeGC(dpy, gc);
  XSync(dpy, False);
  bsod_sleep(dpy, delay);
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
}

static void
amiga (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  char *fontname;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc, gc2;
  int height;
  unsigned long fg, bg, bg2;
  Pixmap pixmap = 0;
  int pix_w, pix_h;

  const char *string =
    ("_Software failure.  Press left mouse button to continue.\n"
     "_Guru Meditation #00000003.00C01570");

  XGetWindowAttributes (dpy, window, &xgwa);

  fontname = get_string_resource ((xgwa.height > 600
				   ? "amiga.font2" : "amiga.font"),
				  "Amiga.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  fg = gcv.foreground = get_pixel_resource("amiga.foreground",
					   "Amiga.Foreground",
					   dpy, xgwa.colormap);
  bg = gcv.background = get_pixel_resource("amiga.background",
					   "Amiga.Background",
					   dpy, xgwa.colormap);
  bg2 = get_pixel_resource("amiga.background2", "Amiga.Background",
			   dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, bg2);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);
  gcv.background = fg; gcv.foreground = bg;
  gc2 = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  height = (font->ascent + font->descent) * 6;

#ifdef HAVE_XPM
  {
    XpmAttributes xpmattrs;
    int result;
    xpmattrs.valuemask = 0;

# ifdef XpmCloseness
    xpmattrs.valuemask |= XpmCloseness;
    xpmattrs.closeness = 40000;
# endif
# ifdef XpmVisual
    xpmattrs.valuemask |= XpmVisual;
    xpmattrs.visual = xgwa.visual;
# endif
# ifdef XpmDepth
    xpmattrs.valuemask |= XpmDepth;
    xpmattrs.depth = xgwa.depth;
# endif
# ifdef XpmColormap
    xpmattrs.valuemask |= XpmColormap;
    xpmattrs.colormap = xgwa.colormap;
# endif

    result = XpmCreatePixmapFromData(dpy, window, amiga_hand,
				     &pixmap, 0 /* mask */, &xpmattrs);
    if (!pixmap || (result != XpmSuccess && result != XpmColorError))
      pixmap = 0;
    pix_w = xpmattrs.width;
    pix_h = xpmattrs.height;
  }
#endif /* HAVE_XPM */

  if (pixmap && xgwa.height > 600)	/* scale up the bitmap */
    {
      pixmap = double_pixmap(dpy, gc, xgwa.visual, xgwa.depth,
			     pixmap, pix_w, pix_h);
      pix_w *= 2;
      pix_h *= 2;
    }

  if (pixmap)
    {
      int x = (xgwa.width - pix_w) / 2;
      int y = ((xgwa.height - pix_h) / 2);
      XCopyArea(dpy, pixmap, window, gc, 0, 0, pix_w, pix_h, x, y);

      XSync(dpy, False);
      bsod_sleep(dpy, 2);

      XCopyArea(dpy, pixmap, window, gc, 0, 0, pix_w, pix_h, x, y + height);
      XClearArea(dpy, window, 0, 0, xgwa.width, y + height, False);
      XFreePixmap(dpy, pixmap);
    }

  XFillRectangle(dpy, window, gc2, 0, 0, xgwa.width, height);
  draw_string(dpy, window, gc, &gcv, font, xgwa.width, height, string, 0);

  {
    GC gca = gc;
    while (delay > 0)
      {
	XFillRectangle(dpy, window, gca, 0, 0, xgwa.width, font->ascent);
	XFillRectangle(dpy, window, gca, 0, 0, font->ascent, height);
	XFillRectangle(dpy, window, gca, xgwa.width-font->ascent, 0,
		       font->ascent, height);
	XFillRectangle(dpy, window, gca, 0, height-font->ascent, xgwa.width,
		       font->ascent);
	gca = (gca == gc ? gc2 : gc);
	XSync(dpy, False);
	if (bsod_sleep(dpy, 1))
	  break;
	delay--;
      }
  }

  XFreeGC(dpy, gc);
  XFreeGC(dpy, gc2);
  XSync(dpy, False);
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
}


static void
mac (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  char *fontname;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc;
  Pixmap pixmap = 0;
  int pix_w = mac_width;
  int pix_h = mac_height;
  int offset = mac_height * 4;
  int i;

  const char *string = ("0 0 0 0 0 0 0 F\n"
			"0 0 0 0 0 0 0 3");

  XGetWindowAttributes (dpy, window, &xgwa);

  fontname = get_string_resource ("mac.font", "Mac.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource("mac.foreground", "Mac.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource("mac.background", "Mac.Background",
				      dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  pixmap = XCreatePixmapFromBitmapData(dpy, window, (char *) mac_bits,
				       mac_width, mac_height,
				       gcv.foreground,
				       gcv.background,
				       xgwa.depth);

  draw_string(dpy, window, gc, &gcv, font, xgwa.width, xgwa.height + offset,
	      string, 0);

  for(i = 0; i < 2; i++)
    {
      pixmap = double_pixmap(dpy, gc, xgwa.visual, xgwa.depth,
			     pixmap, pix_w, pix_h);
      pix_w *= 2; pix_h *= 2;
    }

  {
    int x = (xgwa.width - pix_w) / 2;
    int y = (((xgwa.height + offset) / 2) -
	     pix_h -
	     (font->ascent + font->descent) * 2);
    if (y < 0) y = 0;
    XCopyArea(dpy, pixmap, window, gc, 0, 0, pix_w, pix_h, x, y);
    XFreePixmap(dpy, pixmap);
  }

  XFreeGC(dpy, gc);
  XSync(dpy, False);
  bsod_sleep(dpy, delay);
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
}


char *progclass = "BSOD";

char *defaults [] = {
  "*delay:			30",

  "BSOD.Windows.font:		-*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  "BSOD.Windows.font2:		-*-courier-bold-r-*-*-*-180-*-*-m-*-*-*",
  "BSOD.Windows.foreground:	White",
  "BSOD.Windows.background:	Blue",

  "BSOD.Amiga.font:		-*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  "BSOD.Amiga.font2:		-*-courier-bold-r-*-*-*-180-*-*-m-*-*-*",
  "BSOD.Amiga.foreground:	Red",
  "BSOD.Amiga.background:	Black",
  "BSOD.Amiga.background2:	White",

  "BSOD.Mac.font:		-*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  "BSOD.Mac.foreground:		PaleTurquoise1",
  "BSOD.Mac.background:		Black",
  0
};

XrmOptionDescRec options [] = {
  { "-delay",		".delay",		XrmoptionSepArg, 0 },
  { 0, 0, 0, 0 }
};

void
screenhack (Display *dpy, Window window)
{
  int i = -1;
  int j = -1;
  int delay = get_integer_resource ("delay", "Integer");
  if (delay < 3) delay = 3;

  if (!get_boolean_resource ("root", "Boolean"))
    XSelectInput(dpy, window, KeyPressMask|ButtonPressMask);

  while (1)
    {
      while (i == j) i = random() % 4;
      j = i;

      switch (i)
	{
	case 0: windows(dpy, window, delay, True); break;
	case 1: windows(dpy, window, delay, False); break;
	case 2: amiga(dpy, window, delay); break;
	case 3: mac(dpy, window, delay); break;
	default: abort(); break;
	}
      XSync (dpy, True);
    }
}
