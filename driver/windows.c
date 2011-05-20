/* windows.c --- turning the screen black; dealing with visuals, virtual roots.
 * xscreensaver, Copyright (c) 1991-1998 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef VMS
# include <unixlib.h>		/* for getpid() */
# include "vms-gtod.h"		/* for gettimeofday() */
#endif /* VMS */

#ifndef VMS
# include <pwd.h>		/* for getpwuid() */
#else /* VMS */
# include "vms-pwd.h"
#endif /* VMS */

#ifdef HAVE_UNAME
# include <sys/utsname.h>	/* for uname() */
#endif /* HAVE_UNAME */

#include <stdio.h>
#include <X11/Xproto.h>		/* for CARD32 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>		/* for XSetClassHint() */
#include <X11/Xatom.h>
#include <X11/Xos.h>		/* for time() */
#include <signal.h>		/* for the signal names */

#ifdef HAVE_MIT_SAVER_EXTENSION
# include <X11/extensions/scrnsaver.h>
#endif /* HAVE_MIT_SAVER_EXTENSION */


#ifdef HAVE_XHPDISABLERESET
# include <X11/XHPlib.h>

 /* Calls to XHPDisableReset and XHPEnableReset must be balanced,
    or BadAccess errors occur.  (Ok for this to be global, since it
    affects the whole machine, not just the current screen.) */
  Bool hp_locked_p = False;

#endif /* HAVE_XHPDISABLERESET */


/* This file doesn't need the Xt headers, so stub these types out... */
#undef XtPointer
#define XtAppContext void*
#define XrmDatabase  void*
#define XtIntervalId void*
#define XtPointer    void*
#define Widget       void*

#include "xscreensaver.h"
#include "visual.h"
#include "fade.h"


#ifdef HAVE_VT_LOCKSWITCH
# include <fcntl.h>
# include <sys/ioctl.h>
# include <sys/vt.h>
  static void lock_vt (saver_info *si, Bool lock_p);
#endif /* HAVE_VT_LOCKSWITCH */


extern int kill (pid_t, int);		/* signal() is in sys/signal.h... */

Atom XA_VROOT, XA_XSETROOT_ID;
Atom XA_SCREENSAVER, XA_SCREENSAVER_VERSION, XA_SCREENSAVER_ID;
Atom XA_SCREENSAVER_TIME;


extern saver_info *global_si_kludge;	/* I hate C so much... */


static void store_activate_time (saver_info *si, Bool use_last_p);

#define ALL_POINTER_EVENTS \
	(ButtonPressMask | ButtonReleaseMask | EnterWindowMask | \
	 LeaveWindowMask | PointerMotionMask | PointerMotionHintMask | \
	 Button1MotionMask | Button2MotionMask | Button3MotionMask | \
	 Button4MotionMask | Button5MotionMask | ButtonMotionMask)


static int
grab_kbd(saver_info *si, Window w)
{
  saver_preferences *p = &si->prefs;
  int status = XGrabKeyboard (si->dpy, w, True,
			      /* I don't really understand Sync vs Async,
				 but these seem to work... */
			      GrabModeSync, GrabModeAsync,
			      CurrentTime);
  if (status == GrabSuccess)
    si->keyboard_grab_window = w;

  if (p->verbose_p)
    fprintf(stderr, "%s: grabbing keyboard on 0x%x... %s.\n",
	    blurb(), (unsigned long) w,
	    (status == GrabSuccess ? "GrabSuccess" :
	     status == AlreadyGrabbed ? "AlreadyGrabbed" :
	     status == GrabInvalidTime ? "GrabInvalidTime" :
	     status == GrabNotViewable ? "GrabNotViewable" :
	     status == GrabFrozen ? "GrabFrozen" :
	     "???"));

  return status;
}

static const char *
grab_string(int status)
{
  switch (status)
    {
    case GrabSuccess:     return "GrabSuccess";     break;
    case AlreadyGrabbed:  return "AlreadyGrabbed";  break;
    case GrabInvalidTime: return "GrabInvalidTime"; break;
    case GrabNotViewable: return "GrabNotViewable"; break;
    case GrabFrozen:      return "GrabFrozen";      break;
    default:
      {
	static char foo[255];
	sprintf(foo, "unknown status: %d", status);
	return foo;
      }
    }
}


static int
grab_mouse (saver_info *si, Window w, Cursor cursor)
{
  saver_preferences *p = &si->prefs;
  int status = XGrabPointer (si->dpy, w, True, ALL_POINTER_EVENTS,
			     GrabModeAsync, GrabModeAsync, None,
			     cursor, CurrentTime);
  if (status == GrabSuccess)
    si->mouse_grab_window = w;

  if (p->verbose_p)
    fprintf(stderr, "%s: grabbing mouse on 0x%x... %s.\n",
	    blurb(), (unsigned long) w, grab_string(status));
  return status;
}


static void
ungrab_kbd(saver_info *si)
{
  saver_preferences *p = &si->prefs;
  XUngrabKeyboard(si->dpy, CurrentTime);
  if (p->verbose_p)
    fprintf(stderr, "%s: ungrabbing keyboard (was 0x%x).\n", blurb(),
	    (unsigned long) si->keyboard_grab_window);
  si->keyboard_grab_window = 0;
}


static void
ungrab_mouse(saver_info *si)
{
  saver_preferences *p = &si->prefs;
  XUngrabPointer(si->dpy, CurrentTime);
  if (p->verbose_p)
    fprintf(stderr, "%s: ungrabbing mouse (was 0x%x).\n", blurb(),
	    (unsigned long) si->mouse_grab_window);
  si->mouse_grab_window = 0;
}


