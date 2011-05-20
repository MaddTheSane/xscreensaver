/* xscreensaver, Copyright (c) 1998-2003 Jamie Zawinski <jwz@jwz.org>
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
 */

#include "screenhack.h"
#include "xpm-pixmap.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xutil.h>

#ifdef HAVE_UNAME
# include <sys/utsname.h>
#endif /* HAVE_UNAME */

#include "images/amiga.xpm"
#include "images/atari.xbm"
#include "images/mac.xbm"
#include "images/macbomb.xbm"
#include "images/hmac.xpm"


static int
draw_string (Display *dpy, Window window, GC gc, XGCValues *gcv,
	     XFontStruct *font,
	     int xoff, int yoff,
	     int win_width, int win_height,
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

  if (x < 0) x = 2;
  if (y < 0) y = 2;

  x += xoff;
  y += yoff;

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

  return width * char_width;
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
  i2->data = (char *) calloc(i2->height, i2->bytes_per_line);
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
  int q = seconds * 4;
  int quantum = 250000;

  if (seconds == -1)
    q = 1, quantum = 100000;

  do
    {
      XSync(dpy, False);
      while (XPending (dpy))
        {
          XEvent event;
          XNextEvent (dpy, &event);
          if (event.xany.type == ButtonPress)
            return True;
          if (event.xany.type == KeyPress)
            {
              KeySym keysym;
              char c = 0;
              XLookupString (&event.xkey, &c, 1, &keysym, 0);
              if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                return True;
            }
          screenhack_handle_event (dpy, &event);
        }

      if (q > 0)
	{
	  q--;
	  usleep(quantum);
	}
    }
  while (q > 0);

  return False; 
}


static Bool
windows (Display *dpy, Window window, int delay, int which)
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

  const char *wnt = /* from Jim Niemira <urmane@urmane.org> */
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

  const char *w2ka =
    ("*** STOP: 0x000000D1 (0xE1D38000,0x0000001C,0x00000000,0xF09D42DA)\n"
     "DRIVER_IRQL_NOT_LESS_OR_EQUAL \n"
     "\n"
    "*** Address F09D42DA base at F09D4000, DateStamp 39f459ff - CRASHDD.SYS\n"
     "\n"
     "Beginning dump of physical memory\n");
  const char *w2kb =
    ("Physical memory dump complete. Contact your system administrator or\n"
     "technical support group.\n");

  const char *wmea =
    ("    Windows protection error.  You need to restart your computer.");
  const char *wmeb =
    ("    System halted.");

  if (which < 0 || which > 2) abort();

  /* kludge to lump Win2K and WinME together; seems silly to add another
     preference/command line option just for this little one. */
  if (which == 2 && (random() % 2))
    which = 3;

  if (!get_boolean_resource((which == 0 ? "doWindows" :
                             which == 1 ? "doNT" :
                             which == 2 ? "doWin2K" :
                                          "doWin2K"), /* "doWinME" ? */
                            "DoWindows"))
    return False;

  XGetWindowAttributes (dpy, window, &xgwa);

  fontname = get_string_resource ((xgwa.height > 600
				   ? (which == 0 ? "windows95.font2" :
                                      which == 1 ? "windowsNT.font2" :
                                      which == 2 ? "windows2K.font2" :
                                                   "windowsME.font2")
				   : (which == 0 ? "windows95.font" :
				      which == 1 ? "windowsNT.font" :
				      which == 2 ? "windows2K.font" :
                                                   "windowsME.font")),
				  "Windows.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource((which == 0 ? "windows95.foreground" :
				       which == 1 ? "windowsNT.foreground" :
				       which == 2 ? "windows2K.foreground" :
                                                    "windowsME.foreground"),
				      "Windows.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource((which == 0 ? "windows95.background" :
				       which == 1 ? "windowsNT.background" :
				       which == 2 ? "windows2K.background" :
                                                    "windowsME.background"),
				      "Windows.Background",
				      dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  if (which == 0)
    draw_string(dpy, window, gc, &gcv, font,
		0, 0, xgwa.width, xgwa.height, w95, 0);
  else if (which == 1)
    draw_string(dpy, window, gc, &gcv, font, 0, 0, 10, 10, wnt, 750);
  else if (which == 2)
    {
      int line_height = font->ascent + font->descent + 1;
      int x = 20;
      int y = (xgwa.height / 4);

      draw_string(dpy, window, gc, &gcv, font, x, y, 10, 10, w2ka, 750);
      y += line_height * 6;
      bsod_sleep(dpy, 4);
      draw_string(dpy, window, gc, &gcv, font, x, y, 10, 10, w2kb, 750);
    }
  else if (which == 3)
    {
      int line_height = font->ascent + font->descent;
      int x = 0;
      int y = (xgwa.height - line_height * 3) / 2;
      draw_string (dpy, window, gc, &gcv, font, x, y, 10, 10, wmea, 0);
      y += line_height * 2;
      x = draw_string (dpy, window, gc, &gcv, font, x, y, 10, 10, wmeb, 0);
      y += line_height;
      while (delay > 0)
        {
          XDrawImageString (dpy, window, gc, x, y, "_", 1);
          XSync(dpy, False);
          usleep(120000L);
          XDrawImageString (dpy, window, gc, x, y, " ", 1);
          XSync(dpy, False);
          usleep(120000L);
          if (bsod_sleep(dpy, 0))
            delay = 0;
          else
            delay--;
        }
    }
  else
    abort();

  XFreeGC(dpy, gc);
  XSync(dpy, False);
  bsod_sleep(dpy, delay);
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
  return True;
}

/* SCO OpenServer 5 panic, by Tom Kelly <tom@ancilla.toronto.on.ca>
 */
