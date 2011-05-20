/* xscreensaver, Copyright (c) 1992, 1993, 1994, 1997, 2001, 2003
 *  Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifndef __GRABSCREEN_H__
#define __GRABSCREEN_H__

/* This will write an image onto the given Drawable.
   The Drawable (arg 3) may be a Window or a Pixmap.

   The Window must be the top-level window.  The image *may or may not*
   be written to the window, though it will definitely be written to
   the drawable.  It's fine for args 2 and 3 to be the same window, or
   for arg 2 to be a Window, and arg 3 to be a Pixmap.

   The loaded image might be from a file, or from a screen shot of the
   desktop, or from the system's video input, depending on user
   preferences.

   Many colors may be allocated from the window's colormap.
 */
extern void load_random_image (Screen *screen,
                               Window top_level_window,
                               Drawable target_window_or_pixmap);


/* Uh, don't call this. */
extern void grab_screen_image (Screen *, Window);


/* Whether one should use GCSubwindowMode when drawing on this window
   (assuming a screen image has been grabbed onto it.)  Yes, this is a
   total kludge. */
extern Bool use_subwindow_mode_p(Screen *screen, Window window);

/* Whether the given window is:
   - the real root window;
   - the virtual root window;
   - a direct child of the root window;
   - a direct child of the window manager's decorations.
 */
extern Bool top_level_window_p(Screen *screen, Window window);

#endif /* __GRABSCREEN_H__ */