Bool
grab_keyboard_and_mouse (saver_info *si, Window window, Cursor cursor)
{
  Status mstatus, kstatus;
  XSync (si->dpy, False);

  kstatus = grab_kbd (si, window);
  if (kstatus != GrabSuccess)
    {	/* try again in a second */
      sleep (1);
      kstatus = grab_kbd (si, window);
      if (kstatus != GrabSuccess)
	fprintf (stderr, "%s: couldn't grab keyboard!  (%s)\n",
		 blurb(), grab_string(kstatus));
    }

  mstatus = grab_mouse (si, window, cursor);
  if (mstatus != GrabSuccess)
    {	/* try again in a second */
      sleep (1);
      mstatus = grab_mouse (si, window, cursor);
      if (mstatus != GrabSuccess)
	fprintf (stderr, "%s: couldn't grab pointer!  (%s)\n",
		 blurb(), grab_string(mstatus));
    }

  return (kstatus == GrabSuccess ||
	  mstatus == GrabSuccess);
}

void
ungrab_keyboard_and_mouse (saver_info *si)
{
  ungrab_mouse (si);
  ungrab_kbd (si);
}


/* Prints an error message to stderr and returns True if there is another
   xscreensaver running already.  Silently returns False otherwise. */
Bool
ensure_no_screensaver_running (Display *dpy, Screen *screen)
{
  Bool status = 0;
  int i;
  Window root = RootWindowOfScreen (screen);
  Window root2, parent, *kids;
  unsigned int nkids;
  XErrorHandler old_handler = XSetErrorHandler (BadWindow_ehandler);

  if (! XQueryTree (dpy, root, &root2, &parent, &kids, &nkids))
    abort ();
  if (root != root2)
    abort ();
  if (parent)
    abort ();
  for (i = 0; i < nkids; i++)
    {
      Atom type;
      int format;
      unsigned long nitems, bytesafter;
      char *version;

      if (XGetWindowProperty (dpy, kids[i], XA_SCREENSAVER_VERSION, 0, 1,
			      False, XA_STRING, &type, &format, &nitems,
			      &bytesafter, (unsigned char **) &version)
	  == Success
	  && type != None)
	{
	  char *id;
	  if (!XGetWindowProperty (dpy, kids[i], XA_SCREENSAVER_ID, 0, 512,
				   False, XA_STRING, &type, &format, &nitems,
				   &bytesafter, (unsigned char **) &id)
	      == Success
	      || type == None)
	    id = "???";

	  fprintf (stderr,
      "%s: already running on display %s (window 0x%x)\n from process %s.\n",
		   blurb(), DisplayString (dpy), (int) kids [i], id);
	  status = True;
	}
    }

  if (kids) XFree ((char *) kids);
  XSync (dpy, False);
  XSetErrorHandler (old_handler);
  return status;
}



/* Virtual-root hackery */

#ifdef _VROOT_H_
ERROR!  You must not include vroot.h in this file.
#endif

static void
store_vroot_property (Display *dpy, Window win, Window value)
{
#if 0
  if (p->verbose_p)
    fprintf (stderr,
	     "%s: storing XA_VROOT = 0x%x (%s) = 0x%x (%s)\n", blurb(), 
	     win,
	     (win == screensaver_window ? "ScreenSaver" :
	      (win == real_vroot ? "VRoot" :
	       (win == real_vroot_value ? "Vroot_value" : "???"))),
	     value,
	     (value == screensaver_window ? "ScreenSaver" :
	      (value == real_vroot ? "VRoot" :
	       (value == real_vroot_value ? "Vroot_value" : "???"))));
#endif
  XChangeProperty (dpy, win, XA_VROOT, XA_WINDOW, 32, PropModeReplace,
		   (unsigned char *) &value, 1);
}

static void
remove_vroot_property (Display *dpy, Window win)
{
#if 0
  if (p->verbose_p)
    fprintf (stderr, "%s: removing XA_VROOT from 0x%x (%s)\n", blurb(), win, 
	     (win == screensaver_window ? "ScreenSaver" :
	      (win == real_vroot ? "VRoot" :
	       (win == real_vroot_value ? "Vroot_value" : "???"))));
#endif
  XDeleteProperty (dpy, win, XA_VROOT);
}


static void
kill_xsetroot_data (Display *dpy, Window window, Bool verbose_p)
{
  Atom type;
  int format;
  unsigned long nitems, bytesafter;
  Pixmap *dataP = 0;

  /* If the user has been using xv or xsetroot as a screensaver (to display
     an image on the screensaver window, as a kind of slideshow) then the
     pixmap and its associated color cells have been put in RetainPermanent
     CloseDown mode.  Since we're not destroying the xscreensaver window,
     but merely unmapping it, we need to free these resources or those
     colormap cells will stay allocated while the screensaver is off.  (We
     could just delete the screensaver window and recreate it later, but
     that could cause other problems.)  This code does an atomic read-and-
     delete of the _XSETROOT_ID property, and if it held a pixmap, then we
     cause the RetainPermanent resources of the client which created it
     (and which no longer exists) to be freed.
   */
  if (XGetWindowProperty (dpy, window, XA_XSETROOT_ID, 0, 1,
			  True, AnyPropertyType, &type, &format, &nitems, 
			  &bytesafter, (unsigned char **) &dataP)
      == Success
      && type != None)
    {
      if (dataP && *dataP && type == XA_PIXMAP && format == 32 &&
	  nitems == 1 && bytesafter == 0)
	{
	  if (verbose_p)
	    fprintf (stderr, "%s: destroying xsetroot data (0x%lX).\n",
		     blurb(), *dataP);
	  XKillClient (dpy, *dataP);
	}
      else
	fprintf (stderr, "%s: deleted unrecognised _XSETROOT_ID property: \n\
	%lu, %lu; type: %lu, format: %d, nitems: %lu, bytesafter %ld\n",
		 blurb(), (unsigned long) dataP, (dataP ? *dataP : 0), type,
		 format, nitems, bytesafter);
    }
}


static void handle_signals (saver_info *si, Bool on_p);

