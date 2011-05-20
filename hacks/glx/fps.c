/* tube, Copyright (c) 2001 Jamie Zawinski <jwz@jwz.org>
 * Utility function to draw a frames-per-second display.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>

#include "screenhack.h"
#include "xlockmoreI.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#undef DEBUG  /* Defining this causes check_gl_error() to be called inside
                 time-critical sections, which could slow things down (since
                 it might result in a round-trip, and stall of the pipeline.)
               */

extern void clear_gl_error (void);
extern void check_gl_error (const char *type);

static int fps_text_x = 10;
static int fps_text_y = 10;
static int fps_ascent, fps_descent;
static GLuint font_dlist;
static Bool fps_clear_p = False;
static char fps_string[1024];

static void
fps_init (ModeInfo *mi)
{
  const char *font = get_string_resource ("fpsFont", "Font");
  XFontStruct *f;
  Font id;
  int first, last;

  fps_clear_p = get_boolean_resource ("fpsSolid", "FPSSolid");

  if (!font) font = "-*-courier-bold-r-normal-*-180-*";
  f = XLoadQueryFont(mi->dpy, font);
  if (!f) f = XLoadQueryFont(mi->dpy, "fixed");

  id = f->fid;
  first = f->min_char_or_byte2;
  last = f->max_char_or_byte2;
  
  clear_gl_error ();
  font_dlist = glGenLists ((GLuint) last+1);
  check_gl_error ("glGenLists");

  fps_ascent = f->ascent;
  fps_descent = f->descent;

  if (get_boolean_resource ("fpsTop", "FPSTop"))
    fps_text_y = - (f->ascent + 10);

  glXUseXFont(id, first, last-first+1, font_dlist + first);
  check_gl_error ("glXUseXFont");
}


static void
fps_print_string (ModeInfo *mi, GLfloat x, GLfloat y, const char *string)
{
  const char *L2 = strchr (string, '\n');

  if (y < 0)
    {
      y = mi->xgwa.height + y;
      if (L2)
        y -= (fps_ascent + fps_descent);
    }

# ifdef DEBUG
  clear_gl_error ();
# endif

  /* Sadly, this causes a stall of the graphics pipeline (as would the
     equivalent calls to glGet*.)  But there's no way around this, short
     of having each caller set up the specific display matrix we need
     here, which would kind of defeat the purpose of centralizing this
     code in one file.
   */
  glPushAttrib(GL_TRANSFORM_BIT |  /* for matrix contents */
               GL_ENABLE_BIT |     /* for various glDisable calls */
               GL_CURRENT_BIT |    /* for glColor3f() */
               GL_LIST_BIT);       /* for glListBase() */
  {
# ifdef DEBUG
    check_gl_error ("glPushAttrib");
# endif

    /* disable lighting and texturing when drawing bitmaps!
       (glPopAttrib() restores these, I believe.)
     */
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    /* glPopAttrib() does not restore matrix changes, so we must
       push/pop the matrix stacks to be non-intrusive there.
     */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    {
# ifdef DEBUG
      check_gl_error ("glPushMatrix");
# endif
      glLoadIdentity();

      /* Each matrix mode has its own stack, so we need to push/pop
         them separately. */
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      {
# ifdef DEBUG
        check_gl_error ("glPushMatrix");
# endif
        glLoadIdentity();

        gluOrtho2D(0, mi->xgwa.width, 0, mi->xgwa.height);
# ifdef DEBUG
        check_gl_error ("gluOrtho2D");
# endif

        /* clear the background */
        if (fps_clear_p)
          {
            int lines = L2 ? 2 : 1;
            glColor3f (0, 0, 0);
            glRecti (x / 2, y - fps_descent,
                     mi->xgwa.width - x,
                     y + lines * (fps_ascent + fps_descent));
          }

        /* draw the text */
        glColor3f (1, 1, 1);
        glRasterPos2f (x, y);
        glListBase (font_dlist);

        if (L2)
          {
            L2++;
            glCallLists (strlen(L2), GL_UNSIGNED_BYTE, L2);
            glRasterPos2f (x, y + (fps_ascent + fps_descent));
            glCallLists (L2 - string - 1, GL_UNSIGNED_BYTE, string);
          }
        else
          {
            glCallLists (strlen(string), GL_UNSIGNED_BYTE, string);
          }

# ifdef DEBUG
        check_gl_error ("fps_print_string");
# endif
      }
      glPopMatrix();
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

  }
  /* clean up after our state changes */
  glPopAttrib();
# ifdef DEBUG
  check_gl_error ("glPopAttrib");
# endif
}


GLfloat
fps_1 (ModeInfo *mi)
{
  static Bool initted_p = False;
  static int last_ifps = -1;
  static GLfloat last_fps = -1;
  static int frame_count = 0;
  static struct timeval prev = { 0, };
  static struct timeval now  = { 0, };

  if (!initted_p)
    {
      initted_p = True;
      fps_init (mi);
      strcpy (fps_string, "FPS: (accumulating...)");
    }

  /* Every N frames (where N is approximately one second's worth of frames)
     check the wall clock.  We do this because checking the wall clock is
     a slow operation.
   */
  if (frame_count++ >= last_ifps)
    {
# ifdef GETTIMEOFDAY_TWO_ARGS
      struct timezone tzp;
      gettimeofday(&now, &tzp);
# else
      gettimeofday(&now);
# endif

      if (prev.tv_sec == 0)
        prev = now;
    }

  /* If we've probed the wall-clock time, regenerate the string.
   */
  if (now.tv_sec != prev.tv_sec)
    {
      double uprev = prev.tv_sec + ((double) prev.tv_usec * 0.000001);
      double unow  =  now.tv_sec + ((double)  now.tv_usec * 0.000001);
      double fps   = frame_count / (unow - uprev);

      prev = now;
      frame_count = 0;
      last_ifps = fps;
      last_fps  = fps;

      sprintf (fps_string, "FPS: %.02f", fps);

      if (mi->pause != 0)
        {
          char buf[40];
          sprintf(buf, "%f", mi->pause / 1000000.0); /* FTSO C */
          while(*buf && buf[strlen(buf)-1] == '0')
            buf[strlen(buf)-1] = 0;
          if (buf[strlen(buf)-1] == '.')
            buf[strlen(buf)-1] = 0;
          sprintf(fps_string + strlen(fps_string),
                  " (including %s sec/frame delay)",
                  buf);
        }

      if (mi->polygon_count > 0)
        {
          unsigned long p = mi->polygon_count;
          const char *s = "";
# if 0
          if      (p >= (1024 * 1024)) p >>= 20, s = "M";
          else if (p >= 2048)          p >>= 10, s = "K";
# endif

          strcat (fps_string, "\nPolys: ");
          if (p >= 1000000)
            sprintf (fps_string + strlen(fps_string), "%lu,%03lu,%03lu%s",
                     (p / 1000000), ((p / 1000) % 1000), (p % 1000), s);
          else if (p >= 1000)
            sprintf (fps_string + strlen(fps_string), "%lu,%03lu%s",
                     (p / 1000), (p % 1000), s);
          else
            sprintf (fps_string + strlen(fps_string), "%lu%s", p, s);
        }
    }

  return last_fps;
}

void
fps_2 (ModeInfo *mi)
{
  fps_print_string (mi, fps_text_x, fps_text_y, fps_string);
}


void
do_fps (ModeInfo *mi)
{
  fps_1 (mi);   /* Lazily compute current FPS value, about once a second. */
  fps_2 (mi);   /* Print the string every frame (else nothing shows up.) */
}
