/* xscreensaver, Copyright (c) 2006-2013 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/* This is a subclass of Apple's ScreenSaverView that knows how to run
   xscreensaver programs without X11 via the dark magic of the "jwxyz"
   library.  In xscreensaver terminology, this is the replacement for
   the "screenhack.c" module.
 */

#import <QuartzCore/QuartzCore.h>
#import <zlib.h>
#import "XScreenSaverView.h"
#import "XScreenSaverConfigSheet.h"
#import "Updater.h"
#import "screenhackI.h"
#import "xlockmoreI.h"
#import "jwxyz-timers.h"


extern struct xscreensaver_function_table *xscreensaver_function_table;

/* Global variables used by the screen savers
 */
const char *progname;
const char *progclass;
int mono_p = 0;


# ifdef USE_IPHONE

extern NSDictionary *make_function_table_dict(void);  // ios-function-table.m

/* Stub definition of the superclass, for iPhone.
 */
@implementation ScreenSaverView
{
  NSTimeInterval anim_interval;
  Bool animating_p;
  NSTimer *anim_timer;
}

- (id)initWithFrame:(NSRect)frame isPreview:(BOOL)isPreview {
  self = [super initWithFrame:frame];
  if (! self) return 0;
  anim_interval = 1.0/30;
  return self;
}
- (NSTimeInterval)animationTimeInterval { return anim_interval; }
- (void)setAnimationTimeInterval:(NSTimeInterval)i { anim_interval = i; }
- (BOOL)hasConfigureSheet { return NO; }
- (NSWindow *)configureSheet { return nil; }
- (NSView *)configureView { return nil; }
- (BOOL)isPreview { return NO; }
- (BOOL)isAnimating { return animating_p; }
- (void)animateOneFrame { }

- (void)startAnimation {
  if (animating_p) return;
  animating_p = YES;
  anim_timer = [NSTimer scheduledTimerWithTimeInterval: anim_interval
                        target:self
                        selector:@selector(animateOneFrame)
                        userInfo:nil
                        repeats:YES];
}

- (void)stopAnimation {
  if (anim_timer) {
    [anim_timer invalidate];
    anim_timer = 0;
  }
  animating_p = NO;
}
@end

# endif // !USE_IPHONE



@interface XScreenSaverView (Private)
- (void) stopAndClose:(Bool)relaunch;
@end

@implementation XScreenSaverView

// Given a lower-cased saver name, returns the function table for it.
// If no name, guess the name from the class's bundle name.
//
- (struct xscreensaver_function_table *) findFunctionTable:(NSString *)name
{
  NSBundle *nsb = [NSBundle bundleForClass:[self class]];
  NSAssert1 (nsb, @"no bundle for class %@", [self class]);

  CFURLRef url = CFBridgingRetain([nsb bundleURL]);
  CFBundleRef cfb = CFBundleCreate (kCFAllocatorDefault, url);
  CFRelease (url);
  NSAssert1 (cfb, @"no CFBundle for \"%@\"", url);
  // #### Analyze says "Potential leak of an object stored into cfb"
  
  if (! name)
    name = [[[nsb bundleURL] lastPathComponent] stringByDeletingPathExtension];

  name = [[name lowercaseString]
           stringByReplacingOccurrencesOfString:@" "
           withString:@""];

# ifndef USE_IPHONE
  // CFBundleGetDataPointerForName doesn't work in "Archive" builds.
  // I'm guessing that symbol-stripping is mandatory.  Fuck.
  NSString *table_name = [name stringByAppendingString:
                                 @"_xscreensaver_function_table"];
  void *addr = CFBundleGetDataPointerForName (cfb, (__bridge CFStringRef) table_name);
  CFRelease (cfb);

  if (! addr)
    NSLog (@"no symbol \"%@\" for \"%@\"", table_name, [nsb bundleURL]);

# else  // USE_IPHONE
  // Remember: any time you add a new saver to the iOS app,
  // manually run "make ios-function-table.m"!
  if (! function_tables)
    function_tables = make_function_table_dict();
  NSValue *v = function_tables[name];
  void *addr = v ? [v pointerValue] : 0;
# endif // USE_IPHONE

  return (struct xscreensaver_function_table *) addr;
}


// Add the "Contents/Resources/" subdirectory of this screen saver's .bundle
// to $PATH for the benefit of savers that include helper shell scripts.
//
- (void) setShellPath
{
  NSBundle *nsb = [NSBundle bundleForClass:[self class]];
  NSAssert1 (nsb, @"no bundle for class %@", [self class]);
  
  NSString *nsdir = [nsb resourcePath];
  NSAssert1 (nsdir, @"no resourcePath for class %@", [self class]);
  const char *dir = [nsdir fileSystemRepresentation];
  const char *opath = getenv ("PATH");
  if (!opath) opath = "/bin"; // $PATH is unset when running under Shark!
  char *npath = (char *) calloc (strlen (opath) + strlen (dir) + 30, 1);
  strcat (npath, dir);
  strcat (npath, ":");
  strcat (npath, opath);
  unsetenv("PATH");
  if (setenv ("PATH", npath, 1)) {
    perror ("setenv");
    NSAssert1 (0, @"setenv \"%s\" failed", npath);
  }

  /* Don't free (npath) -- MacOS's putenv() does not copy it. */
  free(npath);
}


// set an $XSCREENSAVER_CLASSPATH variable so that included shell scripts
// (e.g., "xscreensaver-text") know how to look up resources.
//
- (void) setResourcesEnv:(NSString *) name
{
  NSBundle *nsb = [NSBundle bundleForClass:[self class]];
  NSAssert1 (nsb, @"no bundle for class %@", [self class]);
  
  const char *s = [name cStringUsingEncoding:NSUTF8StringEncoding];
  if (setenv ("XSCREENSAVER_CLASSPATH", s, 1)) {
    perror ("setenv");
    NSAssert1 (0, @"setenv \"%s\" failed", s);
  }
}