static void
save_real_vroot (saver_screen_info *ssi)
{
  saver_info *si = ssi->global;
  Display *dpy = si->dpy;
  Screen *screen = ssi->screen;
  int i;
  Window root = RootWindowOfScreen (screen);
  Window root2, parent, *kids;
  unsigned int nkids;
  XErrorHandler old_handler;

  /* It's possible that a window might be deleted between our call to
     XQueryTree() and our call to XGetWindowProperty().  Don't die if
     that happens (but just ignore that window, it's not the one we're
     interested in anyway.)
   */
  XSync (dpy, False);
  old_handler = XSetErrorHandler (BadWindow_ehandler);
  XSync (dpy, False);

  ssi->real_vroot = 0;
  ssi->real_vroot_value = 0;
  if (! XQueryTree (dpy, root, &root2, &parent, &kids, &nkids))
    abort ();
  if (root != root2)
    abort ();
  if (parent)
    abort ();
  for (i = 0; i < nkids; i++)
    {
      Atom type;
      int format;
      unsigned long nitems, bytesafter;
      Window *vrootP = 0;

      if (XGetWindowProperty (dpy, kids[i], XA_VROOT, 0, 1, False, XA_WINDOW,
			      &type, &format, &nitems, &bytesafter,
			      (unsigned char **) &vrootP)
	  != Success)
	continue;
      if (! vrootP)
	continue;
      if (ssi->real_vroot)
	{
	  if (*vrootP == ssi->screensaver_window) abort ();
	  fprintf (stderr,
	    "%s: more than one virtual root window found (0x%x and 0x%x).\n",
		   blurb(), (int) ssi->real_vroot, (int) kids [i]);
	  exit (1);
	}
      ssi->real_vroot = kids [i];
      ssi->real_vroot_value = *vrootP;
    }

  XSync (dpy, False);
  XSetErrorHandler (old_handler);
  XSync (dpy, False);

  if (ssi->real_vroot)
    {
      handle_signals (si, True);
      remove_vroot_property (si->dpy, ssi->real_vroot);
      XSync (dpy, False);
    }

  XFree ((char *) kids);
}


static Bool
restore_real_vroot_2 (saver_screen_info *ssi)
{
  saver_info *si = ssi->global;
  saver_preferences *p = &si->prefs;
  if (p->verbose_p && ssi->real_vroot)
    fprintf (stderr,
	     "%s: restoring __SWM_VROOT property on the real vroot (0x%lx).\n",
	     blurb(), (unsigned long) ssi->real_vroot);
  remove_vroot_property (si->dpy, ssi->screensaver_window);
  if (ssi->real_vroot)
    {
      store_vroot_property (si->dpy, ssi->real_vroot, ssi->real_vroot_value);
      ssi->real_vroot = 0;
      ssi->real_vroot_value = 0;
      /* make sure the property change gets there before this process
	 terminates!  We might be doing this because we have intercepted
	 SIGTERM or something. */
      XSync (si->dpy, False);
      return True;
    }
  return False;
}

static Bool
restore_real_vroot_1 (saver_info *si)
{
  int i;
  Bool did_any = False;
  for (i = 0; i < si->nscreens; i++)
    {
      saver_screen_info *ssi = &si->screens[i];
      if (restore_real_vroot_2 (ssi))
	did_any = True;
    }
  return did_any;
}

void
restore_real_vroot (saver_info *si)
{
  if (restore_real_vroot_1 (si))
    handle_signals (si, False);
}


/* Signal hackery to ensure that the vroot doesn't get left in an 
   inconsistent state
 */

const char *
signal_name(int signal)
{
  switch (signal) {
  case SIGHUP:	  return "SIGHUP";
  case SIGINT:	  return "SIGINT";
  case SIGQUIT:	  return "SIGQUIT";
  case SIGILL:	  return "SIGILL";
  case SIGTRAP:	  return "SIGTRAP";
#ifdef SIGABRT
  case SIGABRT:	  return "SIGABRT";
#endif
  case SIGFPE:	  return "SIGFPE";
  case SIGKILL:	  return "SIGKILL";
  case SIGBUS:	  return "SIGBUS";
  case SIGSEGV:	  return "SIGSEGV";
  case SIGPIPE:	  return "SIGPIPE";
  case SIGALRM:	  return "SIGALRM";
  case SIGTERM:	  return "SIGTERM";
#ifdef SIGSTOP
  case SIGSTOP:	  return "SIGSTOP";
#endif
#ifdef SIGCONT
  case SIGCONT:	  return "SIGCONT";
#endif
#ifdef SIGUSR1
  case SIGUSR1:	  return "SIGUSR1";
#endif
#ifdef SIGUSR2
  case SIGUSR2:	  return "SIGUSR2";
#endif
#ifdef SIGEMT
  case SIGEMT:	  return "SIGEMT";
#endif
#ifdef SIGSYS
  case SIGSYS:	  return "SIGSYS";
#endif
#ifdef SIGCHLD
  case SIGCHLD:	  return "SIGCHLD";
#endif
#ifdef SIGPWR
  case SIGPWR:	  return "SIGPWR";
#endif
#ifdef SIGWINCH
  case SIGWINCH:  return "SIGWINCH";
#endif
#ifdef SIGURG
  case SIGURG:	  return "SIGURG";
#endif
#ifdef SIGIO
  case SIGIO:	  return "SIGIO";
#endif
#ifdef SIGVTALRM
  case SIGVTALRM: return "SIGVTALRM";
#endif
#ifdef SIGXCPU
  case SIGXCPU:	  return "SIGXCPU";
#endif
#ifdef SIGXFSZ
  case SIGXFSZ:	  return "SIGXFSZ";
#endif
#ifdef SIGDANGER
  case SIGDANGER: return "SIGDANGER";
#endif
  default:
    {
      static char buf[50];
      sprintf(buf, "signal %d\n", signal);
      return buf;
    }
  }
}