static Bool
sco (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  char *fontname;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc;
  int lines_1 = 0, lines_2 = 0, lines_3 = 0, lines_4 = 0;
  const char *s;

  const char *sco_panic_1 =
    ("Unexpected trap in kernel mode:\n"
     "\n"
     "cr0 0x80010013     cr2  0x00000014     cr3 0x00000000  tlb  0x00000000\n"
     "ss  0x00071054    uesp  0x00012055     efl 0x00080888  ipl  0x00000005\n"
     "cs  0x00092585     eip  0x00544a4b     err 0x004d4a47  trap 0x0000000E\n"
     "eax 0x0045474b     ecx  0x0042544b     edx 0x57687920  ebx  0x61726520\n"
     "esp 0x796f7520     ebp  0x72656164     esi 0x696e6720  edi  0x74686973\n"
     "ds  0x3f000000     es   0x43494c48     fs  0x43525343  gs   0x4f4d4b53\n"
     "\n"
     "PANIC: k_trap - kernel mode trap type 0x0000000E\n"
     "Trying to dump 5023 pages to dumpdev hd (1/41), 63 pages per '.'\n"
    );
  const char *sco_panic_2 =
   ("...............................................................................\n"
    );
  const char *sco_panic_3 =
    ("5023 pages dumped\n"
     "\n"
     "\n"
     );
  const char *sco_panic_4 =
    ("**   Safe to Power Off   **\n"
     "           - or -\n"
     "** Press Any Key to Reboot **\n"
    );

  if (!get_boolean_resource("doSCO", "DoSCO"))
    return False;

  for (s = sco_panic_1; *s; s++) if (*s == '\n') lines_1++;
  for (s = sco_panic_2; *s; s++) if (*s == '\n') lines_2++;
  for (s = sco_panic_3; *s; s++) if (*s == '\n') lines_3++;
  for (s = sco_panic_4; *s; s++) if (*s == '\n') lines_4++;

  XGetWindowAttributes (dpy, window, &xgwa);

  fontname = get_string_resource ((xgwa.height > 600
				   ? "sco.font2"
				   : "sco.font"),
				  "SCO.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource(("sco.foreground"),
				      "SCO.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource(("sco.background"),
				      "SCO.Background",
				      dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  draw_string(dpy, window, gc, &gcv, font,
	      10, xgwa.height - ((lines_1 + lines_2 + lines_3 + lines_4 + 1) *
                                 (font->ascent + font->descent + 1)),
	      10, 10,
	      sco_panic_1, 0);
  XSync(dpy, False);
  for (s = sco_panic_2; *s; s++)
    {
      char *ss = strdup(sco_panic_2);
      ss[s - sco_panic_2] = 0;
      draw_string(dpy, window, gc, &gcv, font,
                  10, xgwa.height - ((lines_2 + lines_3 + lines_4 + 1) *
                                     (font->ascent + font->descent + 1)),
                  10, 10,
                  ss, 0);
      XSync(dpy, False);
      free(ss);
      if (bsod_sleep (dpy, -1))
        goto DONE;
    }

  draw_string(dpy, window, gc, &gcv, font,
	      10, xgwa.height - ((lines_3 + lines_4 + 1) *
                                 (font->ascent + font->descent + 1)),
	      10, 10,
	      sco_panic_3, 0);
  XSync(dpy, False);
  if (bsod_sleep(dpy, 1))
    goto DONE;
  draw_string(dpy, window, gc, &gcv, font,
	      10, xgwa.height - ((lines_4 + 1) *
                                 (font->ascent + font->descent + 1)),
	      10, 10,
	      sco_panic_4, 0);
  XSync(dpy, False);

  bsod_sleep(dpy, delay);
 DONE:
  XClearWindow(dpy, window);
  XFreeGC(dpy, gc);
  XFreeFont(dpy, font);
  return True;
}


/* Linux (sparc) panic, by Tom Kelly <tom@ancilla.toronto.on.ca>
 */
static Bool
sparc_linux (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  char *fontname;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc;
  int lines = 1;
  const char *s;

  const char *linux_panic =
    ("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
	"Unable to handle kernel paging request at virtual address f0d4a000\n"
	"tsk->mm->context = 00000014\n"
	"tsk->mm->pgd = f26b0000\n"
	"              \\|/ ____ \\|/\n"
	"              \"@'/ ,. \\`@\"\n"
	"              /_| \\__/ |_\\\n"
	"                 \\__U_/\n"
	"gawk(22827): Oops\n"
	"PSR: 044010c1 PC: f001c2cc NPC: f001c2d0 Y: 00000000\n"
	"g0: 00001000 g1: fffffff7 g2: 04401086 g3: 0001eaa0\n"
	"g4: 000207dc g5: f0130400 g6: f0d4a018 g7: 00000001\n"
	"o0: 00000000 o1: f0d4a298 o2: 00000040 o3: f1380718\n"
	"o4: f1380718 o5: 00000200 sp: f1b13f08 ret_pc: f001c2a0\n"
	"l0: efffd880 l1: 00000001 l2: f0d4a230 l3: 00000014\n"
	"l4: 0000ffff l5: f0131550 l6: f012c000 l7: f0130400\n"
	"i0: f1b13fb0 i1: 00000001 i2: 00000002 i3: 0007c000\n"
	"i4: f01457c0 i5: 00000004 i6: f1b13f70 i7: f0015360\n"
	"Instruction DUMP:\n"
    );

  if (!get_boolean_resource("doSparcLinux", "DoSparcLinux"))
    return False;

  for (s = linux_panic; *s; s++) if (*s == '\n') lines++;

  XGetWindowAttributes (dpy, window, &xgwa);

  fontname = get_string_resource ((xgwa.height > 600
				   ? "sparclinux.font2"
				   : "sparclinux.font"),
				  "SparcLinux.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource(("sparclinux.foreground"),
				      "SparcLinux.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource(("sparclinux.background"),
				      "SparcLinux.Background",
				      dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  draw_string(dpy, window, gc, &gcv, font,
	      10, xgwa.height - (lines * (font->ascent + font->descent + 1)),
	      10, 10,
	      linux_panic, 0);
  XFreeGC(dpy, gc);
  XSync(dpy, False);
  bsod_sleep(dpy, delay);
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
  return True;
}

/* BSD Panic by greywolf@starwolf.com - modeled after the Linux panic above.
   By Grey Wolf <greywolf@siteROCK.com>
 */
static Bool
bsd (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  char *fontname;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc;
  int lines = 1;
  int i, n, b;
  const char *rbstr, *panicking;
  char syncing[80], bbuf[5], *bp;

  const char *panicstr[] =
   {"panic: ifree: freeing free inode",
    "panic: blkfree: freeing free block",
    "panic: improbability coefficient below zero",
    "panic: cgsixmmap",
    "panic: crazy interrupts",
    "panic: nmi",
    "panic: attempted windows install",
    "panic: don't",
    "panic: free inode isn't",
    "panic: cpu_fork: curproc",
    "panic: malloc: out of space in kmem_map",
    "panic: vogon starship detected",
    "panic: teleport chamber: out of order",
    "panic: Brain fried - core dumped"};
     
  if (!get_boolean_resource("doBSD", "DoBSD"))
    return False;

  for (i = 0; i < sizeof(syncing); i++)
    syncing[i] = 0;

  i = (random() & 0xffff) % (sizeof(panicstr) / sizeof(*panicstr));

  panicking = panicstr[i];
  strcpy(syncing, "Syncing disks: ");

  b = (random() & 0xff) % 40;
  for (n = 0; (n < 20) && (b > 0); n++)
    {
      if (i)
        {
          i = (random() & 0x7);
          b -= (random() & 0xff) % 20;
          if (b < 0)
            b = 0;
        }
      sprintf (bbuf, "%d ", b);
      strcat (syncing, bbuf);
    }

  if (b)
    rbstr = "damn!";
  else
    rbstr = "sunk!";

  lines = 5;

  XGetWindowAttributes (dpy, window, &xgwa);

  fontname = get_string_resource ((xgwa.height > 600
				   ? "bsd.font2"
				   : "bsd.font"),
				  "BSD.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource(("bsd.foreground"),
				      "BSD.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource(("bsd.background"),
				      "BSD.Background",
				      dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  draw_string(dpy, window, gc, &gcv, font,
	      10, xgwa.height - (lines * (font->ascent + font->descent + 1)),
	      10, 10,
	      panicking, 0);
  XSync(dpy, False);
  lines--;

  for (bp = syncing; *bp;)
    {
      char *bsd_bufs, oc = 0;
      for (;*bp && (*bp != ' '); bp++)
        ;
      if (*bp == ' ')
        {
          oc = *bp;
          *bp = 0;
        }
      bsd_bufs = strdup(syncing);
      draw_string(dpy, window, gc, &gcv, font,
                  10,
                  xgwa.height - (lines * (font->ascent + font->descent + 1)),
                  10, 10,
                  bsd_bufs, 0);
      XSync(dpy, False);
      free(bsd_bufs);
      if (oc)
	*bp = oc;
      if (bsod_sleep(dpy, -1))
        goto DONE;
      bp++;
    }

  lines--;
  
  draw_string(dpy, window, gc, &gcv, font,
	      10, xgwa.height - (lines * (font->ascent + font->descent + 1)),
	      10, 10,
	      rbstr, 0);
  lines--;
  draw_string(dpy, window, gc, &gcv, font,
	      10, xgwa.height - (lines * (font->ascent + font->descent + 1)),
	      10, 10,
	      "Rebooting", 0);

  XFreeGC(dpy, gc);
  XSync(dpy, False);
  bsod_sleep(dpy, delay);

DONE:
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
  return True;
}

static Bool
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
  int pix_w = 0, pix_h = 0;
  int string_width;
  int margin;

  const char *string =
    ("_Software failure.  Press left mouse button to continue.\n"
     "_Guru Meditation #00000003.00C01570");

  if (!get_boolean_resource("doAmiga", "DoAmiga"))
    return False;

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

#if defined(HAVE_GDK_PIXBUF) || defined (HAVE_XPM)
  pixmap = xpm_data_to_pixmap (dpy, window, (char **) amiga_hand,
                               &pix_w, &pix_h, 0);
#endif /* HAVE_GDK_PIXBUF || HAVE_XPM */

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
  margin = font->ascent;
  string_width = draw_string(dpy, window, gc, &gcv, font,
                             margin, 0,
                             xgwa.width - (margin * 2), height,
                             string, 0);
  {
    GC gca = gc;
    while (delay > 0)
      {
        int x2;
	XFillRectangle(dpy, window, gca, 0, 0, xgwa.width, margin);
	XFillRectangle(dpy, window, gca, 0, 0, margin, height);
        XFillRectangle(dpy, window, gca,
                       0, height - margin, xgwa.width, margin);
        x2 = margin + string_width;
        if (x2 < xgwa.width - margin) x2 = xgwa.width - margin;
        XFillRectangle(dpy, window, gca, x2, 0, margin, height);

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
  return True;
}


/* Atari ST, by Marcus Herbert <rhoenie@nobiscum.de>
   Marcus had this to say:

	Though I still have my Atari somewhere, I hardly remember
	the meaning of the bombs. I think 9 bombs was "bus error" or
	something like that.  And you often had a few bombs displayed
	quickly and then the next few ones coming up step by step.
	Perhaps somebody else can tell you more about it..  its just
	a quick hack :-}
 */
static Bool
atari (Display *dpy, Window window, int delay)
{
	
  XGCValues gcv;
  XWindowAttributes xgwa;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc;
  Pixmap pixmap = 0;
  int pix_w = atari_width;
  int pix_h = atari_height;
  int offset;
  int i, x, y;

  if (!get_boolean_resource("doAtari", "DoAtari"))
    return False;

  XGetWindowAttributes (dpy, window, &xgwa);

  font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
                
  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource("atari.foreground", "Atari.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource("atari.background", "Atari.Background",
				      dpy, xgwa.colormap);

  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  pixmap = XCreatePixmapFromBitmapData(dpy, window, (char *) atari_bits,
				       pix_w, pix_h,
				       gcv.foreground, gcv.background,
				       xgwa.depth);
  pixmap = double_pixmap(dpy, gc, xgwa.visual, xgwa.depth,
			 pixmap, pix_w, pix_h);
  pix_w *= 2;
  pix_h *= 2;

  offset = pix_w + 2;
  x = 5;
  y = (xgwa.height - (xgwa.height / 5));
  if (y < 0) y = 0;

  for (i=0 ; i<7 ; i++) {
    XCopyArea(dpy, pixmap, window, gc, 0, 0, pix_w, pix_h,
	      (x + (i*offset)), y);
  }  
  
  for (i=7 ; i<10 ; i++) {
    if (bsod_sleep(dpy, 1))
      goto DONE;
    XCopyArea(dpy, pixmap, window, gc, 0, 0, pix_w, pix_h,
	      (x + (i*offset)), y);
  }

  bsod_sleep(dpy, delay);
 DONE:
  XFreePixmap(dpy, pixmap);
  XFreeGC(dpy, gc);
  XSync(dpy, False);
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
  return True;
}


static Bool
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

  if (!get_boolean_resource("doMac", "DoMac"))
    return False;

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

  draw_string(dpy, window, gc, &gcv, font, 0, 0,
	      xgwa.width, xgwa.height + offset, string, 0);

  XFreeGC(dpy, gc);
  XSync(dpy, False);
  bsod_sleep(dpy, delay);
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
  return True;
}

static Bool
macsbug (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  char *fontname;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc, gc2;

  int char_width, line_height;
  int col_right, row_top, row_bottom, page_right, page_bottom, body_top;
  int xoff, yoff;

  const char *left = ("    SP     \n"
		      " 04EB0A58  \n"
		      "58 00010000\n"
		      "5C 00010000\n"
		      "   ........\n"
		      "60 00000000\n"
		      "64 000004EB\n"
		      "   ........\n"
		      "68 0000027F\n"
		      "6C 2D980035\n"
		      "   ....-..5\n"
		      "70 00000054\n"
		      "74 0173003E\n"
		      "   ...T.s.>\n"
		      "78 04EBDA76\n"
		      "7C 04EBDA8E\n"
		      "   .S.L.a.U\n"
		      "80 00000000\n"
		      "84 000004EB\n"
		      "   ........\n"
		      "88 00010000\n"
		      "8C 00010000\n"
		      "   ...{3..S\n"
		      "\n"
		      "\n"
		      " CurApName \n"
		      "  Finder   \n"
		      "\n"
		      " 32-bit VM \n"
		      "SR Smxnzvc0\n"
		      "D0 04EC0062\n"
		      "D1 00000053\n"
		      "D2 FFFF0100\n"
		      "D3 00010000\n"
		      "D4 00010000\n"
		      "D5 04EBDA76\n"
		      "D6 04EBDA8E\n"
		      "D7 00000001\n"
		      "\n"
		      "A0 04EBDA76\n"
		      "A1 04EBDA8E\n"
		      "A2 A0A00060\n"
		      "A3 027F2D98\n"
		      "A4 027F2E58\n"
		      "A5 04EC04F0\n"
		      "A6 04EB0A86\n"
		      "A7 04EB0A58");
  const char *bottom = ("  _A09D\n"
			"     +00884    40843714     #$0700,SR         "
			"                  ; A973        | A973\n"
			"     +00886    40843765     *+$0400           "
			"                                | 4A1F\n"
			"     +00888    40843718     $0004(A7),([0,A7[)"
			"                  ; 04E8D0AE    | 66B8");

#if 0
  const char *body = ("Bus Error at 4BF6D6CC\n"
		      "while reading word from 4BF6D6CC in User data space\n"
		      " Unable to access that address\n"
		      "  PC: 2A0DE3E6\n"
		      "  Frame Type: B008");
#else
  const char * body = ("PowerPC unmapped memory exception at 003AFDAC "
						"BowelsOfTheMemoryMgr+04F9C\n"
		      " Calling chain using A6/R1 links\n"
		      "  Back chain  ISA  Caller\n"
		      "  00000000    PPC  28C5353C  __start+00054\n"
		      "  24DB03C0    PPC  28B9258C  main+0039C\n"
		      "  24DB0350    PPC  28B9210C  MainEvent+00494\n"
		      "  24DB02B0    PPC  28B91B40  HandleEvent+00278\n"
		      "  24DB0250    PPC  28B83DAC  DoAppleEvent+00020\n"
		      "  24DB0210    PPC  FFD3E5D0  "
						"AEProcessAppleEvent+00020\n"
		      "  24DB0132    68K  00589468\n"
		      "  24DAFF8C    68K  00589582\n"
		      "  24DAFF26    68K  00588F70\n"
		      "  24DAFEB3    PPC  00307098  "
						"EmToNatEndMoveParams+00014\n"
		      "  24DAFE40    PPC  28B9D0B0  DoScript+001C4\n"
		      "  24DAFDD0    PPC  28B9C35C  RunScript+00390\n"
		      "  24DAFC60    PPC  28BA36D4  run_perl+000E0\n"
		      "  24DAFC10    PPC  28BC2904  perl_run+002CC\n"
		      "  24DAFA80    PPC  28C18490  Perl_runops+00068\n"
		      "  24DAFA30    PPC  28BE6CC0  Perl_pp_backtick+000FC\n"
		      "  24DAF9D0    PPC  28BA48B8  Perl_my_popen+00158\n"
		      "  24DAF980    PPC  28C5395C  sfclose+00378\n"
		      "  24DAF930    PPC  28BA568C  free+0000C\n"
		      "  24DAF8F0    PPC  28BA6254  pool_free+001D0\n"
		      "  24DAF8A0    PPC  FFD48F14  DisposePtr+00028\n"
		      "  24DAF7C9    PPC  00307098  "
						"EmToNatEndMoveParams+00014\n"
		      "  24DAF780    PPC  003AA180  __DisposePtr+00010");
#endif

  const char *s;
  int body_lines = 1;

  if (!get_boolean_resource("doMacsBug", "DoMacsBug"))
    return False;

  for (s = body; *s; s++) if (*s == '\n') body_lines++;

  XGetWindowAttributes (dpy, window, &xgwa);

  fontname = get_string_resource ((xgwa.height > 850
				   ? "macsbug.font3"
				   : (xgwa.height > 700
				      ? "macsbug.font2"
				      : "macsbug.font")),
				  "MacsBug.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource("macsbug.foreground",
				      "MacsBug.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource("macsbug.background",
				      "MacsBug.Background",
				      dpy, xgwa.colormap);

  gc = XCreateGC(dpy, window, GCFont|GCForeground|GCBackground, &gcv);

  gcv.foreground = gcv.background;
  gc2 = XCreateGC(dpy, window, GCForeground, &gcv);

  XSetWindowBackground(dpy, window,
		       get_pixel_resource("macsbug.borderColor",
					  "MacsBug.BorderColor",
					  dpy, xgwa.colormap));
  XClearWindow(dpy, window);

  char_width = (font->per_char
		? font->per_char['n'-font->min_char_or_byte2].width
		: font->min_bounds.width);
  line_height = font->ascent + font->descent + 1;

  col_right = char_width * 12;
  page_bottom = line_height * 47;

  if (page_bottom > xgwa.height) page_bottom = xgwa.height;

  row_bottom = page_bottom - line_height;
  row_top = row_bottom - (line_height * 4);
  page_right = col_right + (char_width * 88);
  body_top = row_top - (line_height * body_lines);

  page_bottom += 2;
  row_bottom += 2;
  body_top -= 4;

  xoff = (xgwa.width - page_right) / 2;
  yoff = (xgwa.height - page_bottom) / 2;
  if (xoff < 0) xoff = 0;
  if (yoff < 0) yoff = 0;

  XFillRectangle(dpy, window, gc2, xoff, yoff, page_right, page_bottom);

  draw_string(dpy, window, gc, &gcv, font, xoff, yoff, 10, 10, left, 0);
  draw_string(dpy, window, gc, &gcv, font, xoff+col_right, yoff+row_top,
	      10, 10, bottom, 0);

  XFillRectangle(dpy, window, gc, xoff + col_right, yoff, 2, page_bottom);
  XDrawLine(dpy, window, gc,
	    xoff+col_right, yoff+row_top, xoff+page_right, yoff+row_top);
  XDrawLine(dpy, window, gc,
	    xoff+col_right, yoff+row_bottom, xoff+page_right, yoff+row_bottom);
  XDrawRectangle(dpy, window, gc,  xoff, yoff, page_right, page_bottom);

  if (body_top > 4)
    body_top = 4;

  draw_string(dpy, window, gc, &gcv, font,
	      xoff + col_right + char_width, yoff + body_top, 10, 10, body,
	      500);

  while (delay > 0)
    {
      XDrawLine(dpy, window, gc,
		xoff+col_right+(char_width/2)+2, yoff+row_bottom+3,
		xoff+col_right+(char_width/2)+2, yoff+page_bottom-3);
      XSync(dpy, False);
      usleep(666666L);
      XDrawLine(dpy, window, gc2,
		xoff+col_right+(char_width/2)+2, yoff+row_bottom+3,
		xoff+col_right+(char_width/2)+2, yoff+page_bottom-3);
      XSync(dpy, False);
      usleep(333333L);
      if (bsod_sleep(dpy, 0))
	break;
      delay--;
    }

  XFreeGC(dpy, gc);
  XFreeGC(dpy, gc2);
  XClearWindow(dpy, window);
  XFreeFont(dpy, font);
  return True;
}

static Bool
mac1 (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  GC gc;
  Pixmap pixmap = 0;
  int pix_w = macbomb_width;
  int pix_h = macbomb_height;

  if (!get_boolean_resource("doMac1", "DoMac1"))
    return False;

  XGetWindowAttributes (dpy, window, &xgwa);

  gcv.foreground = get_pixel_resource("mac1.foreground", "Mac.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource("mac1.background", "Mac.Background",
				      dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  gc = XCreateGC(dpy, window, GCForeground|GCBackground, &gcv);

  pixmap = XCreatePixmapFromBitmapData(dpy, window, (char *) macbomb_bits,
				       macbomb_width, macbomb_height,
				       gcv.foreground,
				       gcv.background,
				       xgwa.depth);

  {
    int x = (xgwa.width - pix_w) / 2;
    int y = (xgwa.height - pix_h) / 2;
    if (y < 0) y = 0;
    XFillRectangle (dpy, window, gc, 0, 0, xgwa.width, xgwa.height);
    XSync(dpy, False);
    if (bsod_sleep(dpy, 1))
      goto DONE;
    XCopyArea(dpy, pixmap, window, gc, 0, 0, pix_w, pix_h, x, y);
  }

 DONE:
  XFreeGC(dpy, gc);
  XFreePixmap(dpy, pixmap);
  XSync(dpy, False);
  bsod_sleep(dpy, delay);
  XClearWindow(dpy, window);
  return True;
}


static Bool
macx (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xgwa;
  char *fontname = 0;
  const char *def_font = "fixed";
  XFontStruct *font;
  GC gc;

  const char *macx_panic =
   ("panic(cpu 0): Unable to find driver for this platform: "
    "\"PowerMac 3,5\".\n"
    "\n"
    "backtrace: 0x0008c2f4 0x0002a7a0 0x001f0204 0x001d4e4c 0x001d4c5c "
    "0x001a56cc 0x01d5dbc 0x001c621c 0x00037430 0x00037364\n"
    "\n"
    "\n"
    "\n"
    "No debugger configured - dumping debug information\n"
    "\n"
    "version string : Darwin Kernel Version 1.3:\n"
    "Thu Mar  1 06:56:40 PST 2001; root:xnu/xnu-123.5.obj~1/RELEASE_PPC\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "DBAT0: 00000000 00000000\n"
    "DBAT1: 00000000 00000000\n"
    "DBAT2: 80001FFE 8000003A\n"
    "DBAT3: 90001FFE 9000003A\n"
    "MSR=00001030\n"
    "backtrace: 0x0008c2f4 0x0002a7a0 0x001f0204 0x001d4e4c 0x001d4c5c "
    "0x001a56cc 0x01d5dbc 0x001c621c 0x00037430 0x00037364\n"
    "\n"
    "panic: We are hanging here...\n");

  if (!get_boolean_resource("doMacX", "DoMacX"))
    return False;

  XGetWindowAttributes (dpy, window, &xgwa);

  gcv.background = get_pixel_resource("macX.background",
                                      "MacX.Background",
				      dpy, xgwa.colormap);
  XSetWindowBackground(dpy, window, gcv.background);
  XClearWindow(dpy, window);

  fontname = get_string_resource ((xgwa.height > 900
				   ? "macX.font2"
				   : "macX.font"),
				  "MacX.Font");
  if (!fontname || !*fontname) fontname = (char *)def_font;
  font = XLoadQueryFont (dpy, fontname);
  if (!font) font = XLoadQueryFont (dpy, def_font);
  if (!font) exit(-1);
  if (fontname && fontname != def_font)
    free (fontname);

  gcv.font = font->fid;
  gcv.foreground = get_pixel_resource("macsbug.foreground",
				      "MacsBug.Foreground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource("macsbug.background",
				      "MacsBug.Background",
				      dpy, xgwa.colormap);


  gcv.foreground = get_pixel_resource("macX.textForeground",
                                      "MacX.TextForeground",
				      dpy, xgwa.colormap);
  gcv.background = get_pixel_resource("macX.textBackground",
                                      "MacX.TextBackground",
				      dpy, xgwa.colormap);
  gc = XCreateGC(dpy, window, GCForeground|GCBackground|GCFont, &gcv);

#if defined(HAVE_GDK_PIXBUF) || defined (HAVE_XPM)
  {
    Pixmap pixmap = 0;
    Pixmap mask = 0;
    int x, y, pix_w, pix_h;
    pixmap = xpm_data_to_pixmap (dpy, window, (char **) happy_mac,
                                 &pix_w, &pix_h, &mask);

    x = (xgwa.width - pix_w) / 2;
    y = (xgwa.height - pix_h) / 2;
    if (y < 0) y = 0;
    XSync(dpy, False);
    bsod_sleep(dpy, 2);
    XSetClipMask (dpy, gc, mask);
    XSetClipOrigin (dpy, gc, x, y);
    XCopyArea (dpy, pixmap, window, gc, 0, 0, pix_w, pix_h, x, y);
    XSetClipMask (dpy, gc, None);
    XFreePixmap (dpy, pixmap);
  }
#endif /* HAVE_GDK_PIXBUF || HAVE_XPM */

  bsod_sleep(dpy, 3);

  {
    const char *s;
    int x = 0, y = 0;
    int char_width, line_height;
    char_width = (font->per_char
                  ? font->per_char['n'-font->min_char_or_byte2].width
                  : font->min_bounds.width);
    line_height = font->ascent + font->descent;

    s = macx_panic;
    y = font->ascent;
    while (*s)
      {
        int ox = x;
        int oy = y;
        if (*s == '\n' || x + char_width >= xgwa.width)
          {
            x = 0;
            y += line_height;
          }

        if (*s == '\n')
          {
            /* Note that to get this goofy effect, they must be actually
               emitting LF CR at the end of each line instead of CR LF!
             */
            XDrawImageString (dpy, window, gc, ox, oy, " ", 1);
            XDrawImageString (dpy, window, gc, ox, y, " ", 1);
          }
        else
          {
            XDrawImageString (dpy, window, gc, x, y, s, 1);
            x += char_width;
          }
        s++;
      }
  }

  XFreeGC(dpy, gc);
  XSync(dpy, False);
  bsod_sleep(dpy, delay);
  XClearWindow(dpy, window);
  return True;
}





/* blit damage
 *
 * by Martin Pool <mbp@samba.org>, Feb 2000.
 *
 * This is meant to look like the preferred failure mode of NCD
 * Xterms.  The parameters for choosing what to copy where might not
 * be quite right, but it looks about ugly enough.
 */
static Bool
blitdamage (Display *dpy, Window window, int delay)
{
  XGCValues gcv;
  XWindowAttributes xwa;
  GC gc0;
  int i;
  int delta_x = 0, delta_y = 0;
  int w, h;
  int chunk_h, chunk_w;
  int steps;
  long gc_mask = 0;
  int src_x, src_y;
  int x, y;
  
  if (!get_boolean_resource("doBlitDamage", "DoBlitDamage"))
    return False;

  XGetWindowAttributes(dpy, window, &xwa);

  load_random_image (xwa.screen, window, window);

  w = xwa.width;
  h = xwa.height;

  gc_mask = GCForeground;
  
  gcv.plane_mask = random();
  gc_mask |= GCPlaneMask;
  
  gc0 = XCreateGC(dpy, window, gc_mask, &gcv);

  steps = 50;
  chunk_w = w / (random() % 1 + 1);
  chunk_h = h / (random() % 1 + 1);
  if (random() & 0x1000) 
    delta_y = random() % 600;
  if (!delta_y || (random() & 0x2000))
    delta_x = random() % 600;
  src_x = 0; 
  src_y = 0; 
  x = 0;
  y = 0;
  
  for (i = 0; i < steps; i++) {
    if (x + chunk_w > w) 
      x -= w;
    else
      x += delta_x;
    
    if (y + chunk_h > h)
      y -= h;
    else
      y += delta_y;
    
    XCopyArea(dpy, window, window, gc0,
	      src_x, src_y, 
	      chunk_w, chunk_h,
	      x, y);

    if (bsod_sleep(dpy, 0))
      goto DONE;
  }

  bsod_sleep(dpy, delay);

 DONE:
  XFreeGC(dpy, gc0);

  return True;
}


/*
 * SPARC Solaris panic. Should look pretty authentic on Solaris boxes.
 * Anton Solovyev <solovam@earthlink.net>
 */ 

typedef struct
{
  Display *dpy;
  Window window;
  GC gc, erase_gc;
  XFontStruct *xfs;
  int sub_width;                /* Text subwindow width in pixels */
  int sub_height;               /* Text subwindow height in pixels */
  int sub_x;                    /* upper left corner of the text subwindow */
  int sub_y;                    /* upper left corner of the text subwindow */
  int char_width;               /* Char width in pixels */
  int line_height;              /* Line height in pixels */
  int columns;                  /* Number of columns in the text screen */
  int x;                        /* horizontal position of the cursor */
} scrolling_window;


static scrolling_window *
make_scrolling_window (Display *dpy, Window window,
                       const char *name,
                       Bool grab_screen_p)
{
  const char *def_font = "fixed";
  scrolling_window* ts;
  XWindowAttributes xgwa;
  XGCValues gcv;
  char* fn;
  char buf1[100], buf2[100];

  ts = malloc (sizeof (*ts));
  ts->window = window;
  ts->dpy = dpy;

  ts->x = 0;

  XGetWindowAttributes (dpy, window, &xgwa);

  if (grab_screen_p)
    {
      ts->sub_width = xgwa.width * 0.8;
      ts->sub_height = xgwa.height * 0.8;
    }
  else
    {
      ts->sub_width  = xgwa.width - 20;
      ts->sub_height = xgwa.height - 20;
      if (ts->sub_width  < 20) ts->sub_width  = 20;
      if (ts->sub_height < 20) ts->sub_height = 20;
    }

  sprintf (buf1, "%.50s.font", name);
  sprintf (buf2, "%.50s.Font", name);
  fn = get_string_resource (buf1, buf2);
  ts->xfs = XLoadQueryFont (dpy, fn);
  if (!ts->xfs)
    {
      sprintf (buf1, "%.50s.font2", name);
      fn = get_string_resource(buf1, buf2);
      ts->xfs = XLoadQueryFont(dpy, fn);
    }
  if (!ts->xfs)
    ts->xfs = XLoadQueryFont(dpy, def_font);
  if (!ts->xfs)
    exit (1);
  gcv.font = ts->xfs->fid;
  ts->char_width = (ts->xfs->per_char
                    ? ts->xfs->per_char['n'-ts->xfs->min_char_or_byte2].width
                    : ts->xfs->min_bounds.width);
  ts->line_height = ts->xfs->ascent + ts->xfs->descent + 1;

  ts->columns = ts->sub_width / ts->char_width;

  ts->sub_x = (xgwa.width - ts->sub_width) / 2;
  ts->sub_y = (xgwa.height - ts->sub_height) / 2;

  if (!grab_screen_p) ts->sub_height += ts->sub_y, ts->sub_y = 0;

  if (grab_screen_p)
    load_random_image (xgwa.screen, window, window);

  sprintf (buf1, "%.50s.background", name);
  sprintf (buf2, "%.50s.Background", name);
  gcv.background = get_pixel_resource (buf1, buf2, dpy, xgwa.colormap);

  sprintf (buf1, "%.50s.foreground", name);
  sprintf (buf2, "%.50s.Foreground", name);
  gcv.foreground = get_pixel_resource (buf1, buf2, dpy, xgwa.colormap);

  ts->gc = XCreateGC (dpy, window,
                      GCForeground | GCBackground | GCFont,
                      &gcv);
  gcv.foreground = gcv.background;
  ts->erase_gc = XCreateGC (dpy, window,
                            GCForeground | GCBackground,
                            &gcv);
  XSetWindowBackground (dpy, window, gcv.background);
  return(ts);
}

static void
free_scrolling_window (scrolling_window* ts)
{
  XFreeGC (ts->dpy, ts->gc);
  XFreeGC (ts->dpy, ts->erase_gc);
  XFreeFont (ts->dpy, ts->xfs);
  free (ts);
}

static void
scrolling_putc (scrolling_window* ts, const char aChar)
{
  switch (aChar)
    {
    case '\n':
      ts->x = 0;
      XCopyArea (ts->dpy, ts->window, ts->window, ts->gc,
                 ts->sub_x, ts->sub_y + ts->line_height,
                 ts->sub_width, ts->sub_height,
                 ts->sub_x, ts->sub_y);
      XFillRectangle (ts->dpy, ts->window, ts->erase_gc,
                      ts->sub_x, ts->sub_y + ts->sub_height - ts->line_height,
                      ts->sub_width, ts->line_height);
      break;
    case '\b':
      if(ts->x > 0)
        ts->x--;
      break;
    default:
      if (ts->x >= ts->columns)
        scrolling_putc (ts, '\n');
      XDrawImageString (ts->dpy, ts->window, ts->gc,
                        (ts->sub_x +
                         (ts->x * ts->char_width)
                         - ts->xfs->min_bounds.lbearing),
                        (ts->sub_y + ts->sub_height - ts->xfs->descent),
                        &aChar, 1);
      ts->x++;
      break;
    }
}

static Bool
scrolling_puts (scrolling_window *ts, const char* aString, int delay)
{
  const char *c;
  for (c = aString; *c; ++c)
    {
      scrolling_putc (ts, *c);
      if (delay)
        {
          XSync(ts->dpy, 0);
          usleep(delay);
          if (bsod_sleep (ts->dpy, 0))
            return True;
        }
    }
  XSync (ts->dpy, 0);
  return False;
}

static Bool
sparc_solaris (Display* dpy, Window window, int delay)
{
  const char *msg1 =
    "BAD TRAP: cpu=0 type=0x31 rp=0x2a10043b5e0 addr=0xf3880 mmu_fsr=0x0\n"
    "BAD TRAP occured in module \"unix\" due to an illegal access to a"
    " user address.\n"
    "adb: trap type = 0x31\n"
    "addr=0xf3880\n"
    "pid=307, pc=0x100306e4, sp=0x2a10043ae81, tstate=0x4480001602,"
    " context=0x87f\n"
    "g1-g7: 1045b000, 32f, 10079440, 180, 300000ebde8, 0, 30000953a20\n"
    "Begin traceback... sp = 2a10043ae81\n"
    "Called from 100bd060, fp=2a10043af31, args=f3700 300008cc988 f3880 0"
    " 1 300000ebde0.\n"
    "Called from 101fe1bc, fp=2a10043b011, args=3000045a240 104465a0"
    " 300008e47d0 300008e48fa 300008ae350 300008ae410\n"
    "Called from 1007c520, fp=2a10043b0c1, args=300008e4878 300003596e8 0"
    " 3000045a320 0 3000045a220\n"
    "Called from 1007c498, fp=2a10043b171, args=1045a000 300007847f0 20"
    " 3000045a240 1 0\n"
    "Called from 1007972c, fp=2a10043b221, args=1 300009517c0 30000951e58 1"
    " 300007847f0 0\n"
    "Called from 10031e10, fp=2a10043b2d1, args=3000095b0c8 0 300009396a8"
    " 30000953a20 0 1\n"
    "Called from 10000bdd8, fp=ffffffff7ffff1c1, args=0 57 100131480"
    " 100131480 10012a6e0 0\n"
    "End traceback...\n"
    "panic[cpu0]/thread=30000953a20: trap\n"
    "syncing file systems...";
  const char *msg2 =
    " 1 done\n"
    "dumping to /dev/dsk/c0t0d0s3, offset 26935296\n";
  const char *msg3 =
    ": 2803 pages dumped, compression ratio 2.88, dump succeeded\n";
  const char *msg4 =
    "rebooting...\n"
    "Resetting ...";

  scrolling_window *ts;
  int i;
  char buf[256];

  if (!get_boolean_resource("doSolaris", "DoSolaris"))
    return False;

  ts = make_scrolling_window (dpy, window, "Solaris", True);

  scrolling_puts (ts, msg1, 0);
  if (bsod_sleep (dpy, 3))
    goto DONE;

  scrolling_puts (ts, msg2, 0);
  if (bsod_sleep (dpy, 2))
    goto DONE;

  for (i = 1; i <= 100; ++i)
    {
      sprintf(buf, "\b\b\b\b\b\b\b\b\b\b\b%3d%% done", i);
      scrolling_puts (ts, buf, 0);
      if (bsod_sleep (dpy, -1))
        goto DONE;
    }

  scrolling_puts (ts, msg3, 0);
  if (bsod_sleep (dpy, 2))
    goto DONE;

  scrolling_puts (ts, msg4, 0);
  if (bsod_sleep(dpy, 3))
    goto DONE;

  bsod_sleep (dpy, 3);

 DONE:
  free_scrolling_window (ts);

  return True;
}

/* Linux panic and fsck, by jwz
 */
static Bool
linux_fsck (Display *dpy, Window window, int delay)
{
  XWindowAttributes xgwa;
  scrolling_window *ts;
  int i;
  const char *sysname;
  char buf[1024];

  const char *linux_panic[] = {
   " kernel: Unable to handle kernel paging request at virtual "
     "address 0000f0ad\n",
   " kernel:  printing eip:\n",
   " kernel: c01becd7\n",
   " kernel: *pde = 00000000\n",
   " kernel: Oops: 0000\n",
   " kernel: CPU:    0\n",
   " kernel: EIP:    0010:[<c01becd7>]    Tainted: P \n",
   " kernel: EFLAGS: 00010286\n",
   " kernel: eax: 0000ff00   ebx: ca6b7e00   ecx: ce1d7a60   edx: ce1d7a60\n",
   " kernel: esi: ca6b7ebc   edi: 00030000   ebp: d3655ca0   esp: ca6b7e5c\n",
   " kernel: ds: 0018   es: 0018   ss: 0018\n",
   " kernel: Process crond (pid: 1189, stackpage=ca6b7000)\n",
   " kernel: Stack: d3655ca0 ca6b7ebc 00030054 ca6b7e7c c01c1e5b "
       "00000287 00000020 c01c1fbf \n",
   "",
   " kernel:        00005a36 000000dc 000001f4 00000000 00000000 "
       "ce046d40 00000001 00000000 \n",
   "", "", "",
   " kernel:        ffffffff d3655ca0 d3655b80 00030054 c01bef93 "
       "d3655ca0 ca6b7ebc 00030054 \n",
   "", "", "",
   " kernel: Call Trace:    [<c01c1e5b>] [<c01c1fbf>] [<c01bef93>] "
       "[<c01bf02b>] [<c0134c4f>]\n",
   "", "", "",
   " kernel:   [<c0142562>] [<c0114f8c>] [<c0134de3>] [<c010891b>]\n",
   " kernel: \n",
   " kernel: Code: 2a 00 75 08 8b 44 24 2c 85 c0 74 0c 8b 44 24 58 83 48 18 "
      "08 \n",
   0
  };

  if (!get_boolean_resource("doLinux", "DoLinux"))
    return False;

  XGetWindowAttributes (dpy, window, &xgwa);
  XSetWindowBackground (dpy, window, 
                        get_pixel_resource("Linux.background",
                                           "Linux.Background",
                                           dpy, xgwa.colormap));
  XClearWindow(dpy, window);

  sysname = "linux";
# ifdef HAVE_UNAME
  {
    struct utsname uts;
    char *s;
    if (uname (&uts) >= 0)
      sysname = uts.nodename;
    s = strchr (sysname, '.');
    if (s) *s = 0;
  }
# endif	/* !HAVE_UNAME */


  ts = make_scrolling_window (dpy, window, "Linux", False);

  scrolling_puts (ts, "waiting for X server to shut down ", 0);
  usleep (100000);
  if (bsod_sleep (dpy, 0))
    goto PANIC;
  scrolling_puts (ts,
                  "XIO:  fatal IO error 2 (broken pipe) on X server \":0.0\"\n"
                  "        after 339471 requests (339471 known processed) "
                  "with 0 events remaining\n",
                  0);
  if (scrolling_puts (ts, ".........\n", 300000))
    goto PANIC;
  if (bsod_sleep (dpy, 0))
    goto PANIC;
  scrolling_puts (ts,
                  "xinit:  X server slow to shut down, sending KILL signal.\n",
                  0);
  scrolling_puts (ts, "waiting for server to die ", 0);
  if (scrolling_puts (ts, "...\n", 300000))
    goto PANIC;
  if (bsod_sleep (dpy, 0))
    goto PANIC;
  scrolling_puts (ts, "xinit:  Can't kill server\n", 0);

  if (bsod_sleep (dpy, 2))
    goto PANIC;

  sprintf (buf, "\n%s Login: ", sysname);
  scrolling_puts (ts, buf, 0);
  if (bsod_sleep (dpy, 1))
    goto PANIC;
  scrolling_puts (ts,
    "\n\n"
    "Parallelizing fsck version 1.22 (22-Jun-2001)\n"
    "e2fsck 1.22, 22-Jun-2001 for EXT2 FS 0.5b, 95/08/09\n"
    "Warning!  /dev/hda1 is mounted.\n"
    "/dev/hda1 contains a file system with errors, check forced.\n",
                0);
  if (bsod_sleep (dpy, 1))
    goto PANIC;

  if (0 == random() % 2)
  scrolling_puts (ts,
     "Couldn't find ext2 superblock, trying backup blocks...\n"
     "The filesystem size (according to the superblock) is 3644739 blocks\n"
     "The physical size of the device is 3636706 blocks\n"
     "Either the superblock or the partition table is likely to be corrupt!\n"
     "Abort<y>? no\n",
                0);
  if (bsod_sleep (dpy, 1))
    goto PANIC;

 AGAIN:

  scrolling_puts (ts, "Pass 1: Checking inodes, blocks, and sizes\n", 0);
  if (bsod_sleep (dpy, 2))
    goto PANIC;

  i = (random() % 60) - 20;
  while (--i > 0)
    {
      int b = random() % 0xFFFF;
      sprintf (buf, "Deleted inode %d has zero dtime.  Fix<y>? yes\n\n", b);
      scrolling_puts (ts, buf, 0);
    }

  i = (random() % 40) - 10;
  if (i > 0)
    {
      int g = random() % 0xFFFF;
      int b = random() % 0xFFFFFFF;

      if (bsod_sleep (dpy, 1))
        goto PANIC;

      sprintf (buf, "Warning: Group %d's copy of the group descriptors "
               "has a bad block (%d).\n", g, b);
      scrolling_puts (ts, buf, 0);

      b = random() % 0x3FFFFF;
      while (--i > 0)
        {
          b += random() % 0xFFFF;
          sprintf (buf,
                   "Error reading block %d (Attempt to read block "
                   "from filesystem resulted in short read) while doing "
                   "inode scan.  Ignore error<y>?",
                   b);
          scrolling_puts (ts, buf, 0);
          usleep (10000);
          scrolling_puts (ts, " yes\n\n", 0);
        }
    }

  if (0 == (random() % 10))
    {

      if (bsod_sleep (dpy, 1))
        goto PANIC;

      i = 3 + (random() % 10);
      while (--i > 0)
        scrolling_puts (ts, "Could not allocate 256 block(s) for inode table: "
                        "No space left on device\n", 0);
      scrolling_puts (ts, "Restarting e2fsck from the beginning...\n", 0);

      if (bsod_sleep (dpy, 2))
        goto PANIC;

      goto AGAIN;
    }

  i = (random() % 20) - 5;

  if (i > 0)
    if (bsod_sleep (dpy, 1))
      goto PANIC;

  while (--i > 0)
    {
      int j = 5 + (random() % 10);
      int w = random() % 4;

      while (--j > 0)
        {
          int b = random() % 0xFFFFF;
          int g = random() % 0xFFF;

          if (0 == (random() % 10))
            b = 0;
          else if (0 == (random() % 10))
            b = -1;

          if (w == 0)
            sprintf (buf,
                     "Inode table for group %d not in group.  (block %d)\n"
                     "WARNING: SEVERE DATA LOSS POSSIBLE.\n"
                     "Relocate<y>?",
                     g, b);
          else if (w == 1)
            sprintf (buf,
                     "Block bitmap for group %d not in group.  (block %d)\n"
                     "Relocate<y>?",
                     g, b);
          else if (w == 2)
            sprintf (buf,
                     "Inode bitmap %d for group %d not in group.\n"
                     "Continue<y>?",
                     b, g);
          else /* if (w == 3) */
            sprintf (buf,
                     "Bad block %d in group %d's inode table.\n"
                     "WARNING: SEVERE DATA LOSS POSSIBLE.\n"
                     "Relocate<y>?",
                     b, g);

          scrolling_puts (ts, buf, 0);
          scrolling_puts (ts, " yes\n\n", 0);
        }
      if (bsod_sleep (dpy, 0))
        goto PANIC;
      usleep (1000);
    }


  if (0 == random() % 10) goto PANIC;
  scrolling_puts (ts, "Pass 2: Checking directory structure\n", 0);
  if (bsod_sleep (dpy, 2))
    goto PANIC;

  i = (random() % 20) - 5;
  while (--i > 0)
    {
      int n = random() % 0xFFFFF;
      int o = random() % 0xFFF;
      sprintf (buf, "Directory inode %d, block 0, offset %d: "
               "directory corrupted\n"
               "Salvage<y>? ",
               n, o);
      scrolling_puts (ts, buf, 0);
      usleep (1000);
      scrolling_puts (ts, " yes\n\n", 0);

      if (0 == (random() % 100))
        {
          sprintf (buf, "Missing '.' in directory inode %d.\nFix<y>?", n);
          scrolling_puts (ts, buf, 0);
          usleep (1000);
          scrolling_puts (ts, " yes\n\n", 0);
        }

      if (bsod_sleep (dpy, 0))
        goto PANIC;
    }

  if (0 == random() % 10) goto PANIC;
  scrolling_puts (ts,
                  "Pass 3: Checking directory connectivity\n"
                  "/lost+found not found.  Create? yes\n",
                0);
  if (bsod_sleep (dpy, 2))
    goto PANIC;

  /* Unconnected directory inode 4949 (/var/spool/squid/06/???)
     Connect to /lost+found<y>? yes

     '..' in /var/spool/squid/06/08 (20351) is <The NULL inode> (0), should be 
     /var/spool/squid/06 (20350).
     Fix<y>? yes

     Unconnected directory inode 128337 (/var/spool/squid/06/???)
     Connect to /lost+found<y>? yes
   */


  if (0 == random() % 10) goto PANIC;
  scrolling_puts (ts, "Pass 4: Checking reference counts\n", 0);
  if (bsod_sleep (dpy, 2))
    goto PANIC;

  /* Inode 2 ref count is 19, should be 20.  Fix<y>? yes

     Inode 4949 ref count is 3, should be 2.  Fix<y>? yes

        ...

     Inode 128336 ref count is 3, should be 2.  Fix<y>? yes

     Inode 128337 ref count is 3, should be 2.  Fix<y>? yes

   */


  if (0 == random() % 10) goto PANIC;
  scrolling_puts (ts, "Pass 5: Checking group summary information\n", 0);
  if (bsod_sleep (dpy, 2))
    goto PANIC;

  i = (random() % 200) - 50;
  if (i > 0)
    {
      scrolling_puts (ts, "Block bitmap differences: ", 0);
      while (--i > 0)
        {
          sprintf (buf, " %d", -(random() % 0xFFF));
          scrolling_puts (ts, buf, 0);
          usleep (1000);
        }
      scrolling_puts (ts, "\nFix? yes\n\n", 0);
    }


  i = (random() % 100) - 50;
  if (i > 0)
    {
      scrolling_puts (ts, "Inode bitmap differences: ", 0);
      while (--i > 0)
        {
          sprintf (buf, " %d", -(random() % 0xFFF));
          scrolling_puts (ts, buf, 0);
          usleep (1000);
        }
      scrolling_puts (ts, "\nFix? yes\n\n", 0);
    }

  i = (random() % 20) - 5;
  while (--i > 0)
    {
      int g = random() % 0xFFFF;
      int c = random() % 0xFFFF;
      sprintf (buf,
               "Free blocks count wrong for group #0 (%d, counted=%d).\nFix? ",
               g, c);
      scrolling_puts (ts, buf, 0);
      usleep (1000);
      scrolling_puts (ts, " yes\n\n", 0);
      if (bsod_sleep (dpy, 0))
        goto PANIC;
    }

 PANIC:

  i = 0;
  scrolling_puts (ts, "\n\n", 0);
  while (linux_panic[i])
    {
      time_t t = time ((time_t *) 0);
      struct tm *tm = localtime (&t);
      char prefix[100];

      if (*linux_panic[i])
        {
          strftime (prefix, sizeof(prefix)-1, "%b %d %k:%M:%S ", tm);
          scrolling_puts (ts, prefix, 0);
          scrolling_puts (ts, sysname, 0);
          scrolling_puts (ts, linux_panic[i], 0);
          XSync(dpy, False);
          usleep(1000);
        }
      else
        usleep (300000);

      if (bsod_sleep (dpy, 0))
        goto DONE;
      i++;
    }

  if (bsod_sleep (dpy, 4))
    goto DONE;


  XSync(dpy, False);
  bsod_sleep(dpy, delay);

 DONE:
  free_scrolling_window (ts);
  XClearWindow(dpy, window);
  return True;
}



char *progclass = "BSOD";

char *defaults [] = {
  "*delay:		   30",

  "*doWindows:		   True",
  "*doNT:		   True",
  "*doWin2K:		   True",
  "*doAmiga:		   True",
  "*doMac:		   True",
  "*doMacsBug:		   True",
  "*doMac1:		   True",
  "*doMacX:		   True",
  "*doSCO:		   True",
  "*doAtari:		   False",	/* boring */
  "*doBSD:		   False",	/* boring */
  "*doLinux:		   True",
  "*doSparcLinux:	   False",	/* boring */
  "*doBlitDamage:          True",
  "*doSolaris:             True",

  ".Windows.font:	   -*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  ".Windows.font2:	   -*-courier-bold-r-*-*-*-180-*-*-m-*-*-*",
  ".Windows.foreground:	   White",
  ".Windows.background:	   #0000AA",    /* EGA color 0x01. */

  ".Amiga.font:		   -*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  ".Amiga.font2:	   -*-courier-bold-r-*-*-*-180-*-*-m-*-*-*",
  ".Amiga.foreground:	   Red",
  ".Amiga.background:	   Black",
  ".Amiga.background2:	   White",

  ".Mac.font:		   -*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  ".Mac.foreground:	   PaleTurquoise1",
  ".Mac.background:	   Black",

  ".Atari.foreground:	   Black",
  ".Atari.background:	   White",

  ".MacsBug.font:	   -*-courier-medium-r-*-*-*-100-*-*-m-*-*-*",
  ".MacsBug.font2:	   -*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  ".MacsBug.font3:	   -*-courier-bold-r-*-*-*-140-*-*-m-*-*-*",
  ".MacsBug.foreground:	   Black",
  ".MacsBug.background:	   White",
  ".MacsBug.borderColor:   #AAAAAA",

  ".mac1.foreground:	   Black",
  ".mac1.background:	   White",

  ".macX.textForeground:   White",
  ".macX.textBackground:   Black",
  ".macX.background:	   #888888",
  ".macX.font:		   -*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  ".macX.font2:		   -*-courier-bold-r-*-*-*-240-*-*-m-*-*-*",

  ".SCO.font:		   -*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  ".SCO.font2:		   -*-courier-bold-r-*-*-*-140-*-*-m-*-*-*",
  ".SCO.foreground:	   White",
  ".SCO.background:	   Black",

  ".Linux.font:		   9x15bold",
  ".Linux.font2:	   -*-courier-bold-r-*-*-*-140-*-*-m-*-*-*",
  ".Linux.foreground: White",
  ".Linux.background: Black",

  ".SparcLinux.font:	   -*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  ".SparcLinux.font2:	   -*-courier-bold-r-*-*-*-140-*-*-m-*-*-*",
  ".SparcLinux.foreground: White",
  ".SparcLinux.background: Black",

  ".BSD.font:		    vga",
  ".BSD.font:		    -*-courier-bold-r-*-*-*-120-*-*-m-*-*-*",
  ".BSD.font2:		    -*-courier-bold-r-*-*-*-140-*-*-m-*-*-*",
/* ".BSD.font2:		    -sun-console-medium-r-*-*-22-*-*-*-m-*-*-*", */
  ".BSD.foreground:	    #c0c0c0",
  ".BSD.background:	    Black",

  ".Solaris.font:           -sun-gallant-*-*-*-*-19-*-*-*-*-120-*-*",
  ".Solaris.font2:          -*-courier-bold-r-*-*-*-140-*-*-m-*-*-*",
  ".Solaris.foreground:     Black",
  ".Solaris.background:     White",
  "*dontClearRoot:          True",
  0
};

XrmOptionDescRec options [] = {
  { "-delay",		".delay",		XrmoptionSepArg, 0 },
  { "-windows",		".doWindows",		XrmoptionNoArg,  "True"  },
  { "-no-windows",	".doWindows",		XrmoptionNoArg,  "False" },
  { "-nt",		".doNT",		XrmoptionNoArg,  "True"  },
  { "-no-nt",		".doNT",		XrmoptionNoArg,  "False" },
  { "-2k",		".doWin2K",		XrmoptionNoArg,  "True"  },
  { "-no-2k",		".doWin2K",		XrmoptionNoArg,  "False" },
  { "-amiga",		".doAmiga",		XrmoptionNoArg,  "True"  },
  { "-no-amiga",	".doAmiga",		XrmoptionNoArg,  "False" },
  { "-mac",		".doMac",		XrmoptionNoArg,  "True"  },
  { "-no-mac",		".doMac",		XrmoptionNoArg,  "False" },
  { "-mac1",		".doMac1",		XrmoptionNoArg,  "True"  },
  { "-no-mac1",		".doMac1",		XrmoptionNoArg,  "False" },
  { "-no-macx",		".doMacX",		XrmoptionNoArg,  "False" },
  { "-atari",		".doAtari",		XrmoptionNoArg,  "True"  },
  { "-no-atari",	".doAtari",		XrmoptionNoArg,  "False" },
  { "-macsbug",		".doMacsBug",		XrmoptionNoArg,  "True"  },
  { "-no-macsbug",	".doMacsBug",		XrmoptionNoArg,  "False" },
  { "-sco",		".doSCO",		XrmoptionNoArg,  "True"  },
  { "-no-sco",		".doSCO",		XrmoptionNoArg,  "False" },
  { "-bsd",		".doBSD",		XrmoptionNoArg,  "True"  },
  { "-no-bsd",		".doBSD",		XrmoptionNoArg,  "False" },
  { "-linux",		".doLinux",		XrmoptionNoArg,  "True" },
  { "-no-linux",	".doLinux",		XrmoptionNoArg,  "False" },
  { "-sparclinux",	".doSparcLinux",	XrmoptionNoArg,  "True"  },
  { "-no-sparclinux",	".doSparcLinux",	XrmoptionNoArg,  "False" },
  { "-blitdamage",	".doBlitDamage",	XrmoptionNoArg,  "True"  },
  { "-no-blitdamage",	".doBlitDamage",	XrmoptionNoArg,  "False" },
  { "-solaris",		".doSolaris",		XrmoptionNoArg,  "True"  },
  { "-no-solaris",	".doSolaris",		XrmoptionNoArg,  "False" },
  { 0, 0, 0, 0 }
};


void
screenhack (Display *dpy, Window window)
{
  int loop = 0;
  int i = -1;
  int j = -1;
  int delay = get_integer_resource ("delay", "Integer");
  if (delay < 3) delay = 3;

  if (!get_boolean_resource ("root", "Boolean"))
    {
      XWindowAttributes xgwa;
      XGetWindowAttributes (dpy, window, &xgwa);
      XSelectInput (dpy, window,
                    xgwa.your_event_mask | KeyPressMask | ButtonPressMask);
    }

  while (1)
    {
      Bool did;
      do {  i = (random() & 0xFF) % 15; } while (i == j);
      switch (i)
	{
	case 0:  did = windows(dpy, window, delay, 0); break;
	case 1:  did = windows(dpy, window, delay, 1); break;
	case 2:  did = windows(dpy, window, delay, 2); break;
	case 3:  did = amiga(dpy, window, delay); break;
	case 4:  did = mac(dpy, window, delay); break;
	case 5:  did = macsbug(dpy, window, delay); break;
	case 6:  did = mac1(dpy, window, delay); break;
	case 7:  did = macx(dpy, window, delay); break;
	case 8:  did = sco(dpy, window, delay); break;
	case 9:  did = sparc_linux(dpy, window, delay); break;
 	case 10: did = bsd(dpy, window, delay); break;
	case 11: did = atari(dpy, window, delay); break;
	case 12: did = blitdamage(dpy, window, delay); break;
	case 13: did = sparc_solaris(dpy, window, delay); break;
	case 14: did = linux_fsck(dpy, window, delay); break;
	default: abort(); break;
	}
      loop++;
      if (loop > 100) j = -1;
      if (loop > 200)
        {
          fprintf (stderr, "%s: no display modes enabled?\n", progname);
          exit(-1);
        }
      if (!did) continue;
      XSync (dpy, False);
      j = i;
      loop = 0;
    }
}