static void
add_default_options (const XrmOptionDescRec *opts,
                     const char * const *defs,
                     XrmOptionDescRec **opts_ret,
                     const char ***defs_ret)
{
  /* These aren't "real" command-line options (there are no actual command-line
     options in the Cocoa version); but this is the somewhat kludgey way that
     the <xscreensaver-text /> and <xscreensaver-image /> tags in the
     ../hacks/config/\*.xml files communicate with the preferences database.
  */
  static const XrmOptionDescRec default_options [] = {
    { "-text-mode",              ".textMode",          XrmoptionSepArg, 0 },
    { "-text-literal",           ".textLiteral",       XrmoptionSepArg, 0 },
    { "-text-file",              ".textFile",          XrmoptionSepArg, 0 },
    { "-text-url",               ".textURL",           XrmoptionSepArg, 0 },
    { "-text-program",           ".textProgram",       XrmoptionSepArg, 0 },
    { "-grab-desktop",           ".grabDesktopImages", XrmoptionNoArg, "True" },
    { "-no-grab-desktop",        ".grabDesktopImages", XrmoptionNoArg, "False"},
    { "-choose-random-images",   ".chooseRandomImages",XrmoptionNoArg, "True" },
    { "-no-choose-random-images",".chooseRandomImages",XrmoptionNoArg, "False"},
    { "-image-directory",        ".imageDirectory",    XrmoptionSepArg, 0 },
    { "-fps",                    ".doFPS",             XrmoptionNoArg, "True" },
    { "-no-fps",                 ".doFPS",             XrmoptionNoArg, "False"},
    { "-foreground",             ".foreground",        XrmoptionSepArg, 0 },
    { "-fg",                     ".foreground",        XrmoptionSepArg, 0 },
    { "-background",             ".background",        XrmoptionSepArg, 0 },
    { "-bg",                     ".background",        XrmoptionSepArg, 0 },

# ifndef USE_IPHONE
    // <xscreensaver-updater />
    {    "-" SUSUEnableAutomaticChecksKey,
         "." SUSUEnableAutomaticChecksKey, XrmoptionNoArg, "True"  },
    { "-no-" SUSUEnableAutomaticChecksKey,
         "." SUSUEnableAutomaticChecksKey, XrmoptionNoArg, "False" },
    {    "-" SUAutomaticallyUpdateKey,
         "." SUAutomaticallyUpdateKey, XrmoptionNoArg, "True"  },
    { "-no-" SUAutomaticallyUpdateKey,
         "." SUAutomaticallyUpdateKey, XrmoptionNoArg, "False" },
    {    "-" SUSendProfileInfoKey,
         "." SUSendProfileInfoKey, XrmoptionNoArg,"True" },
    { "-no-" SUSendProfileInfoKey,
         "." SUSendProfileInfoKey, XrmoptionNoArg,"False"},
    {    "-" SUScheduledCheckIntervalKey,
         "." SUScheduledCheckIntervalKey, XrmoptionSepArg, 0 },
# endif // !USE_IPHONE

    { 0, 0, 0, 0 }
  };
  static const char *default_defaults [] = {
    ".doFPS:              False",
    ".doubleBuffer:       True",
    ".multiSample:        False",
# ifndef USE_IPHONE
    ".textMode:           date",
# else
    ".textMode:           url",
# endif
 // ".textLiteral:        ",
 // ".textFile:           ",
    ".textURL:            http://en.wikipedia.org/w/index.php?title=Special:NewPages&feed=rss",
 // ".textProgram:        ",
    ".grabDesktopImages:  yes",
# ifndef USE_IPHONE
    ".chooseRandomImages: no",
# else
    ".chooseRandomImages: yes",
# endif
    ".imageDirectory:     ~/Pictures",
    ".relaunchDelay:      2",

# ifndef USE_IPHONE
#  define STR1(S) #S
#  define STR(S) STR1(S)
#  define __objc_yes Yes
#  define __objc_no  No
    "." SUSUEnableAutomaticChecksKey ": " STR(SUSUEnableAutomaticChecksDef),
    "." SUAutomaticallyUpdateKey ":  "    STR(SUAutomaticallyUpdateDef),
    "." SUSendProfileInfoKey ": "         STR(SUSendProfileInfoDef),
    "." SUScheduledCheckIntervalKey ": "  STR(SUScheduledCheckIntervalDef),
#  undef __objc_yes
#  undef __objc_no
#  undef STR1
#  undef STR
# endif // USE_IPHONE
    0
  };

  int count = 0, i, j;
  for (i = 0; default_options[i].option; i++)
    count++;
  for (i = 0; opts[i].option; i++)
    count++;

  XrmOptionDescRec *opts2 = (XrmOptionDescRec *) 
    calloc (count + 1, sizeof (*opts2));

  i = 0;
  j = 0;
  while (default_options[j].option) {
    opts2[i] = default_options[j];
    i++, j++;
  }
  j = 0;
  while (opts[j].option) {
    opts2[i] = opts[j];
    i++, j++;
  }

  *opts_ret = opts2;


  /* now the defaults
   */
  count = 0;
  for (i = 0; default_defaults[i]; i++)
    count++;
  for (i = 0; defs[i]; i++)
    count++;

  const char **defs2 = (const char **) calloc (count + 1, sizeof (*defs2));

  i = 0;
  j = 0;
  while (default_defaults[j]) {
    defs2[i] = default_defaults[j];
    i++, j++;
  }
  j = 0;
  while (defs[j]) {
    defs2[i] = defs[j];
    i++, j++;
  }

  *defs_ret = defs2;
}


#ifdef USE_IPHONE
/* Returns the current time in seconds as a double.
 */
static double
double_time (void)
{
  struct timeval now;
# ifdef GETTIMEOFDAY_TWO_ARGS
  struct timezone tzp;
  gettimeofday(&now, &tzp);
# else
  gettimeofday(&now);
# endif

  return (now.tv_sec + ((double) now.tv_usec * 0.000001));
}
#endif // USE_IPHONE


- (id) initWithFrame:(NSRect)frame
           saverName:(NSString *)saverName
           isPreview:(BOOL)isPreview
{
# ifdef USE_IPHONE
  initial_bounds = frame.size;
  rot_current_size = frame.size;	// needs to be early, because
  rot_from = rot_current_size;		// [self setFrame] is called by
  rot_to = rot_current_size;		// [super initWithFrame].
  rotation_ratio = -1;
# endif

  if (! (self = [super initWithFrame:frame isPreview:isPreview]))
    return nil;
  
  xsft = [self findFunctionTable: saverName];
  if (! xsft) {
    return nil;
  }

  [self setShellPath];

# ifdef USE_IPHONE
  [self setMultipleTouchEnabled:YES];
  orientation = UIDeviceOrientationUnknown;
  [self didRotate:nil];
# endif // USE_IPHONE

  setup_p = YES;
  if (xsft->setup_cb)
    xsft->setup_cb (xsft, xsft->setup_arg);

  /* The plist files for these preferences show up in
     $HOME/Library/Preferences/ByHost/ in a file named like
     "org.jwz.xscreensaver.<SAVERNAME>.<NUMBERS>.plist"
   */
  NSString *name = [NSString stringWithCString:xsft->progclass
                             encoding:NSISOLatin1StringEncoding];
  name = [@"org.jwz.xscreensaver." stringByAppendingString:name];
  [self setResourcesEnv:name];

  
  XrmOptionDescRec *opts = 0;
  const char **defs = 0;
  add_default_options (xsft->options, xsft->defaults, &opts, &defs);
  prefsReader = [[PrefsReader alloc]
                  initWithName:name xrmKeys:opts defaults:defs];
  free (defs);
  // free (opts);  // bah, we need these! #### leak!
  xsft->options = opts;
  
  progname = progclass = xsft->progclass;

  next_frame_time = 0;
  
# ifdef USE_BACKBUFFER
  [self createBackbuffer];
  [self initLayer];
# endif

# ifdef USE_IPHONE
  // So we can tell when we're docked.
  [UIDevice currentDevice].batteryMonitoringEnabled = YES;
# endif // USE_IPHONE

  return self;
}

- (void) initLayer
{
# if !defined(USE_IPHONE) && defined(USE_CALAYER)
  [self setLayer: [CALayer layer]];
  self.layer.delegate = self;
  self.layer.opaque = YES;
  [self setWantsLayer: YES];
# endif  // !USE_IPHONE && USE_CALAYER
}


- (id) initWithFrame:(NSRect)frame isPreview:(BOOL)p
{
  return [self initWithFrame:frame saverName:0 isPreview:p];
}


- (void) dealloc
{
  NSAssert(![self isAnimating], @"still animating");
  NSAssert(!xdata, @"xdata not yet freed");
  if (xdpy)
    jwxyz_free_display (xdpy);

# ifdef USE_BACKBUFFER
  if (backbuffer)
    CGContextRelease (backbuffer);

  if (colorspace)
    CGColorSpaceRelease (colorspace);

#  ifndef USE_CALAYER
  if (window_ctx)
    CGContextRelease (window_ctx);
#  endif // !USE_CALAYER

# endif // USE_BACKBUFFER

  // xsft
  // fpst
}

- (PrefsReader *) prefsReader
{
  return prefsReader;
}


#ifdef USE_IPHONE
- (void) lockFocus { }
- (void) unlockFocus { }
#endif // USE_IPHONE



# ifdef USE_IPHONE
/* A few seconds after the saver launches, we store the "wasRunning"
   preference.  This is so that if the saver is crashing at startup,
   we don't launch it again next time, getting stuck in a crash loop.
 */
- (void) allSystemsGo: (NSTimer *) timer
{
  NSAssert (timer == crash_timer, @"crash timer screwed up");
  crash_timer = 0;

  NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
  [prefs setBool:YES forKey:@"wasRunning"];
  [prefs synchronize];
}
#endif // USE_IPHONE