static RETSIGTYPE
restore_real_vroot_handler (int sig)
{
  saver_info *si = global_si_kludge;	/* I hate C so much... */

  signal (sig, SIG_DFL);
  if (restore_real_vroot_1 (si))
    fprintf (real_stderr, "\n%s: %s intercepted, vroot restored.\n",
	     blurb(), signal_name(sig));
  kill (getpid (), sig);
}

static void
catch_signal (saver_info *si, int sig, Bool on_p)
{
  if (! on_p)
    signal (sig, SIG_DFL);
  else
    {
      if (((long) signal (sig, restore_real_vroot_handler)) == -1L)
	{
	  char buf [255];
	  sprintf (buf, "%s: couldn't catch %s", blurb(), signal_name(sig));
	  perror (buf);
	  saver_exit (si, 1, 0);
	}
    }
}

static void
handle_signals (saver_info *si, Bool on_p)
{
#if 0
  if (on_p) fprintf (stderr, "handling signals\n");
  else fprintf (stderr, "unhandling signals\n");
#endif

  catch_signal (si, SIGHUP,  on_p);
  catch_signal (si, SIGINT,  on_p);
  catch_signal (si, SIGQUIT, on_p);
  catch_signal (si, SIGILL,  on_p);
  catch_signal (si, SIGTRAP, on_p);
  catch_signal (si, SIGIOT,  on_p);
  catch_signal (si, SIGABRT, on_p);
#ifdef SIGEMT
  catch_signal (si, SIGEMT,  on_p);
#endif
  catch_signal (si, SIGFPE,  on_p);
  catch_signal (si, SIGBUS,  on_p);
  catch_signal (si, SIGSEGV, on_p);
#ifdef SIGSYS
  catch_signal (si, SIGSYS,  on_p);
#endif
  catch_signal (si, SIGTERM, on_p);
#ifdef SIGXCPU
  catch_signal (si, SIGXCPU, on_p);
#endif
#ifdef SIGXFSZ
  catch_signal (si, SIGXFSZ, on_p);
#endif
#ifdef SIGDANGER
  catch_signal (si, SIGDANGER, on_p);
#endif
}

void
saver_exit (saver_info *si, int status, const char *dump_core_reason)
{
  saver_preferences *p = &si->prefs;
  static Bool exiting = False;
  Bool bugp;
  Bool vrs;

  if (exiting)
    exit(status);

  exiting = True;
  
  vrs = restore_real_vroot_1 (si);
  emergency_kill_subproc (si);

  if (vrs && (p->verbose_p || status != 0))
    fprintf (real_stderr, "%s: vroot restored, exiting.\n", blurb());
  else if (p->verbose_p)
    fprintf (real_stderr, "%s: no vroot to restore; exiting.\n", blurb());

  fflush(real_stdout);

#ifdef VMS	/* on VMS, 1 is the "normal" exit code instead of 0. */
  if (status == 0) status = 1;
  else if (status == 1) status = -1;
#endif

  bugp = !!dump_core_reason;

  if (si->prefs.debug_p && !dump_core_reason)
    dump_core_reason = "because of -debug";

  if (dump_core_reason)
    {
      /* Note that the Linux man page for setuid() says If uid is
	 different from the old effective uid, the process will be
	 forbidden from leaving core dumps.
      */
      char cwd[4096]; /* should really be PATH_MAX, but who cares. */
      cwd[0] = 0;
      fprintf(real_stderr, "%s: dumping core (%s)\n", blurb(),
	      dump_core_reason);

      if (bugp)
	fprintf(real_stderr,
		"%s: see http://www.jwz.org/xscreensaver/bugs.html\n"
		"\t\tfor bug reporting information.\n\n",
		blurb());

# if defined(HAVE_GETCWD)
      if (!getcwd (cwd, sizeof(cwd)))
# elif defined(HAVE_GETWD)
      if (!getwd (cwd))
# endif
        strcpy(cwd, "unknown.");

      fprintf (real_stderr, "%s: current directory is %s\n", blurb(), cwd);
      describe_uids (si, real_stderr);

      /* Do this to drop a core file, so that we can get a stack trace. */
      abort();
    }

  exit (status);
}


/* Managing the actual screensaver window */

Bool
window_exists_p (Display *dpy, Window window)
{
  XErrorHandler old_handler;
  XWindowAttributes xgwa;
  xgwa.screen = 0;
  old_handler = XSetErrorHandler (BadWindow_ehandler);
  XGetWindowAttributes (dpy, window, &xgwa);
  XSync (dpy, False);
  XSetErrorHandler (old_handler);
  return (xgwa.screen != 0);
}

static void
store_saver_id (saver_screen_info *ssi)
{
  XClassHint class_hints;
  saver_info *si = ssi->global;
  unsigned long pid = (unsigned long) getpid ();
  char buf[20];
  struct passwd *p = getpwuid (getuid ());
  const char *name, *host;
  char *id;
  
  /* First store the name and class on the window.
   */
  class_hints.res_name = progname;
  class_hints.res_class = progclass;
  XSetClassHint (si->dpy, ssi->screensaver_window, &class_hints);
  XStoreName (si->dpy, ssi->screensaver_window, "screensaver");

  /* Then store the xscreensaver version number.
   */
  XChangeProperty (si->dpy, ssi->screensaver_window,
		   XA_SCREENSAVER_VERSION,
		   XA_STRING, 8, PropModeReplace,
		   (unsigned char *) si->version,
		   strlen (si->version));

  /* Now store the XSCREENSAVER_ID property, that says what user and host
     xscreensaver is running as.
   */

  if (p && p->pw_name && *p->pw_name)
    name = p->pw_name;
  else if (p)
    {
      sprintf (buf, "%lu", (unsigned long) p->pw_uid);
      name = buf;
    }
  else
    name = "???";

# if defined(HAVE_UNAME)
  {
    struct utsname uts;
    if (uname (&uts) < 0)
      host = "???";
    else
      host = uts.nodename;
  }
# elif defined(VMS)
  host = getenv("SYS$NODE");
# else  /* !HAVE_UNAME && !VMS */
  host = "???";
# endif /* !HAVE_UNAME && !VMS */

  id = (char *) malloc (strlen(name) + strlen(host) + 50);
  sprintf (id, "%lu (%s@%s)", pid, name, host);

  XChangeProperty (si->dpy, ssi->screensaver_window,
		   XA_SCREENSAVER_ID, XA_STRING,
		   8, PropModeReplace,
		   (unsigned char *) id, strlen (id));
  free (id);
}


