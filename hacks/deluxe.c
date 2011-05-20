/* xscreensaver, Copyright (c) 1999 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include <math.h>
#include "screenhack.h"

#define countof(x) (sizeof(x)/sizeof(*(x)))
#define ABS(x) ((x)<0?-(x):(x))

struct throbber {
  int x, y;
  int size;
  int max_size;
  int thickness;
  int speed;
  int fuse;
  GC gc;
  void (*draw) (Display *, Drawable, struct throbber *);
};

static void
draw_star (Display *dpy, Drawable w, struct throbber *t)
{
  XPoint points[11];
  int x = t->x;
  int y = t->y;
  int s = t->size / 0.383;  /* trial and error, I forget how to derive this */
  int s2 = t->size;
  double c = M_PI * 2;
  double o = -M_PI / 2;

  points[0].x = x + s  * cos(o + 0.0*c); points[0].y = y + s  * sin(o + 0.0*c);
  points[1].x = x + s2 * cos(o + 0.1*c); points[1].y = y + s2 * sin(o + 0.1*c);
  points[2].x = x + s  * cos(o + 0.2*c); points[2].y = y + s  * sin(o + 0.2*c);
  points[3].x = x + s2 * cos(o + 0.3*c); points[3].y = y + s2 * sin(o + 0.3*c);
  points[4].x = x + s  * cos(o + 0.4*c); points[4].y = y + s  * sin(o + 0.4*c);
  points[5].x = x + s2 * cos(o + 0.5*c); points[5].y = y + s2 * sin(o + 0.5*c);
  points[6].x = x + s  * cos(o + 0.6*c); points[6].y = y + s  * sin(o + 0.6*c);
  points[7].x = x + s2 * cos(o + 0.7*c); points[7].y = y + s2 * sin(o + 0.7*c);
  points[8].x = x + s  * cos(o + 0.8*c); points[8].y = y + s  * sin(o + 0.8*c);
  points[9].x = x + s2 * cos(o + 0.9*c); points[9].y = y + s2 * sin(o + 0.9*c);
  points[10] = points[0];

  XDrawLines (dpy, w, t->gc, points, countof(points), CoordModeOrigin);
}

static void
draw_circle (Display *dpy, Drawable w, struct throbber *t)
{
  XDrawArc (dpy, w, t->gc,
            t->x - t->size / 2,
            t->y - t->size / 2,
            t->size, t->size,
            0, 360*64);
}

static void
draw_hlines (Display *dpy, Drawable w, struct throbber *t)
{
  XDrawLine (dpy, w, t->gc, 0,
             t->y - t->size, t->max_size,
             t->y - t->size);
  XDrawLine (dpy, w, t->gc, 0,
             t->y + t->size, t->max_size,
             t->y + t->size);
}

static void
draw_vlines (Display *dpy, Drawable w, struct throbber *t)
{
  XDrawLine (dpy, w, t->gc,
             t->x - t->size, 0,
             t->x - t->size, t->max_size);
  XDrawLine (dpy, w, t->gc,
             t->x + t->size, 0,
             t->x + t->size, t->max_size);
}

static void
draw_corners (Display *dpy, Drawable w, struct throbber *t)
{
  int s = (t->size + t->thickness) / 2;
  XPoint points[3];

  points[0].x = 0;        points[0].y = t->y - s;
  points[1].x = t->x - s; points[1].y = t->y - s;
  points[2].x = t->x - s; points[2].y = 0;
  XDrawLines (dpy, w, t->gc, points, countof(points), CoordModeOrigin);

  points[0].x = 0;        points[0].y = t->y + s;
  points[1].x = t->x - s; points[1].y = t->y + s;
  points[2].x = t->x - s; points[2].y = t->max_size;
  XDrawLines (dpy, w, t->gc, points, countof(points), CoordModeOrigin);

  points[0].x = t->x + s;    points[0].y = 0;
  points[1].x = t->x + s;    points[1].y = t->y - s;
  points[2].x = t->max_size; points[2].y = t->y - s;
  XDrawLines (dpy, w, t->gc, points, countof(points), CoordModeOrigin);

  points[0].x = t->x + s;    points[0].y = t->max_size;
  points[1].x = t->x + s;    points[1].y = t->y + s;
  points[2].x = t->max_size; points[2].y = t->y + s;
  XDrawLines (dpy, w, t->gc, points, countof(points), CoordModeOrigin);
}