- (void) startAnimation
{
  NSAssert(![self isAnimating], @"already animating");
  NSAssert(!initted_p && !xdata, @"already initialized");
  [super startAnimation];
  /* We can't draw on the window from this method, so we actually do the
     initialization of the screen saver (xsft->init_cb) in the first call
     to animateOneFrame() instead.
   */

# ifdef USE_IPHONE
  if (crash_timer)
    [crash_timer invalidate];

  NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
  [prefs removeObjectForKey:@"wasRunning"];
  [prefs synchronize];

  crash_timer = [NSTimer scheduledTimerWithTimeInterval: 5
                         target:self
                         selector:@selector(allSystemsGo:)
                         userInfo:nil
                         repeats:NO];

# endif // USE_IPHONE

  // Never automatically turn the screen off if we are docked,
  // and an animation is running.
  //
# ifdef USE_IPHONE
  [UIApplication sharedApplication].idleTimerDisabled =
    ([UIDevice currentDevice].batteryState != UIDeviceBatteryStateUnplugged);
  [[UIApplication sharedApplication]
    setStatusBarHidden:YES withAnimation:UIStatusBarAnimationNone];
# endif
}


- (void)stopAnimation
{
  NSAssert([self isAnimating], @"not animating");

  if (initted_p) {

    [self lockFocus];       // in case something tries to draw from here
    [self prepareContext];

    /* I considered just not even calling the free callback at all...
       But webcollage-cocoa needs it, to kill the inferior webcollage
       processes (since the screen saver framework never generates a
       SIGPIPE for them...)  Instead, I turned off the free call in
       xlockmore.c, which is where all of the bogus calls are anyway.
     */
    xsft->free_cb (xdpy, xwindow, xdata);
    [self unlockFocus];

//  setup_p = NO; // #### wait, do we need this?
    initted_p = NO;
    xdata = 0;
  }

# ifdef USE_IPHONE
  if (crash_timer)
    [crash_timer invalidate];
  crash_timer = 0;
  NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
  [prefs removeObjectForKey:@"wasRunning"];
  [prefs synchronize];
# endif // USE_IPHONE

  [super stopAnimation];

  // When an animation is no longer running (e.g., looking at the list)
  // then it's ok to power off the screen when docked.
  //
# ifdef USE_IPHONE
  [UIApplication sharedApplication].idleTimerDisabled = NO;
  [[UIApplication sharedApplication]
    setStatusBarHidden:NO withAnimation:UIStatusBarAnimationNone];
# endif
}


/* Hook for the XScreenSaverGLView subclass
 */
- (void) prepareContext
{
}

/* Hook for the XScreenSaverGLView subclass
 */
- (void) resizeContext
{
}


static void
screenhack_do_fps (Display *dpy, Window w, fps_state *fpst, void *closure)
{
  fps_compute (fpst, 0, -1);
  fps_draw (fpst);
}


#ifdef USE_IPHONE

/* On iPhones with Retina displays, we can draw the savers in "real"
   pixels, and that works great.  The 320x480 "point" screen is really
   a 640x960 *pixel* screen.  However, Retina iPads have 768x1024
   point screens which are 1536x2048 pixels, and apparently that's
   enough pixels that copying those bits to the screen is slow.  Like,
   drops us from 15fps to 7fps.  So, on Retina iPads, we don't draw in
   real pixels.  This will probably make the savers look better
   anyway, since that's a higher resolution than most desktop monitors
   have even today.  (This is only true for X11 programs, not GL 
   programs.  Those are fine at full rez.)

   This method is overridden in XScreenSaverGLView, since this kludge
   isn't necessary for GL programs, being resolution independent by
   nature.
 */
- (CGFloat) hackedContentScaleFactor
{
  GLfloat s = [self contentScaleFactor];
  if (initial_bounds.width  >= 1024 ||
      initial_bounds.height >= 1024)
    s = 1;
  return s;
}


static GLfloat _global_rot_current_angle_kludge;

double current_device_rotation (void)
{
  return -_global_rot_current_angle_kludge;
}


- (void) hackRotation
{
  if (rotation_ratio >= 0) {	// in the midst of a rotation animation

#   define CLAMP180(N) while (N < 0) N += 360; while (N > 180) N -= 360
    GLfloat f = angle_from;
    GLfloat t = angle_to;
    CLAMP180(f);
    CLAMP180(t);
    GLfloat dist = -(t-f);
    CLAMP180(dist);

    // Intermediate angle.
    rot_current_angle = f - rotation_ratio * dist;

    // Intermediate frame size.
    rot_current_size.width = rot_from.width + 
      rotation_ratio * (rot_to.width - rot_from.width);
    rot_current_size.height = rot_from.height + 
      rotation_ratio * (rot_to.height - rot_from.height);

    // Tick animation.  Complete rotation in 1/6th sec.
    double now = double_time();
    double duration = 1/6.0;
    rotation_ratio = 1 - ((rot_start_time + duration - now) / duration);

    if (rotation_ratio > 1) {	// Done animating.
      orientation = new_orientation;
      rot_current_angle = angle_to;
      rot_current_size = rot_to;
      rotation_ratio = -1;

      // Check orientation again in case we rotated again while rotating:
      // this is a no-op if nothing has changed.
      [self didRotate:nil];
    }
  } else {			// Not animating a rotation.
    rot_current_angle = angle_to;
    rot_current_size = rot_to;
  }

  CLAMP180(rot_current_angle);
  _global_rot_current_angle_kludge = rot_current_angle;

#   undef CLAMP180

  double s = [self hackedContentScaleFactor];
  if (!ignore_rotation_p &&
      /* rotation_ratio && */
      ((int) backbuffer_size.width  != (int) (s * rot_current_size.width) ||
       (int) backbuffer_size.height != (int) (s * rot_current_size.height)))
    [self resize_x11];
}


- (void)alertView:(UIAlertView *)av clickedButtonAtIndex:(NSInteger)i
{
  if (i == 0) exit (-1);	// Cancel
  [self stopAndClose:NO];	// Keep going
}

- (void) handleException: (NSException *)e
{
  NSLog (@"Caught exception: %@", e);
  [[[UIAlertView alloc] initWithTitle:
                          [NSString stringWithFormat: @"%s crashed!",
                                    xsft->progclass]
                        message:
                          [NSString stringWithFormat:
                                      @"The error message was:"
                                    "\n\n%@\n\n"
                                    "If it keeps crashing, try "
                                    "resetting its options.",
                                    e]
                        delegate: self
                        cancelButtonTitle: @"Exit"
                        otherButtonTitles: @"Keep going", nil]
    show];
  [self stopAnimation];
}

#endif // USE_IPHONE


#ifdef USE_BACKBUFFER

/* Create a bitmap context into which we render everything.
   If the desired size has changed, re-created it.
 */