static void
initialize_screensaver_window_1 (saver_screen_info *ssi)
{
  saver_info *si = ssi->global;
  saver_preferences *p = &si->prefs;
  Bool install_cmap_p = ssi->install_cmap_p;   /* not p->install_cmap_p */

  /* This resets the screensaver window as fully as possible, since there's
     no way of knowing what some random client may have done to us in the
     meantime.  We could just destroy and recreate the window, but that has
     its own set of problems...
   */
  XColor black;
  XSetWindowAttributes attrs;
  unsigned long attrmask;
  int width = WidthOfScreen (ssi->screen);
  int height = HeightOfScreen (ssi->screen);
  static Bool printed_visual_info = False;  /* only print the message once. */

  black.red = black.green = black.blue = 0;

  if (ssi->cmap == DefaultColormapOfScreen (ssi->screen))
    ssi->cmap = 0;

  if (ssi->current_visual != DefaultVisualOfScreen (ssi->screen))
    /* It's not the default visual, so we have no choice but to install. */
    install_cmap_p = True;

  if (install_cmap_p)
    {
      if (! ssi->cmap)
	{
	  ssi->cmap = XCreateColormap (si->dpy,
				       RootWindowOfScreen (ssi->screen),
				      ssi->current_visual, AllocNone);
	  if (! XAllocColor (si->dpy, ssi->cmap, &black)) abort ();
	  ssi->black_pixel = black.pixel;
	}
    }
  else
    {
      Colormap def_cmap = DefaultColormapOfScreen (ssi->screen);
      if (ssi->cmap)
	{
	  XFreeColors (si->dpy, ssi->cmap, &ssi->black_pixel, 1, 0);
	  if (ssi->cmap != ssi->demo_cmap &&
	      ssi->cmap != def_cmap)
	    XFreeColormap (si->dpy, ssi->cmap);
	}
      ssi->cmap = def_cmap;
      ssi->black_pixel = BlackPixelOfScreen (ssi->screen);
    }

  attrmask = (CWOverrideRedirect | CWEventMask | CWBackingStore | CWColormap |
	      CWBackPixel | CWBackingPixel | CWBorderPixel);
  attrs.override_redirect = True;

  /* When use_mit_saver_extension or use_sgi_saver_extension is true, we won't
     actually be reading these events during normal operation; but we still
     need to see Button events for demo-mode to work properly.
   */
  attrs.event_mask = (KeyPressMask | KeyReleaseMask |
		      ButtonPressMask | ButtonReleaseMask |
		      PointerMotionMask);

  attrs.backing_store = NotUseful;
  attrs.colormap = ssi->cmap;
  attrs.background_pixel = ssi->black_pixel;
  attrs.backing_pixel = ssi->black_pixel;
  attrs.border_pixel = ssi->black_pixel;

  if (p->debug_p) width = width / 2;

  if (!p->verbose_p || printed_visual_info)
    ;
  else if (ssi->current_visual == DefaultVisualOfScreen (ssi->screen))
    {
      fprintf (stderr, "%s: using default visual ", blurb());
      describe_visual (stderr, ssi->screen, ssi->current_visual,
		       install_cmap_p);
    }
  else
    {
      fprintf (stderr, "%s: using visual:   ", blurb());
      describe_visual (stderr, ssi->screen, ssi->current_visual,
		       install_cmap_p);
      fprintf (stderr, "%s: default visual: ", blurb());
      describe_visual (stderr, ssi->screen,
		       DefaultVisualOfScreen (ssi->screen),
		       ssi->install_cmap_p);
    }
  printed_visual_info = True;

#ifdef HAVE_MIT_SAVER_EXTENSION
  if (si->using_mit_saver_extension)
    {
      XScreenSaverInfo *info;
      Window root = RootWindowOfScreen (ssi->screen);

#if 0
      /* This call sets the server screensaver timeouts to what we think
	 they should be (based on the resources and args xscreensaver was
	 started with.)  It's important that we do this to sync back up
	 with the server - if we have turned on prematurely, as by an
	 ACTIVATE ClientMessage, then the server may decide to activate
	 the screensaver while it's already active.  That's ok for us,
	 since we would know to ignore that ScreenSaverActivate event,
	 but a side effect of this would be that the server would map its
	 saver window (which we then hide again right away) meaning that
	 the bits currently on the screen get blown away.  Ugly. */

      /* #### Ok, that doesn't work - when we tell the server that the
	 screensaver is "off" it sends us a Deactivate event, which is
	 sensible... but causes the saver to never come on.  Hmm. */
      disable_builtin_screensaver (si, True);
#endif /* 0 */

#if 0
      /* #### The MIT-SCREEN-SAVER extension gives us access to the
	 window that the server itself uses for saving the screen.
	 However, using this window in any way, in particular, calling
	 XScreenSaverSetAttributes() as below, tends to make the X server
	 crash.  So fuck it, let's try and get along without using it...

	 It's also inconvenient to use this window because it doesn't
	 always exist (though the ID is constant.)  So to use this
	 window, we'd have to reimplement the ACTIVATE ClientMessage to
	 tell the *server* to tell *us* to turn on, to cause the window
	 to get created at the right time.  Gag.  */
      XScreenSaverSetAttributes (si->dpy, root,
				 0, 0, width, height, 0,
				 current_depth, InputOutput, visual,
				 attrmask, &attrs);
      XSync (si->dpy, False);
#endif /* 0 */

      info = XScreenSaverAllocInfo ();
      XScreenSaverQueryInfo (si->dpy, root, info);
      ssi->server_mit_saver_window = info->window;
      if (! ssi->server_mit_saver_window) abort ();
      XFree (info);
    }
#endif /* HAVE_MIT_SAVER_EXTENSION */

  if (ssi->screensaver_window)
    {
      XWindowChanges changes;
      unsigned int changesmask = CWX|CWY|CWWidth|CWHeight|CWBorderWidth;
      changes.x = 0;
      changes.y = 0;
      changes.width = width;
      changes.height = height;
      changes.border_width = 0;

      XConfigureWindow (si->dpy, ssi->screensaver_window,
			changesmask, &changes);
      XChangeWindowAttributes (si->dpy, ssi->screensaver_window,
			       attrmask, &attrs);
    }
  else
    {
      ssi->screensaver_window =
	XCreateWindow (si->dpy, RootWindowOfScreen (ssi->screen), 0, 0,
		       width, height, 0, ssi->current_depth, InputOutput,
		       ssi->current_visual, attrmask, &attrs);
      reset_stderr (ssi);
      store_activate_time(si, True);
      if (p->verbose_p)
	fprintf (stderr, "%s: saver window is 0x%lx.\n",
		 blurb(), (unsigned long) ssi->screensaver_window);
    }


  store_saver_id (ssi);

  if (!ssi->cursor)
    {
      Pixmap bit;
      bit = XCreatePixmapFromBitmapData (si->dpy, ssi->screensaver_window,
					 "\000", 1, 1,
					 BlackPixelOfScreen (ssi->screen),
					 BlackPixelOfScreen (ssi->screen),
					 1);
      ssi->cursor = XCreatePixmapCursor (si->dpy, bit, bit, &black, &black,
					 0, 0);
      XFreePixmap (si->dpy, bit);
    }

  XSetWindowBackground (si->dpy, ssi->screensaver_window, ssi->black_pixel);

  if (si->demoing_p)
    XUndefineCursor (si->dpy, ssi->screensaver_window);
  else
    XDefineCursor (si->dpy, ssi->screensaver_window, ssi->cursor);
}