static struct throbber *
make_throbber (Display *dpy, Drawable d, int w, int h, unsigned long pixel)
{
  XGCValues gcv;
  struct throbber *t = (struct throbber *) malloc (sizeof (*t));
  t->x = w / 2;
  t->y = h / 2;
  t->max_size = w;
  t->speed = get_integer_resource ("speed", "Speed");
  t->fuse = 1 + (random() % 4);
  t->thickness = get_integer_resource ("thickness", "Thickness");

  if (t->speed < 0) t->speed = -t->speed;
  t->speed += (((random() % t->speed) / 2) - (t->speed / 2));
  if (t->speed > 0) t->speed = -t->speed;

  if (random() % 4)
    t->size = t->max_size;
  else
    t->size = t->thickness, t->speed = -t->speed;

  gcv.foreground = pixel;
  gcv.line_width = t->thickness;
  gcv.line_style = LineSolid;
  gcv.cap_style = CapProjecting;
  gcv.join_style = JoinMiter;
  t->gc = XCreateGC (dpy, d,
                     (GCForeground|GCLineWidth|GCLineStyle|
                      GCCapStyle|GCJoinStyle),
                     &gcv);

  switch (random() % 11) {
  case 0: case 1: case 2: case 3: t->draw = draw_star; break;
  case 4: case 5: case 6: case 7: t->draw = draw_circle; break;
  case 8: t->draw = draw_hlines; break;
  case 9: t->draw = draw_vlines; break;
  case 10: t->draw = draw_corners; break;
  default: abort(); break;
  }

  return t;
}

static int
throb (Display *dpy, Drawable window, struct throbber *t)
{
  t->size += t->speed;
  if (t->size <= (t->thickness / 2))
    {
      t->speed = -t->speed;
      t->size += (t->speed * 2);
    }
  else if (t->size > t->max_size)
    {
      t->speed = -t->speed;
      t->size += (t->speed * 2);
      t->fuse--;
    }

  if (t->fuse <= 0)
    {
      XFreeGC (dpy, t->gc);
      memset (t, 0, sizeof(*t));
      free (t);
      return -1;
    }
  else
    {
      t->draw (dpy, window, t);
      return 0;
    }
}



char *progclass = "Deluxe";

char *defaults [] = {
  ".background:		black",
  ".foreground:		white",
  "*delay:		5000",
  "*count:		5",
  "*thickness:		50",
  "*speed:		15",
  "*ncolors:		20",
  "*doubleBuffer:	True",
  0
};

XrmOptionDescRec options [] = {
  { "-delay",		".delay",	XrmoptionSepArg, 0 },
  { "-thickness",	".thickness",	XrmoptionSepArg, 0 },
  { "-count",		".count",	XrmoptionSepArg, 0 },
  { "-ncolors",		".ncolors",	XrmoptionSepArg, 0 },
  { "-speed",		".speed",	XrmoptionSepArg, 0 },
  { 0, 0, 0, 0 }
};

void
screenhack (Display *dpy, Window window)
{
  int count = get_integer_resource ("count", "Integer");
  int delay = get_integer_resource ("delay", "Integer");
  int ncolors = get_integer_resource ("ncolors", "Integer");
  Bool dbuf = get_boolean_resource ("doubleBuffer", "Boolean");
  XColor colors[255];
  XGCValues gcv;
  GC erase_gc = 0;
  int i;
  struct throbber **throbbers;
  XWindowAttributes xgwa;
  Pixmap b=0, ba=0, bb=0;	/* double-buffer to reduce flicker */

  XGetWindowAttributes (dpy, window, &xgwa);
  make_random_colormap (dpy, xgwa.visual, xgwa.colormap,
                        colors, &ncolors, True, True, 0, True);

  if (dbuf)
    {
      ba = XCreatePixmap (dpy, window, xgwa.width, xgwa.height, xgwa.depth);
      bb = XCreatePixmap (dpy, window, xgwa.width, xgwa.height, xgwa.depth);
      b = ba;
    }
  else
    {
      b = window;
    }

  throbbers = (struct throbber **) calloc (count, sizeof(struct throbber *));
  for (i = 0; i < count; i++)
    throbbers[i] = make_throbber (dpy, b, xgwa.width, xgwa.height,
                                  colors[random() % ncolors].pixel);

  if (dbuf)
    {
      gcv.foreground = get_pixel_resource ("background", "Background",
                                           dpy, xgwa.colormap);
      erase_gc = XCreateGC (dpy, b, GCForeground, &gcv);
      XFillRectangle (dpy, ba, erase_gc, 0, 0, xgwa.width, xgwa.height);
      XFillRectangle (dpy, bb, erase_gc, 0, 0, xgwa.width, xgwa.height);
    }

  while (1)
    {
      if (dbuf)
        XFillRectangle (dpy, b, erase_gc, 0, 0, xgwa.width, xgwa.height);
      else
        XClearWindow (dpy, b);

      for (i = 0; i < count; i++)
        if (throb (dpy, b, throbbers[i]) < 0)
          throbbers[i] = make_throbber (dpy, b, xgwa.width, xgwa.height,
                                        colors[random() % ncolors].pixel);
      if (dbuf)
        {
          XCopyArea (dpy, b, window, erase_gc, 0, 0,
                     xgwa.width, xgwa.height, 0, 0);
          b = (b == ba ? bb : ba);
        }

      XSync (dpy, False);
      screenhack_handle_events (dpy);
      if (delay)
        usleep (delay);
    }
}