- (void) createBackbuffer
{
# ifdef USE_IPHONE
  double s = [self hackedContentScaleFactor];
  CGSize rotsize = ignore_rotation_p ? initial_bounds : rot_current_size;
  int new_w = s * rotsize.width;
  int new_h = s * rotsize.height;
# else
  int new_w = [self bounds].size.width;
  int new_h = [self bounds].size.height;
# endif
	
  // Colorspaces and CGContexts only happen with non-GL hacks.
  if (colorspace)
    CGColorSpaceRelease (colorspace);
# ifndef USE_CALAYER
  if (window_ctx)
    CGContextRelease (window_ctx);
# endif
	
  NSWindow *window = [self window];

  if (window && xdpy) {
    [self lockFocus];

# ifndef USE_CALAYER
    // TODO: This was borrowed from jwxyz_window_resized, and should
    // probably be refactored.
	  
    // Figure out which screen the window is currently on.
    CGDirectDisplayID cgdpy = 0;

    {
//    int wx, wy;
//    TODO: XTranslateCoordinates is returning (0,1200) on my system.
//    Is this right?
//    In any case, those weren't valid coordinates for CGGetDisplaysWithPoint.
//    XTranslateCoordinates (xdpy, xwindow, NULL, 0, 0, &wx, &wy, NULL);
//    p.x = wx;
//    p.y = wy;

      NSPoint p0 = {0, 0};
      p0 = [window convertBaseToScreen:p0];
      CGPoint p = {p0.x, p0.y};
      CGDisplayCount n;
      CGGetDisplaysWithPoint (p, 1, &cgdpy, &n);
      NSAssert (cgdpy, @"unable to find CGDisplay");
    }

    {
      // Figure out this screen's colorspace, and use that for every CGImage.
      //
      CMProfileRef profile = 0;

      // CMGetProfileByAVID is deprecated as of OS X 10.6, but there's no
      // documented replacement as of OS X 10.9.
      // http://lists.apple.com/archives/colorsync-dev/2012/Nov/msg00001.html
      CMGetProfileByAVID ((CMDisplayIDType) cgdpy, &profile);
      NSAssert (profile, @"unable to find colorspace profile");
      colorspace = CGColorSpaceCreateWithPlatformColorSpace (profile);
      NSAssert (colorspace, @"unable to find colorspace");
    }
# else  // USE_CALAYER
    // Was apparently faster until 10.9.
    colorspace = CGColorSpaceCreateDeviceRGB ();
# endif // USE_CALAYER

# ifndef USE_CALAYER
    window_ctx = [[window graphicsContext] graphicsPort];
    CGContextRetain (window_ctx);
# endif // !USE_CALAYER
	  
    [self unlockFocus];
  } else {
# ifndef USE_CALAYER
    window_ctx = NULL;
# endif // !USE_CALAYER
    colorspace = CGColorSpaceCreateDeviceRGB();
  }

  if (backbuffer &&
      backbuffer_size.width  == new_w &&
      backbuffer_size.height == new_h)
    return;

  CGSize osize = backbuffer_size;
  CGContextRef ob = backbuffer;

  backbuffer_size.width  = new_w;
  backbuffer_size.height = new_h;

  backbuffer = CGBitmapContextCreate (NULL,
                                      backbuffer_size.width,
                                      backbuffer_size.height,
                                      8, 
                                      backbuffer_size.width * 4,
                                      colorspace,
                                      // kCGImageAlphaPremultipliedLast
                                      (kCGImageAlphaNoneSkipFirst |
                                       kCGBitmapByteOrder32Host)
                                      );
  NSAssert (backbuffer, @"unable to allocate back buffer");

  // Clear it.
  CGRect r;
  r.origin.x = r.origin.y = 0;
  r.size = backbuffer_size;
  CGContextSetGrayFillColor (backbuffer, 0, 1);
  CGContextFillRect (backbuffer, r);

  if (ob) {
    // Restore old bits, as much as possible, to the X11 upper left origin.
    CGRect rect;
    rect.origin.x = 0;
    rect.origin.y = (backbuffer_size.height - osize.height);
    rect.size  = osize;
    CGImageRef img = CGBitmapContextCreateImage (ob);
    CGContextDrawImage (backbuffer, rect, img);
    CGImageRelease (img);
    CGContextRelease (ob);
  }
}

#endif // USE_BACKBUFFER


/* Inform X11 that the size of our window has changed.
 */
- (void) resize_x11
{
  if (!xwindow) return;  // early

# ifdef USE_BACKBUFFER
  [self createBackbuffer];
  jwxyz_window_resized (xdpy, xwindow,
                        0, 0,
                        backbuffer_size.width, backbuffer_size.height,
                        backbuffer);
# else   // !USE_BACKBUFFER
  NSRect r = [self frame];		// ignoring rotation is closer
  r.size = [self bounds].size;		// to what XGetGeometry expects.
  jwxyz_window_resized (xdpy, xwindow,
                        r.origin.x, r.origin.y,
                        r.size.width, r.size.height,
                        0);
# endif  // !USE_BACKBUFFER

  // Next time render_x11 is called, run the saver's reshape_cb.
  resized_p = YES;
}


- (void) render_x11
{
# ifdef USE_IPHONE
  @try {

  if (orientation == UIDeviceOrientationUnknown)
    [self didRotate:nil];
  [self hackRotation];
# endif

  if (!initted_p) {

    if (! xdpy) {
# ifdef USE_BACKBUFFER
      NSAssert (backbuffer, @"no back buffer");
      xdpy = jwxyz_make_display ((__bridge void *)(self), backbuffer);
# else
      xdpy = jwxyz_make_display (self, 0);
# endif
      xwindow = XRootWindow (xdpy, 0);

# ifdef USE_IPHONE
      /* Some X11 hacks (fluidballs) want to ignore all rotation events. */
      ignore_rotation_p =
        get_boolean_resource (xdpy, "ignoreRotation", "IgnoreRotation");
# endif // USE_IPHONE

      [self resize_x11];
    }

    if (!setup_p) {
      setup_p = YES;
      if (xsft->setup_cb)
        xsft->setup_cb (xsft, xsft->setup_arg);
    }
    initted_p = YES;
    resized_p = NO;
    NSAssert(!xdata, @"xdata already initialized");


# undef ya_rand_init
    ya_rand_init (0);
    
    XSetWindowBackground (xdpy, xwindow,
                          get_pixel_resource (xdpy, 0,
                                              "background", "Background"));
    XClearWindow (xdpy, xwindow);
    
# ifndef USE_IPHONE
    [[self window] setAcceptsMouseMovedEvents:YES];
# endif

    /* In MacOS 10.5, this enables "QuartzGL", meaning that the Quartz
       drawing primitives will run on the GPU instead of the CPU.
       It seems like it might make things worse rather than better,
       though...  Plus it makes us binary-incompatible with 10.4.

# if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
    [[self window] setPreferredBackingLocation:
                     NSWindowBackingLocationVideoMemory];
# endif
     */

    /* Kludge: even though the init_cb functions are declared to take 2 args,
      actually call them with 3, for the benefit of xlockmore_init() and
      xlockmore_setup().
      */
    void *(*init_cb) (Display *, Window, void *) = 
      (void *(*) (Display *, Window, void *)) xsft->init_cb;
    
    xdata = init_cb (xdpy, xwindow, xsft->setup_arg);

    if (get_boolean_resource (xdpy, "doFPS", "DoFPS")) {
      fpst = fps_init (xdpy, xwindow);
      if (! xsft->fps_cb) xsft->fps_cb = screenhack_do_fps;
    } else {
      xsft->fps_cb = 0;
    }

    [self checkForUpdates];
  }


  /* I don't understand why we have to do this *every frame*, but we do,
     or else the cursor comes back on.
   */
# ifndef USE_IPHONE
  if (![self isPreview])
    [NSCursor setHiddenUntilMouseMoves:YES];
# endif


  if (fpst)
    {
      /* This is just a guess, but the -fps code wants to know how long
         we were sleeping between frames.
       */
      long usecs = 1000000 * [self animationTimeInterval];
      usecs -= 200;  // caller apparently sleeps for slightly less sometimes...
      if (usecs < 0) usecs = 0;
      fps_slept (fpst, usecs);
    }


  /* It turns out that [ScreenSaverView setAnimationTimeInterval] does nothing.
     This is bad, because some of the screen hacks want to delay for long 
     periods (like 5 seconds or a minute!) between frames, and running them
     all at 60 FPS is no good.
  
     So, we don't use setAnimationTimeInterval, and just let the framework call
     us whenever.  But, we only invoke the screen hack's "draw frame" method
     when enough time has expired.
  
     This means two extra calls to gettimeofday() per frame.  For fast-cycling
     screen savers, that might actually slow them down.  Oh well.

     #### Also, we do not run the draw callback faster than the system's
          animationTimeInterval, so if any savers are pickier about timing
          than that, this may slow them down too much.  If that's a problem,
          then we could call draw_cb in a loop here (with usleep) until the
          next call would put us past animationTimeInterval...  But a better
          approach would probably be to just change the saver to not do that.
   */
  struct timeval tv;
  gettimeofday (&tv, 0);
  double now = tv.tv_sec + (tv.tv_usec / 1000000.0);
  if (now < next_frame_time) return;
  
  [self prepareContext];

  if (resized_p) {
    // We do this here instead of in setFrame so that all the
    // Xlib drawing takes place under the animation timer.
    [self resizeContext];
    NSRect r;
# ifndef USE_BACKBUFFER
    r = [self bounds];
# else  // USE_BACKBUFFER
    r.origin.x = 0;
    r.origin.y = 0;
    r.size.width  = backbuffer_size.width;
    r.size.height = backbuffer_size.height;
# endif // USE_BACKBUFFER

    xsft->reshape_cb (xdpy, xwindow, xdata, r.size.width, r.size.height);
    resized_p = NO;
  }

  // Run any XtAppAddInput callbacks now.
  // (Note that XtAppAddTimeOut callbacks have already been run by
  // the Cocoa event loop.)
  //
  jwxyz_sources_run (display_sources_data (xdpy));


  // And finally:
  //
# ifndef USE_IPHONE
  NSDisableScreenUpdates();
# endif
  unsigned long delay = xsft->draw_cb (xdpy, xwindow, xdata);
  if (fpst) xsft->fps_cb (xdpy, xwindow, fpst, xdata);
# ifndef USE_IPHONE
  NSEnableScreenUpdates();
# endif

  gettimeofday (&tv, 0);
  now = tv.tv_sec + (tv.tv_usec / 1000000.0);
  next_frame_time = now + (delay / 1000000.0);

# ifdef USE_IPHONE	// Allow savers on the iPhone to run full-tilt.
  if (delay < [self animationTimeInterval])
    [self setAnimationTimeInterval:(delay / 1000000.0)];
# endif

# ifdef USE_IPHONE
  }
  @catch (NSException *e) {
    [self handleException: e];
  }
# endif // USE_IPHONE
}