void
initialize_screensaver_window (saver_info *si)
{
  int i;
  for (i = 0; i < si->nscreens; i++)
    initialize_screensaver_window_1 (&si->screens[i]);
}


void 
raise_window (saver_info *si,
	      Bool inhibit_fade, Bool between_hacks_p, Bool dont_clear)
{
  saver_preferences *p = &si->prefs;
  int i;

  if (si->demoing_p)
    inhibit_fade = True;

  initialize_screensaver_window (si);
  reset_watchdog_timer (si, True);

  if (p->fade_p && p->fading_possible_p && !inhibit_fade)
    {
      Window *current_windows = (Window *)
	calloc(sizeof(Window), si->nscreens);
      Colormap *current_maps = (Colormap *)
	calloc(sizeof(Colormap), si->nscreens);

      for (i = 0; i < si->nscreens; i++)
	{
	  saver_screen_info *ssi = &si->screens[i];
	  current_windows[i] = ssi->screensaver_window;
	  current_maps[i] = (between_hacks_p
			     ? ssi->cmap
			     : DefaultColormapOfScreen (ssi->screen));
	  /* Ensure that the default background of the window is really black,
	     not a pixmap or something.  (This does not clear the window.) */
	  XSetWindowBackground (si->dpy, ssi->screensaver_window,
				ssi->black_pixel);
	}

      if (p->verbose_p) fprintf (stderr, "%s: fading...\n", blurb());

      XGrabServer (si->dpy);			/* ############ DANGER! */

      /* Clear the stderr layer on each screen.
       */
      if (!dont_clear)
	for (i = 0; i < si->nscreens; i++)
	  {
	    saver_screen_info *ssi = &si->screens[i];
	    if (ssi->stderr_overlay_window)
	      /* Do this before the fade, since the stderr cmap won't fade
		 even if we uninstall it (beats me...) */
	      clear_stderr (ssi);
	  }

      /* Note!  The server is grabbed, and this will take several seconds
	 to complete! */
      fade_screens (si->dpy, current_maps, current_windows,
		    p->fade_seconds/1000, p->fade_ticks, True, !dont_clear);

      free(current_maps);
      free(current_windows);
      current_maps = 0;
      current_windows = 0;

      if (p->verbose_p) fprintf (stderr, "%s: fading done.\n", blurb());

#ifdef HAVE_MIT_SAVER_EXTENSION
      for (i = 0; i < si->nscreens; i++)
	{
	  saver_screen_info *ssi = &si->screens[i];
	  if (ssi->server_mit_saver_window &&
	      window_exists_p (si->dpy, ssi->server_mit_saver_window))
	    XUnmapWindow (si->dpy, ssi->server_mit_saver_window);
	}
#endif /* HAVE_MIT_SAVER_EXTENSION */

      XUngrabServer (si->dpy);
      XSync (si->dpy, False);			/* ###### (danger over) */
    }
  else
    {
      for (i = 0; i < si->nscreens; i++)
	{
	  saver_screen_info *ssi = &si->screens[i];
	  if (!dont_clear)
	    XClearWindow (si->dpy, ssi->screensaver_window);
	  if (!dont_clear || ssi->stderr_overlay_window)
	    clear_stderr (ssi);
	  XMapRaised (si->dpy, ssi->screensaver_window);
#ifdef HAVE_MIT_SAVER_EXTENSION
	  if (ssi->server_mit_saver_window &&
	      window_exists_p (si->dpy, ssi->server_mit_saver_window))
	    XUnmapWindow (si->dpy, ssi->server_mit_saver_window);
#endif /* HAVE_MIT_SAVER_EXTENSION */
	}
    }

  for (i = 0; i < si->nscreens; i++)
    {
      saver_screen_info *ssi = &si->screens[i];
      if (ssi->cmap)
	XInstallColormap (si->dpy, ssi->cmap);
    }
}