/* drawRect always does nothing, and animateOneFrame renders bits to the
   screen.  This is (now) true of both X11 and GL on both MacOS and iOS.
 */

- (void)drawRect:(NSRect)rect
{
  if (xwindow)    // clear to the X window's bg color, not necessarily black.
    XClearWindow (xdpy, xwindow);
  else
    [super drawRect:rect];    // early: black.
}


#ifndef USE_BACKBUFFER

- (void) animateOneFrame
{
  [self render_x11];
  jwxyz_flush_context(xdpy);
}

#else  // USE_BACKBUFFER

- (void) animateOneFrame
{
  // Render X11 into the backing store bitmap...

  NSAssert (backbuffer, @"no back buffer");

# ifdef USE_IPHONE
  UIGraphicsPushContext (backbuffer);
# endif

  [self render_x11];

# ifdef USE_IPHONE
  UIGraphicsPopContext();
# endif

# ifdef USE_IPHONE
  // Then compute the transformations for rotation.
  double hs = [self hackedContentScaleFactor];
  double s = [self contentScaleFactor];

  // The rotation origin for layer.affineTransform is in the center already.
  CGAffineTransform t = ignore_rotation_p ?
    CGAffineTransformIdentity :
    CGAffineTransformMakeRotation (rot_current_angle / (180.0 / M_PI));

  CGFloat f = s / hs;
  self.layer.affineTransform = CGAffineTransformScale(t, f, f);

  CGRect bounds;
  bounds.origin.x = 0;
  bounds.origin.y = 0;
  bounds.size.width = backbuffer_size.width / s;
  bounds.size.height = backbuffer_size.height / s;
  self.layer.bounds = bounds;
# endif // USE_IPHONE
 
# ifdef USE_CALAYER
  [self.layer setNeedsDisplay];
# else // !USE_CALAYER
  size_t
    w = CGBitmapContextGetWidth (backbuffer),
    h = CGBitmapContextGetHeight (backbuffer);
  
  size_t bpl = CGBitmapContextGetBytesPerRow (backbuffer);
  CGDataProviderRef prov = CGDataProviderCreateWithData (NULL,
                                            CGBitmapContextGetData(backbuffer),
                                                         bpl * h,
                                                         NULL);


  CGImageRef img = CGImageCreate (w, h,
                                  8, 32,
                                  CGBitmapContextGetBytesPerRow(backbuffer),
                                  colorspace,
                                  CGBitmapContextGetBitmapInfo(backbuffer),
                                  prov, NULL, NO,
                                  kCGRenderingIntentDefault);

  CGDataProviderRelease (prov);
  
  CGRect rect;
  rect.origin.x = 0;
  rect.origin.y = 0;
  rect.size = backbuffer_size;
  CGContextDrawImage (window_ctx, rect, img);
  
  CGImageRelease (img);

  CGContextFlush (window_ctx);
# endif // !USE_CALAYER
}

# ifdef USE_CALAYER

- (void) drawLayer:(CALayer *)layer inContext:(CGContextRef)ctx
{
  // This "isn't safe" if NULL is passed to CGBitmapCreateContext before iOS 4.
  char *dest_data = (char *)CGBitmapContextGetData (ctx);

  // The CGContext here is normally upside-down on iOS.
  if (dest_data &&
      CGBitmapContextGetBitmapInfo (ctx) ==
        (kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host)
#  ifdef USE_IPHONE
      && CGContextGetCTM (ctx).d < 0
#  endif // USE_IPHONE
      )
  {
    size_t dest_height = CGBitmapContextGetHeight (ctx);
    size_t dest_bpr = CGBitmapContextGetBytesPerRow (ctx);
    size_t src_height = CGBitmapContextGetHeight (backbuffer);
    size_t src_bpr = CGBitmapContextGetBytesPerRow (backbuffer);
    char *src_data = (char *)CGBitmapContextGetData (backbuffer);

    size_t height = src_height < dest_height ? src_height : dest_height;
    
    if (src_bpr == dest_bpr) {
      // iPad 1: 4.0 ms, iPad 2: 6.7 ms
      memcpy (dest_data, src_data, src_bpr * height);
    } else {
      // iPad 1: 4.6 ms, iPad 2: 7.2 ms
      size_t bpr = src_bpr < dest_bpr ? src_bpr : dest_bpr;
      while (height) {
        memcpy (dest_data, src_data, bpr);
        --height;
        src_data += src_bpr;
        dest_data += dest_bpr;
      }
    }
  } else {

    // iPad 1: 9.6 ms, iPad 2: 12.1 ms

#  ifdef USE_IPHONE
    CGContextScaleCTM (ctx, 1, -1);
    CGFloat s = [self contentScaleFactor];
    CGFloat hs = [self hackedContentScaleFactor];
    CGContextTranslateCTM (ctx, 0, -backbuffer_size.height * hs / s);
#  endif // USE_IPHONE
    
    CGImageRef img = CGBitmapContextCreateImage (backbuffer);
    CGContextDrawImage (ctx, self.layer.bounds, img);
    CGImageRelease (img);
  }
}
# endif  // USE_CALAYER

#endif // USE_BACKBUFFER



- (void) setFrame:(NSRect) newRect
{
  [super setFrame:newRect];

  if (xwindow)     // inform Xlib that the window has changed now.
    [self resize_x11];
}


# ifndef USE_IPHONE  // Doesn't exist on iOS
- (void) setFrameSize:(NSSize) newSize
{
  [super setFrameSize:newSize];
  if (xwindow)
    [self resize_x11];
}
# endif // !USE_IPHONE


+(BOOL) performGammaFade
{
  return YES;
}

- (BOOL) hasConfigureSheet
{
  return YES;
}

+ (NSString *) decompressXML: (NSData *)data
{
  if (! data) return 0;
  BOOL compressed_p = !!strncmp ((const char *) data.bytes, "<?xml", 5);

  // If it's not already XML, decompress it.
  NSAssert (compressed_p, @"xml isn't compressed");
  if (compressed_p) {
    NSMutableData *data2 = 0;
    int ret = -1;
    z_stream zs;
    memset (&zs, 0, sizeof(zs));
    ret = inflateInit2 (&zs, 16 + MAX_WBITS);
    if (ret == Z_OK) {
      UInt32 usize = * (UInt32 *) (data.bytes + data.length - 4);
      data2 = [NSMutableData dataWithLength: usize];
      zs.next_in   = (Bytef *) data.bytes;
      zs.avail_in  = (unsigned int)data.length;
      zs.next_out  = (Bytef *) data2.bytes;
      zs.avail_out = (unsigned int)data2.length;
      ret = inflate (&zs, Z_FINISH);
      inflateEnd (&zs);
    }
    if (ret == Z_OK || ret == Z_STREAM_END)
      data = data2;
    else
      NSAssert2 (0, @"gunzip error: %d: %s",
                 ret, (zs.msg ? zs.msg : "<null>"));
  }

  return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}


#ifndef USE_IPHONE
- (NSWindow *) configureSheet
#else
- (UIViewController *) configureView
#endif
{
  NSBundle *bundle = [NSBundle bundleForClass:[self class]];
  NSString *file = [NSString stringWithCString:xsft->progclass
                                      encoding:NSISOLatin1StringEncoding];
  file = [file lowercaseString];
  NSString *path = [bundle pathForResource:file ofType:@"xml"];
  if (!path) {
    NSLog (@"%@.xml does not exist in the application bundle: %@/",
           file, [bundle resourcePath]);
    return nil;
  }
  
# ifdef USE_IPHONE
  UIViewController *sheet;
# else  // !USE_IPHONE
  NSWindow *sheet;
# endif // !USE_IPHONE

  NSData *xmld = [NSData dataWithContentsOfFile:path];
  NSString *xml = [[self class] decompressXML: xmld];
  sheet = [[XScreenSaverConfigSheet alloc]
            initWithXML:[xml dataUsingEncoding:NSUTF8StringEncoding]
                options:xsft->options
             controller:[prefsReader userDefaultsController]
       globalController:[prefsReader globalDefaultsController]
               defaults:[prefsReader defaultOptions]];

  // #### am I expected to retain this, or not? wtf.
  //      I thought not, but if I don't do this, we (sometimes) crash.
  // #### Analyze says "potential leak of an object stored into sheet"

  return sheet;
}


- (NSUserDefaultsController *) userDefaultsController
{
  return [prefsReader userDefaultsController];
}


/* Announce our willingness to accept keyboard input.
 */
- (BOOL)acceptsFirstResponder
{
  return YES;
}


#ifndef USE_IPHONE

/* Convert an NSEvent into an XEvent, and pass it along.
   Returns YES if it was handled.
 */
- (BOOL) doEvent: (NSEvent *) e
            type: (int) type
{
  if (![self isPreview] ||     // no event handling if actually screen-saving!
      ![self isAnimating] ||
      !initted_p)
    return NO;

  XEvent xe;
  memset (&xe, 0, sizeof(xe));
  
  int state = 0;
  
  NSUInteger flags = [e modifierFlags];
  if (flags & NSAlphaShiftKeyMask) state |= LockMask;
  if (flags & NSShiftKeyMask)      state |= ShiftMask;
  if (flags & NSControlKeyMask)    state |= ControlMask;
  if (flags & NSAlternateKeyMask)  state |= Mod1Mask;
  if (flags & NSCommandKeyMask)    state |= Mod2Mask;
  
  NSPoint p = [[[e window] contentView] convertPoint:[e locationInWindow]
                                            toView:self];
# ifdef USE_IPHONE
  double s = [self hackedContentScaleFactor];
# else
  int s = 1;
# endif
  int x = s * p.x;
  int y = s * ([self bounds].size.height - p.y);

  xe.xany.type = type;
  switch (type) {
    case ButtonPress:
    case ButtonRelease:
      xe.xbutton.x = x;
      xe.xbutton.y = y;
      xe.xbutton.state = state;
      if ([e type] == NSScrollWheel)
        xe.xbutton.button = ([e deltaY] > 0 ? Button4 :
                             [e deltaY] < 0 ? Button5 :
                             [e deltaX] > 0 ? Button6 :
                             [e deltaX] < 0 ? Button7 :
                             0);
      else
        xe.xbutton.button = (unsigned int)([e buttonNumber] + 1);
      break;
    case MotionNotify:
      xe.xmotion.x = x;
      xe.xmotion.y = y;
      xe.xmotion.state = state;
      break;
    case KeyPress:
    case KeyRelease:
      {
        NSString *ns = (([e type] == NSFlagsChanged) ? 0 :
                        [e charactersIgnoringModifiers]);
        KeySym k = 0;

        if (!ns || [ns length] == 0)			// dead key
          {
            // Cocoa hides the difference between left and right keys.
            // Also we only get KeyPress events for these, no KeyRelease
            // (unless we hack the mod state manually.  Bleh.)
            //
            if      (flags & NSAlphaShiftKeyMask)   k = XK_Caps_Lock;
            else if (flags & NSShiftKeyMask)        k = XK_Shift_L;
            else if (flags & NSControlKeyMask)      k = XK_Control_L;
            else if (flags & NSAlternateKeyMask)    k = XK_Alt_L;
            else if (flags & NSCommandKeyMask)      k = XK_Meta_L;
          }
        else if ([ns length] == 1)			// real key
          {
            switch ([ns characterAtIndex:0]) {
            case NSLeftArrowFunctionKey:  k = XK_Left;      break;
            case NSRightArrowFunctionKey: k = XK_Right;     break;
            case NSUpArrowFunctionKey:    k = XK_Up;        break;
            case NSDownArrowFunctionKey:  k = XK_Down;      break;
            case NSPageUpFunctionKey:     k = XK_Page_Up;   break;
            case NSPageDownFunctionKey:   k = XK_Page_Down; break;
            case NSHomeFunctionKey:       k = XK_Home;      break;
            case NSPrevFunctionKey:       k = XK_Prior;     break;
            case NSNextFunctionKey:       k = XK_Next;      break;
            case NSBeginFunctionKey:      k = XK_Begin;     break;
            case NSEndFunctionKey:        k = XK_End;       break;
            default:
              {
                const char *s =
                  [ns cStringUsingEncoding:NSISOLatin1StringEncoding];
                k = (s && *s ? *s : 0);
              }
              break;
            }
          }

        if (! k) return YES;   // E.g., "KeyRelease XK_Shift_L"

        xe.xkey.keycode = k;
        xe.xkey.state = state;
        break;
      }
    default:
      NSAssert1 (0, @"unknown X11 event type: %d", type);
      break;
  }

  [self lockFocus];
  [self prepareContext];
  BOOL result = xsft->event_cb (xdpy, xwindow, xdata, &xe);
  [self unlockFocus];
  return result;
}


- (void) mouseDown: (NSEvent *) e
{
  if (! [self doEvent:e type:ButtonPress])
    [super mouseDown:e];
}

- (void) mouseUp: (NSEvent *) e
{
  if (! [self doEvent:e type:ButtonRelease])
    [super mouseUp:e];
}

- (void) otherMouseDown: (NSEvent *) e
{
  if (! [self doEvent:e type:ButtonPress])
    [super otherMouseDown:e];
}

- (void) otherMouseUp: (NSEvent *) e
{
  if (! [self doEvent:e type:ButtonRelease])
    [super otherMouseUp:e];
}

- (void) mouseMoved: (NSEvent *) e
{
  if (! [self doEvent:e type:MotionNotify])
    [super mouseMoved:e];
}

- (void) mouseDragged: (NSEvent *) e
{
  if (! [self doEvent:e type:MotionNotify])
    [super mouseDragged:e];
}

- (void) otherMouseDragged: (NSEvent *) e
{
  if (! [self doEvent:e type:MotionNotify])
    [super otherMouseDragged:e];
}