void
blank_screen (saver_info *si)
{
  int i;

  /* Note: we do our grabs on the root window, not on the screensaver window.
     If we grabbed on the saver window, then the demo mode and lock dialog
     boxes wouldn't get any events.
   */
  grab_keyboard_and_mouse (si,
			   /*si->screens[0].screensaver_window,*/
			   RootWindowOfScreen(si->screens[0].screen),
			   (si->demoing_p
			    ? 0
			    : si->screens[0].cursor));

  for (i = 0; i < si->nscreens; i++)
    {
      saver_screen_info *ssi = &si->screens[i];

      save_real_vroot (ssi);
      store_vroot_property (si->dpy,
			    ssi->screensaver_window,
			    ssi->screensaver_window);
    }
  store_activate_time (si, si->screen_blanked_p);
  raise_window (si, False, False, False);

#ifdef HAVE_XHPDISABLERESET
  if (si->locked_p && !hp_locked_p)
    {
      XHPDisableReset (si->dpy);	/* turn off C-Sh-Reset */
      hp_locked_p = True;
    }
#endif

#ifdef HAVE_VT_LOCKSWITCH
  if (si->locked_p)
      lock_vt (si, True);		/* turn off C-Alt-Fn */
#endif

  si->screen_blanked_p = True;
}

void
unblank_screen (saver_info *si)
{
  saver_preferences *p = &si->prefs;
  Bool unfade_p = (p->fading_possible_p && p->unfade_p);
  int i;

  monitor_power_on (si);

  store_activate_time (si, True);
  reset_watchdog_timer (si, False);

  if (si->demoing_p)
    unfade_p = False;

  if (unfade_p)
    {
      Window *current_windows = (Window *)
	calloc(sizeof(Window), si->nscreens);

      for (i = 0; i < si->nscreens; i++)
	{
	  saver_screen_info *ssi = &si->screens[i];
	  current_windows[i] = ssi->screensaver_window;
	  /* Ensure that the default background of the window is really black,
	     not a pixmap or something.  (This does not clear the window.) */
	  XSetWindowBackground (si->dpy, ssi->screensaver_window,
				ssi->black_pixel);
	}

      if (p->verbose_p) fprintf (stderr, "%s: unfading...\n", blurb());


      XSync (si->dpy, False);
      XGrabServer (si->dpy);			/* ############ DANGER! */
      XSync (si->dpy, False);

      /* Clear the stderr layer on each screen.
       */
      for (i = 0; i < si->nscreens; i++)
	{
	  saver_screen_info *ssi = &si->screens[i];
	  clear_stderr (ssi);
	}

      XUngrabServer (si->dpy);
      XSync (si->dpy, False);			/* ###### (danger over) */


      fade_screens (si->dpy, 0, current_windows,
		    p->fade_seconds/1000, p->fade_ticks,
		    False, False);

      free(current_windows);
      current_windows = 0;

      if (p->verbose_p) fprintf (stderr, "%s: unfading done.\n", blurb());
    }
  else
    {
      for (i = 0; i < si->nscreens; i++)
	{
	  saver_screen_info *ssi = &si->screens[i];
	  if (ssi->cmap)
	    {
	      Colormap c = DefaultColormapOfScreen (ssi->screen);
	      /* avoid technicolor */
	      XClearWindow (si->dpy, ssi->screensaver_window);
	      if (c) XInstallColormap (si->dpy, c);
	    }
	  XUnmapWindow (si->dpy, ssi->screensaver_window);
	}
    }


  /* If the focus window does has a non-default colormap, then install
     that colormap as well.  (On SGIs, this will cause both the root map
     and the focus map to be installed simultaniously.  It'd be nice to
     pick up the other colormaps that had been installed, too; perhaps
     XListInstalledColormaps could be used for that?)
   */
  {
    Window focus = 0;
    int revert_to;
    XGetInputFocus (si->dpy, &focus, &revert_to);
    if (focus && focus != PointerRoot && focus != None)
      {
	XWindowAttributes xgwa;
	xgwa.colormap = 0;
	XGetWindowAttributes (si->dpy, focus, &xgwa);
	if (xgwa.colormap &&
	    xgwa.colormap != DefaultColormapOfScreen (xgwa.screen))
	  XInstallColormap (si->dpy, xgwa.colormap);
      }
  }


  for (i = 0; i < si->nscreens; i++)
    {
      saver_screen_info *ssi = &si->screens[i];
      kill_xsetroot_data (si->dpy, ssi->screensaver_window, p->verbose_p);
    }

  store_activate_time(si, False);  /* store unblank time */

  ungrab_keyboard_and_mouse (si);
  restore_real_vroot (si);

#ifdef HAVE_XHPDISABLERESET
  if (hp_locked_p)
    {
      XHPEnableReset (si->dpy);	/* turn C-Sh-Reset back on */
      hp_locked_p = False;
    }
#endif

#ifdef HAVE_VT_LOCKSWITCH
  lock_vt (si, False);		/* turn C-Alt-Fn back on */
#endif

  /* Unmap the windows a second time, dammit -- just to avoid a race
     with the screen-grabbing hacks.  (I'm not sure if this is really
     necessary; I'm stabbing in the dark now.)
  */
  for (i = 0; i < si->nscreens; i++)
    XUnmapWindow (si->dpy, si->screens[i].screensaver_window);

  si->screen_blanked_p = False;
}