- (void) scrollWheel: (NSEvent *) e
{
  if (! [self doEvent:e type:ButtonPress])
    [super scrollWheel:e];
}

- (void) keyDown: (NSEvent *) e
{
  if (! [self doEvent:e type:KeyPress])
    [super keyDown:e];
}

- (void) keyUp: (NSEvent *) e
{
  if (! [self doEvent:e type:KeyRelease])
    [super keyUp:e];
}

- (void) flagsChanged: (NSEvent *) e
{
  if (! [self doEvent:e type:KeyPress])
    [super flagsChanged:e];
}

#else  // USE_IPHONE


- (void) stopAndClose:(Bool)relaunch_p
{
  if ([self isAnimating])
    [self stopAnimation];

  /* Need to make the SaverListController be the firstResponder again
     so that it can continue to receive its own shake events.  I
     suppose that this abstraction-breakage means that I'm adding
     XScreenSaverView to the UINavigationController wrong...
   */
  UIViewController *v = [[self window] rootViewController];
  if ([v isKindOfClass: [UINavigationController class]]) {
    UINavigationController *n = (UINavigationController *) v;
    [[n topViewController] becomeFirstResponder];
  }

  UIView *fader = [self superview];  // the "backgroundView" view is our parent

  if (relaunch_p) {   // Fake a shake on the SaverListController.
    // Why is [self window] sometimes null here?
    UIWindow *w = [[UIApplication sharedApplication] keyWindow];
    UIViewController *v = [w rootViewController];
    if ([v isKindOfClass: [UINavigationController class]]) {
      UINavigationController *n = (UINavigationController *) v;
      [[n topViewController] motionEnded: UIEventSubtypeMotionShake
                               withEvent: nil];
    }
  } else {	// Not launching another, animate our return to the list.
    [UIView animateWithDuration: 0.5
            animations:^{ fader.alpha = 0.0; }
            completion:^(BOOL finished) {
               [fader removeFromSuperview];
               fader.alpha = 1.0;
            }];
  }
}


/* Called after the device's orientation has changed.

   Note: we could include a subclass of UIViewController which
   contains a shouldAutorotateToInterfaceOrientation method that
   returns YES, in which case Core Animation would auto-rotate our
   View for us in response to rotation events... but, that interacts
   badly with the EAGLContext -- if you introduce Core Animation into
   the path, the OpenGL pipeline probably falls back on software
   rendering and performance goes to hell.  Also, the scaling and
   rotation that Core Animation does interacts incorrectly with the GL
   context anyway.

   So, we have to hack the rotation animation manually, in the GL world.

   Possibly XScreenSaverView should use Core Animation, and 
   XScreenSaverGLView should override that.
 */
- (void)didRotate:(NSNotification *)notification
{
  UIDeviceOrientation current = [[UIDevice currentDevice] orientation];

  /* If the simulator starts up in the rotated position, sometimes
     the UIDevice says we're in Portrait when we're not -- but it
     turns out that the UINavigationController knows what's up!
     So get it from there.
   */
  if (current == UIDeviceOrientationUnknown) {
    switch ([[[self window] rootViewController] interfaceOrientation]) {
    case UIInterfaceOrientationPortrait:
      current = UIDeviceOrientationPortrait;
      break;
    case UIInterfaceOrientationPortraitUpsideDown:
      current = UIDeviceOrientationPortraitUpsideDown;
      break;
    case UIInterfaceOrientationLandscapeLeft:		// It's opposite day
      current = UIDeviceOrientationLandscapeRight;
      break;
    case UIInterfaceOrientationLandscapeRight:
      current = UIDeviceOrientationLandscapeLeft;
      break;
    default:
      break;
    }
  }

  /* On the iPad (but not iPhone 3GS, or the simulator) sometimes we get
     an orientation change event with an unknown orientation.  Those seem
     to always be immediately followed by another orientation change with
     a *real* orientation change, so let's try just ignoring those bogus
     ones and hoping that the real one comes in shortly...
   */
  if (current == UIDeviceOrientationUnknown)
    return;

  if (rotation_ratio >= 0) return;	// in the midst of rotation animation
  if (orientation == current) return;	// no change

  // When transitioning to FaceUp or FaceDown, pretend there was no change.
  if (current == UIDeviceOrientationFaceUp ||
      current == UIDeviceOrientationFaceDown)
    return;

  new_orientation = current;		// current animation target
  rotation_ratio = 0;			// start animating
  rot_start_time = double_time();

  switch (orientation) {
  case UIDeviceOrientationLandscapeLeft:      angle_from = 90;  break;
  case UIDeviceOrientationLandscapeRight:     angle_from = 270; break;
  case UIDeviceOrientationPortraitUpsideDown: angle_from = 180; break;
  default:                                    angle_from = 0;   break;
  }

  switch (new_orientation) {
  case UIDeviceOrientationLandscapeLeft:      angle_to = 90;  break;
  case UIDeviceOrientationLandscapeRight:     angle_to = 270; break;
  case UIDeviceOrientationPortraitUpsideDown: angle_to = 180; break;
  default:                                    angle_to = 0;   break;
  }

  switch (orientation) {
  case UIDeviceOrientationLandscapeRight:	// from landscape
  case UIDeviceOrientationLandscapeLeft:
    rot_from.width  = initial_bounds.height;
    rot_from.height = initial_bounds.width;
    break;
  default:					// from portrait
    rot_from.width  = initial_bounds.width;
    rot_from.height = initial_bounds.height;
    break;
  }

  switch (new_orientation) {
  case UIDeviceOrientationLandscapeRight:	// to landscape
  case UIDeviceOrientationLandscapeLeft:
    rot_to.width  = initial_bounds.height;
    rot_to.height = initial_bounds.width;
    break;
  default:					// to portrait
    rot_to.width  = initial_bounds.width;
    rot_to.height = initial_bounds.height;
    break;
  }

 if (! initted_p) {
   // If we've done a rotation but the saver hasn't been initialized yet,
   // don't bother going through an X11 resize, but just do it now.
   rot_start_time = 0;  // dawn of time
   [self hackRotation];
 }
}


/* I believe we can't use UIGestureRecognizer for tracking touches
   because UIPanGestureRecognizer doesn't give us enough detail in its
   callbacks.

   Currently we don't handle multi-touches (just the first touch) but
   I'm leaving this comment here for future reference:

   In the simulator, multi-touch sequences look like this:

     touchesBegan [touchA, touchB]
     touchesEnd [touchA, touchB]

   But on real devices, sometimes you get that, but sometimes you get:

     touchesBegan [touchA, touchB]
     touchesEnd [touchB]
     touchesEnd [touchA]

   Or even

     touchesBegan [touchA]
     touchesBegan [touchB]
     touchesEnd [touchA]
     touchesEnd [touchB]

   So the only way to properly detect a "pinch" gesture is to remember
   the start-point of each touch as it comes in; and the end-point of
   each touch as those come in; and only process the gesture once the
   number of touchEnds matches the number of touchBegins.
 */

- (void) rotateMouse:(int)rot x:(int*)x y:(int *)y w:(int)w h:(int)h
{
  // This is a no-op unless contentScaleFactor != hackedContentScaleFactor.
  // Currently, this is the iPad Retina only.
  CGRect frame = [self bounds];		// Scale.
  double s = [self hackedContentScaleFactor];
  *x *= (backbuffer_size.width  / frame.size.width)  / s;
  *y *= (backbuffer_size.height / frame.size.height) / s;
}


#if 0  // AudioToolbox/AudioToolbox.h
- (void) beep
{
  // There's no way to play a standard system alert sound!
  // We'd have to include our own WAV for that.  Eh, fuck it.
  AudioServicesPlaySystemSound (kSystemSoundID_Vibrate);
# if TARGET_IPHONE_SIMULATOR
  NSLog(@"BEEP");  // The sim doesn't vibrate.
# endif
}
#endif


/* We distinguish between taps and drags.
   - Drags (down, motion, up) are sent to the saver to handle.
   - Single-taps exit the saver.
   This means a saver cannot respond to a single-tap.  Only a few try to.
 */

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  // If they are trying to pinch, just do nothing.
  if ([[event allTouches] count] > 1)
    return;

  tap_time = 0;

  if (xsft->event_cb && xwindow) {
    double s = [self hackedContentScaleFactor];
    XEvent xe;
    memset (&xe, 0, sizeof(xe));
    int i = 0;
    // #### 'frame' here or 'bounds'?
    int w = s * [self frame].size.width;
    int h = s * [self frame].size.height;
    for (UITouch *touch in touches) {
      CGPoint p = [touch locationInView:self];
      xe.xany.type = ButtonPress;
      xe.xbutton.button = i + 1;
      xe.xbutton.button = i + 1;
      xe.xbutton.x      = s * p.x;
      xe.xbutton.y      = s * p.y;
      [self rotateMouse: rot_current_angle
            x: &xe.xbutton.x y: &xe.xbutton.y w: w h: h];
      jwxyz_mouse_moved (xdpy, xwindow, xe.xbutton.x, xe.xbutton.y);

      // Ignore return code: don't care whether the hack handled it.
      xsft->event_cb (xdpy, xwindow, xdata, &xe);

      // Remember when/where this was, to determine tap versus drag or hold.
      tap_time = double_time();
      tap_point = p;

      i++;
      break;  // No pinches: only look at the first touch.
    }
  }
}


- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  // If they are trying to pinch, just do nothing.
  if ([[event allTouches] count] > 1)
    return;

  if (xsft->event_cb && xwindow) {
    double s = [self hackedContentScaleFactor];
    XEvent xe;
    memset (&xe, 0, sizeof(xe));
    int i = 0;
    // #### 'frame' here or 'bounds'?
    int w = s * [self frame].size.width;
    int h = s * [self frame].size.height;
    for (UITouch *touch in touches) {
      CGPoint p = [touch locationInView:self];

      // If the ButtonRelease came less than half a second after ButtonPress,
      // and didn't move far, then this was a tap, not a drag or a hold.
      // Interpret it as "exit".
      //
      double dist = sqrt (((p.x - tap_point.x) * (p.x - tap_point.x)) +
                          ((p.y - tap_point.y) * (p.y - tap_point.y)));
      if (tap_time + 0.5 >= double_time() && dist < 20) {
        [self stopAndClose:NO];
        return;
      }

      xe.xany.type      = ButtonRelease;
      xe.xbutton.button = i + 1;
      xe.xbutton.x      = s * p.x;
      xe.xbutton.y      = s * p.y;
      [self rotateMouse: rot_current_angle
            x: &xe.xbutton.x y: &xe.xbutton.y w: w h: h];
      jwxyz_mouse_moved (xdpy, xwindow, xe.xbutton.x, xe.xbutton.y);
      xsft->event_cb (xdpy, xwindow, xdata, &xe);
      i++;
      break;  // No pinches: only look at the first touch.
    }
  }
}


- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  // If they are trying to pinch, just do nothing.
  if ([[event allTouches] count] > 1)
    return;

  if (xsft->event_cb && xwindow) {
    double s = [self hackedContentScaleFactor];
    XEvent xe;
    memset (&xe, 0, sizeof(xe));
    int i = 0;
    // #### 'frame' here or 'bounds'?
    int w = s * [self frame].size.width;
    int h = s * [self frame].size.height;
    for (UITouch *touch in touches) {
      CGPoint p = [touch locationInView:self];
      xe.xany.type      = MotionNotify;
      xe.xmotion.x      = s * p.x;
      xe.xmotion.y      = s * p.y;
      [self rotateMouse: rot_current_angle
            x: &xe.xbutton.x y: &xe.xbutton.y w: w h: h];
      jwxyz_mouse_moved (xdpy, xwindow, xe.xmotion.x, xe.xmotion.y);
      xsft->event_cb (xdpy, xwindow, xdata, &xe);
      i++;
      break;  // No pinches: only look at the first touch.
    }
  }
}


/* We need this to respond to "shake" gestures
 */
- (BOOL)canBecomeFirstResponder
{
  return YES;
}

- (void)motionBegan:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
}


- (void)motionCancelled:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
}

/* Shake means exit and launch a new saver.
 */
- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
  [self stopAndClose:YES];
}


- (void)setScreenLocked:(BOOL)locked
{
  if (screenLocked == locked) return;
  screenLocked = locked;
  if (locked) {
    if ([self isAnimating])
      [self stopAnimation];
  } else {
    if (! [self isAnimating])
      [self startAnimation];
  }
}

#endif // USE_IPHONE


- (void) checkForUpdates
{
# ifndef USE_IPHONE
  // We only check once at startup, even if there are multiple screens,
  // and even if this saver is running for many days.
  // (Uh, except this doesn't work because this static isn't shared,
  // even if we make it an exported global. Not sure why. Oh well.)
  static BOOL checked_p = NO;
  if (checked_p) return;
  checked_p = YES;

  // If it's off, don't bother running the updater.  Otherwise, the
  // updater will decide if it's time to hit the network.
  if (! get_boolean_resource (xdpy,
                              SUSUEnableAutomaticChecksKey,
                              SUSUEnableAutomaticChecksKey))
    return;

  NSString *updater = @"XScreenSaverUpdater.app";

  // There may be multiple copies of the updater: e.g., one in /Applications
  // and one in the mounted installer DMG!  It's important that we run the
  // one from the disk and not the DMG, so search for the right one.
  //
  NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
  NSBundle *bundle = [NSBundle bundleForClass:[self class]];
  NSArray *search =
    @[[[bundle bundlePath] stringByDeletingLastPathComponent],
      [@"~/Library/Screen Savers" stringByExpandingTildeInPath],
      @"/Library/Screen Savers",
      @"/System/Library/Screen Savers",
      @"/Applications",
      @"/Applications/Utilities"];
  NSString *app_path = nil;
  for (NSString *dir in search) {
    NSString *p = [dir stringByAppendingPathComponent:updater];
    if ([[NSFileManager defaultManager] fileExistsAtPath:p]) {
      app_path = p;
      break;
    }
  }

  if (! app_path)
    app_path = [workspace fullPathForApplication:updater];

  if (app_path && [app_path hasPrefix:@"/Volumes/XScreenSaver "])
    app_path = 0;  // The DMG version will not do.

  if (!app_path) {
    NSLog(@"Unable to find %@", updater);
    return;
  }

  NSError *err = nil;
  if (! [workspace launchApplicationAtURL:[NSURL fileURLWithPath:app_path]
                   options:(NSWorkspaceLaunchWithoutAddingToRecents |
                            NSWorkspaceLaunchWithoutActivation |
                            NSWorkspaceLaunchAndHide)
                   configuration:nil
                   error:&err]) {
    NSLog(@"Unable to launch %@: %@", app_path, err);
  }

# endif // !USE_IPHONE
}


@end

/* Utility functions...
 */

static PrefsReader *
get_prefsReader (Display *dpy)
{
  XScreenSaverView *view = (__bridge XScreenSaverView *)(jwxyz_window_view (XRootWindow (dpy, 0)));
  if (!view) return 0;
  return [view prefsReader];
}


char *
get_string_resource (Display *dpy, char *name, char *class)
{
  return [get_prefsReader(dpy) getStringResource:name];
}

Bool
get_boolean_resource (Display *dpy, char *name, char *class)
{
  return [get_prefsReader(dpy) getBooleanResource:name];
}

int
get_integer_resource (Display *dpy, char *name, char *class)
{
  return [get_prefsReader(dpy) getIntegerResource:name];
}

double
get_float_resource (Display *dpy, char *name, char *class)
{
  return [get_prefsReader(dpy) getFloatResource:name];
}