static void
store_activate_time (saver_info *si, Bool use_last_p)
{
  static time_t last_time = 0;
  time_t now = ((use_last_p && last_time) ? last_time : time ((time_t) 0));
  CARD32 now32 = (CARD32) now;
  int i;
  last_time = now;

  for (i = 0; i < si->nscreens; i++)
    {
      saver_screen_info *ssi = &si->screens[i];
      if (!ssi->screensaver_window) continue;
      XChangeProperty (si->dpy, ssi->screensaver_window, XA_SCREENSAVER_TIME,
		       XA_INTEGER, 32, PropModeReplace,
		       (unsigned char *) &now32, 1);
    }
}


Bool
select_visual (saver_screen_info *ssi, const char *visual_name)
{
  saver_info *si = ssi->global;
  saver_preferences *p = &si->prefs;
  Bool install_cmap_p = p->install_cmap_p;
  Bool was_installed_p = (ssi->cmap != DefaultColormapOfScreen(ssi->screen));
  Visual *new_v;
  Bool got_it;

  if (visual_name && *visual_name)
    {
      if (!strcmp(visual_name, "default-i"))
	{
	  visual_name = "default";
	  install_cmap_p = True;
	}
      else if (!strcmp(visual_name, "default-n"))
	{
	  visual_name = "default";
	  install_cmap_p = False;
	}
      new_v = get_visual (ssi->screen, visual_name, True, False);
    }
  else
    {
      new_v = ssi->default_visual;
    }

  got_it = !!new_v;

  if (new_v && new_v != DefaultVisualOfScreen(ssi->screen))
    /* It's not the default visual, so we have no choice but to install. */
    install_cmap_p = True;

  ssi->install_cmap_p = install_cmap_p;

  if (new_v &&
      ((ssi->current_visual != new_v) ||
       (install_cmap_p != was_installed_p)))
    {
      Colormap old_c = ssi->cmap;
      Window old_w = ssi->screensaver_window;

      if (p->verbose_p)
	{
	  fprintf (stderr, "%s: switching to visual ", blurb());
	  describe_visual (stderr, ssi->screen, new_v, install_cmap_p);
#if 0
	  fprintf (stderr, "%s:                from ", blurb());
	  describe_visual (stderr, ssi->screen, ssi->current_visual,
			   was_installed_p);
#endif
	}

      reset_stderr (ssi);
      ssi->current_visual = new_v;
      ssi->current_depth = visual_depth(ssi->screen, new_v);
      ssi->cmap = 0;
      ssi->screensaver_window = 0;

      initialize_screensaver_window_1 (ssi);

      /* stderr_overlay_window is a child of screensaver_window, so we need
	 to destroy that as well (actually, we just need to invalidate and
	 drop our pointers to it, but this will destroy it, which is ok so
	 long as it happens before old_w itself is destroyed.) */
      reset_stderr (ssi);

      raise_window (si, True, True, False);
      store_vroot_property (si->dpy,
			    ssi->screensaver_window, ssi->screensaver_window);
      store_activate_time (si, True);



      /* Transfer the grabs from the old window to the new.
	 Actually I think none of this is necessary, since we always
	 hold our grabs on the root window, but I wrote this before
	 re-discovering that...
       */


      /* If we're destroying the window that holds our mouse grab,
	 transfer the grab to the new window.  (Grab the server while
	 so doing, to avoid a race condition.)
       */
      if (old_w == si->mouse_grab_window)
	{
	  XGrabServer (si->dpy);		/* ############ DANGER! */
	  ungrab_mouse (si);
	  grab_mouse (si, ssi->screensaver_window,
		      (si->demoing_p
		       ? 0
		       : ssi->cursor));
	  XUngrabServer (si->dpy);
	  XSync (si->dpy, False);		/* ###### (danger over) */
	}

      /* If we're destroying the window that holds our keyboard grab,
	 transfer the grab to the new window.  (Grab the server while
	 so doing, to avoid a race condition.)
       */
      if (old_w == si->keyboard_grab_window)
	{
	  XGrabServer (si->dpy);		/* ############ DANGER! */
	  ungrab_kbd(si);
	  grab_kbd(si, ssi->screensaver_window);
	  XUngrabServer (si->dpy);
	  XSync (si->dpy, False);		/* ###### (danger over) */
	}

      /* Now we can destroy this window without horking our grabs. */

      XDestroyWindow (si->dpy, old_w);

      if (p->verbose_p)
	fprintf (stderr, "%s: destroyed old saver window 0x%lx.\n",
		 blurb(), (unsigned long) old_w);

      if (old_c &&
	  old_c != DefaultColormapOfScreen (ssi->screen) &&
	  old_c != ssi->demo_cmap)
	XFreeColormap (si->dpy, old_c);
    }

  return got_it;
}


/* VT locking */

#ifdef HAVE_VT_LOCKSWITCH
static void
lock_vt (saver_info *si, Bool lock_p)
{
  saver_preferences *p = &si->prefs;
  static Bool locked_p = False;
  const char *dev_console = "/dev/console";
  int fd;

  if (lock_p == locked_p)
    return;

  if (lock_p && !p->lock_vt_p)
    return;

  fd = open (dev_console, O_RDWR);
  if (fd < 0)
    {
      char buf [255];
      sprintf (buf, "%s: couldn't %s VTs: %s", blurb(),
	       (lock_p ? "lock" : "unlock"),
	       dev_console);
#if 0 /* #### doesn't work yet, so don't bother complaining */
      perror (buf);
#endif
      return;
    }

  if (ioctl (fd, (lock_p ? VT_LOCKSWITCH : VT_UNLOCKSWITCH)) == 0)
    {
      locked_p = lock_p;

      if (p->verbose_p)
	fprintf (stderr, "%s: %s VTs\n", blurb(),
		 (lock_p ? "locked" : "unlocked"));
    }
  else
    {
      char buf [255];
      sprintf (buf, "%s: couldn't %s VTs: ioctl", blurb(),
	       (lock_p ? "lock" : "unlock"));
#if 0 /* #### doesn't work yet, so don't bother complaining */
      perror (buf);
#endif
    }

  close (fd);
}
#endif /* HAVE_VT_LOCKSWITCH */
