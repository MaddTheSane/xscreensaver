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

/* This implements the substrate of the xscreensaver preferences code:
   It does this by writing defaults to, and reading values from, the
   NSUserDefaultsController (and ScreenSaverDefaults/NSUserDefaults)
   and thereby reading the preferences that may have been edited by
   the UI (XScreenSaverConfigSheet).
 */

#ifndef USE_IPHONE
# import <ScreenSaver/ScreenSaverDefaults.h>
#endif

#import "PrefsReader.h"
#import "Updater.h"
#import "screenhackI.h"

#ifndef USE_IPHONE


/* GlobalDefaults is an NSUserDefaults implementation that writes into
   the preferences key we provide, instead of whatever the default would
   be for this app.  We do this by invoking the Core Foundation preferences
   routines directly, while presenting the same API as NSUserDefaults.

   We need this so that global prefs will go into the file
   Library/Preferences/org.jwz.xscreensaver.updater.plist instead of into
   Library/Preferences/ByHost/org.jwz.xscreensaver.Maze.XXXXX.plist
   with the per-saver prefs.

   The ScreenSaverDefaults class *almost* does this, but it always writes
   into the ByHost subdirectory, which means it's not readable by an app
   that tries to access it with a plain old +standardUserDefaults.
 */
@interface GlobalDefaults : NSUserDefaults
@property (strong) NSString *domain;
@property (strong) NSDictionary *defaults;
@end

@implementation GlobalDefaults
@synthesize domain;
@synthesize defaults;

- (id) initWithDomain:(NSString *)_domain
{
  if (self = [super init]) {
    self.domain = _domain;
  }
  return self;
}


- (void)registerDefaults:(NSDictionary *)dict
{
  self.defaults = dict;
}

- (id)objectForKey:(NSString *)key
{
  NSObject *obj =
    CFBridgingRelease(CFPreferencesCopyAppValue ((__bridge CFStringRef) key, (__bridge CFStringRef) domain));
  if (!obj && defaults)
    obj = defaults[key];
  return obj;
}

- (void)setObject:(id)value forKey:(NSString *)key
{
  if (value && defaults) {
    // If the value is the default, then remove it instead.
    NSObject *def = defaults[key];
    if (def && [def isEqual:value])
      value = NULL;
  }
  CFPreferencesSetAppValue ((__bridge CFStringRef) key,
                            (__bridge CFPropertyListRef) value,
                            (__bridge CFStringRef) domain);
}


- (BOOL)synchronize
{
  return CFPreferencesAppSynchronize ((__bridge CFStringRef) domain);
}


// Make sure these all call our objectForKey.
// Might not be necessary, but safe.

- (NSString *)stringForKey:(NSString *)key
{
  return [[self objectForKey:key] stringValue];
}

- (NSArray *)arrayForKey:(NSString *)key
{
  return (NSArray *) [self objectForKey:key];
}

- (NSDictionary *)dictionaryForKey:(NSString *)key
{
  return (NSDictionary *) [self objectForKey:key];
}

- (NSData *)dataForKey:(NSString *)key
{
  return (NSData *) [self objectForKey:key];
}

- (NSArray *)stringArrayForKey:(NSString *)key
{
  return (NSArray *) [self objectForKey:key];
}

- (NSInteger)integerForKey:(NSString *)key
{
  return [[self objectForKey:key] integerValue];
}

- (float)floatForKey:(NSString *)key
{
  return [[self objectForKey:key] floatValue];
}

- (double)doubleForKey:(NSString *)key
{
  return [[self objectForKey:key] doubleValue];
}

- (BOOL)boolForKey:(NSString *)key
{
  return [[self objectForKey:key] integerValue];
}

// Make sure these all call our setObject.
// Might not be necessary, but safe.

- (void)removeObjectForKey:(NSString *)key
{
  [self setObject:NULL forKey:key];
}

- (void)setInteger:(NSInteger)value forKey:(NSString *)key
{
  [self setObject:@(value) forKey:key];
}

- (void)setFloat:(float)value forKey:(NSString *)key
{
  [self setObject:@(value) forKey:key];
}

- (void)setDouble:(double)value forKey:(NSString *)key
{
  [self setObject:@(value) forKey:key];
}

- (void)setBool:(BOOL)value forKey:(NSString *)key
{
  [self setObject:@(value) forKey:key];
}
@end


#endif // !USE_IPHONE


@implementation PrefsReader

/* Normally we read resources by looking up "KEY" in the database
   "org.jwz.xscreensaver.SAVERNAME".  But in the all-in-one iPhone
   app, everything is stored in the database "org.jwz.xscreensaver"
   instead, so transform keys to "SAVERNAME.KEY".

   NOTE: This is duplicated in XScreenSaverConfigSheet.m, cause I suck.
 */
- (NSString *) makeKey:(NSString *)key
{
# ifdef USE_IPHONE
  NSString *prefix = [saver_name stringByAppendingString:@"."];
  if (! [key hasPrefix:prefix])  // Don't double up!
    key = [prefix stringByAppendingString:key];
# endif
  return key;
}

- (NSString *) makeCKey:(const char *)key
{
  return [self makeKey:@(key)];
}


/* Converts an array of "key:value" strings to an NSDictionary.
 */
- (NSDictionary *) defaultsToDict: (const char * const *) defs
{
  NSDictionary *dict = [NSMutableDictionary dictionaryWithCapacity:20];
  while (*defs) {
    char *line = strdup (*defs);
    char *key, *val;
    key = line;
    while (*key == '.' || *key == '*' || *key == ' ' || *key == '\t')
      key++;
    val = key;
    while (*val && *val != ':')
      val++;
    if (*val != ':') abort();
    *val++ = 0;
    while (*val == ' ' || *val == '\t')
      val++;

    size_t L = strlen(val);
    while (L > 0 && (val[L-1] == ' ' || val[L-1] == '\t'))
      val[--L] = 0;

    // When storing into preferences, look at the default string and
    // decide whether it's a boolean, int, float, or string, and store
    // an object of the appropriate type in the prefs.
    //
    NSString *nskey = [self makeCKey:key];
    NSObject *nsval;
    int dd;
    double ff;
    char cc;
    if (!strcasecmp (val, "true") || !strcasecmp (val, "yes"))
      nsval = @YES;
    else if (!strcasecmp (val, "false") || !strcasecmp (val, "no"))
      nsval = @NO;
    else if (1 == sscanf (val, " %d %c", &dd, &cc))
      nsval = @(dd);
    else if (1 == sscanf (val, " %lf %c", &ff, &cc))
      nsval = @(ff);
    else
      nsval = @(val);
      
//    NSLog (@"default: \"%@\" = \"%@\" [%@]", nskey, nsval, [nsval class]);
    [dict setValue:nsval forKey:nskey];
    free (line);
    defs++;
  }
  return dict;
}


/* Initialize the Cocoa preferences database:
   - sets the default preferences values from the 'defaults' array;
   - binds 'self' to each preference as an observer;
   - ensures that nothing is mentioned in 'options' and not in 'defaults';
   - ensures that nothing is mentioned in 'defaults' and not in 'options'.
 */
- (void) registerXrmKeys: (const XrmOptionDescRec *) opts
                defaults: (const char * const *) defs
{
  // Store the contents of 'defaults' into the real preferences database.
  NSDictionary *defsdict = [self defaultsToDict:defs];
  [userDefaults registerDefaults:defsdict];
  [globalDefaults registerDefaults:UPDATER_DEFAULTS];

  // Save a copy of the default options, since iOS doesn't have
  // [userDefaultsController initialValues].
  //
  defaultOptions = [NSMutableDictionary dictionaryWithCapacity:20];
  for (NSString *key in defsdict) {
    [defaultOptions setValue:defsdict[key] forKey:key];
  }

# ifndef USE_IPHONE
  userDefaultsController = 
    [[NSUserDefaultsController alloc] initWithDefaults:userDefaults
                                      initialValues:defsdict];
  globalDefaultsController = 
    [[NSUserDefaultsController alloc] initWithDefaults:globalDefaults
                                      initialValues:defsdict];
# else  // USE_IPHONE
  userDefaultsController   = userDefaults;
  globalDefaultsController = userDefaults;
# endif // USE_IPHONE

  NSDictionary *optsdict = [NSMutableDictionary dictionaryWithCapacity:20];

  while (opts[0].option) {
    //const char *option   = opts->option;
    const char *resource = opts->specifier;
    
    while (*resource == '.' || *resource == '*')
      resource++;
    NSString *nsresource = [self makeCKey:resource];
    
    // make sure there's no resource mentioned in options and not defaults.
    if (!defsdict[nsresource]) {
      if (! (!strcmp(resource, "font")        ||    // don't warn about these
             !strcmp(resource, "foreground")  ||
             !strcmp(resource, "textLiteral") ||
             !strcmp(resource, "textFile")    ||
             !strcmp(resource, "textURL")     ||
             !strcmp(resource, "textProgram") ||
             !strcmp(resource, "imageDirectory")))
        NSLog (@"warning: \"%s\" is in options but not defaults", resource);
    }
    [optsdict setValue:nsresource forKey:nsresource];
    
    opts++;
  }

#if 0
  // make sure there's no resource mentioned in defaults and not options.
  for (NSString *key in defsdict) {
    if (! [optsdict objectForKey:key])
      if (! ([key isEqualToString:@"foreground"] || // don't warn about these
             [key isEqualToString:@"background"] ||
             [key isEqualToString:@"Background"] ||
             [key isEqualToString:@"geometry"] ||
             [key isEqualToString:@"font"] ||
             [key isEqualToString:@"dontClearRoot"] ||

             // fps.c settings
             [key isEqualToString:@"fpsSolid"] ||
             [key isEqualToString:@"fpsTop"] ||
             [key isEqualToString:@"titleFont"] ||

             // analogtv.c settings
             [key isEqualToString:@"TVBrightness"] ||
             [key isEqualToString:@"TVColor"] ||
             [key isEqualToString:@"TVContrast"] ||
             [key isEqualToString:@"TVTint"]
             ))
      NSLog (@"warning: \"%@\" is in defaults but not options", key);
  }
#endif /* 0 */

#if 0
  // Dump the entire resource database.
  NSLog(@"userDefaults:");
  NSDictionary *d = [userDefaults dictionaryRepresentation];
  for (NSObject *key in [[d allKeys]
                          sortedArrayUsingSelector:@selector(compare:)]) {
    NSObject *val = [d objectForKey:key];
    NSLog (@"%@ = %@", key, val);
  }
  NSLog(@"globalDefaults:");
  d = [globalDefaults dictionaryRepresentation];
  for (NSObject *key in [[d allKeys]
                          sortedArrayUsingSelector:@selector(compare:)]) {
    NSObject *val = [d objectForKey:key];
    NSLog (@"%@ = %@", key, val);
  }
#endif

}

- (NSUserDefaultsController *) userDefaultsController
{
  NSAssert(userDefaultsController, @"userDefaultsController uninitialized");
  return userDefaultsController;
}

- (NSUserDefaultsController *) globalDefaultsController
{
  NSAssert(globalDefaultsController, @"globalDefaultsController uninitialized");
  return globalDefaultsController;
}

- (NSDictionary *) defaultOptions
{
  NSAssert(defaultOptions, @"defaultOptions uninitialized");
  return defaultOptions;
}


- (NSObject *) getObjectResource: (const char *) name
{
  // First look in userDefaults, then in globalDefaults.
  for (int globalp = 0; globalp <= 1; globalp++) {
    const char *name2 = name;
    while (1) {
      NSString *key = [self makeCKey:name2];
      NSObject *obj = [(globalp ? globalDefaults : userDefaults)
                        objectForKey:key];
      if (obj)
        return obj;

      // If key is "foo.bar.baz", check "foo.bar.baz", "bar.baz", and "baz".
      //
      const char *dot = strchr (name2, '.');
      if (dot && dot[1])
        name2 = dot + 1;
      else
        break;
    }
  }
  return NULL;
}


- (char *) getStringResource: (const char *) name
{
  NSObject *o = [self getObjectResource:name];
  //NSLog(@"%s = %@",name,o);
  if (o == nil) {
    if (! (!strcmp(name, "eraseMode") || // erase.c
           // xlockmore.c reads all of these whether used or not...
           !strcmp(name, "right3d") ||
           !strcmp(name, "left3d") ||
           !strcmp(name, "both3d") ||
           !strcmp(name, "none3d") ||
           !strcmp(name, "font") ||
           !strcmp(name, "labelFont") ||  // grabclient.c
           !strcmp(name, "titleFont") ||
           !strcmp(name, "fpsFont") ||    // fps.c
           !strcmp(name, "foreground") || // fps.c
           !strcmp(name, "background") ||
           !strcmp(name, "textLiteral")
           ))
      NSLog(@"warning: no preference \"%s\" [string]", name);
    return NULL;
  }
  if (! [o isKindOfClass:[NSString class]]) {
    NSLog(@"asked for %s as a string, but it is a %@", name, [o class]);
    o = [(NSNumber *) o stringValue];
  }

  NSString *os = (NSString *) o;
  char *result = strdup ([os cStringUsingEncoding:NSUTF8StringEncoding]);

  // Kludge: if the string is surrounded with single-quotes, remove them.
  // This happens when the .xml file says things like arg="-foo 'bar baz'"
  if (result[0] == '\'' && result[strlen(result)-1] == '\'') {
    result[strlen(result)-1] = 0;
    strcpy (result, result+1);
  }

  // Kludge: assume that any string that begins with "~" and has a "/"
  // anywhere in it should be expanded as if it is a pathname.
  if (result[0] == '~' && strchr (result, '/')) {
    os = @(result);
    free (result);
    result = strdup ([[os stringByExpandingTildeInPath]
                       cStringUsingEncoding:NSUTF8StringEncoding]);
  }

  return result;
}


- (double) getFloatResource: (const char *) name
{
  NSObject *o = [self getObjectResource:name];
  if (o == nil) {
    // xlockmore.c reads all of these whether used or not...
    if (! (!strcmp(name, "cycles") ||
           !strcmp(name, "size") ||
           !strcmp(name, "use3d") ||
           !strcmp(name, "delta3d") ||
           !strcmp(name, "wireframe") ||
           !strcmp(name, "showFPS") ||
           !strcmp(name, "fpsSolid") ||
           !strcmp(name, "fpsTop") ||
           !strcmp(name, "mono") ||
           !strcmp(name, "count") ||
           !strcmp(name, "ncolors") ||
           !strcmp(name, "doFPS") ||      // fps.c
           !strcmp(name, "eraseSeconds")  // erase.c
           ))
      NSLog(@"warning: no preference \"%s\" [float]", name);
    return 0.0;
  }
  if ([o isKindOfClass:[NSString class]]) {
    return [(NSString *) o doubleValue];
  } else if ([o isKindOfClass:[NSNumber class]]) {
    return [(NSNumber *) o doubleValue];
  } else {
    NSAssert2(0, @"%s = \"%@\" but should have been an NSNumber", name, o);
    abort();
  }
}


- (int) getIntegerResource: (const char *) name
{
  // Sliders might store float values for integral resources; round them.
  float v = [self getFloatResource:name];
  int i = (int) (v + (v < 0 ? -0.5 : 0.5)); // ignore sign or -1 rounds to 0
  // if (i != v) NSLog(@"%s: rounded %.3f to %d", name, v, i);
  return i;
}


- (BOOL) getBooleanResource: (const char *) name
{
  NSObject *o = [self getObjectResource:name];
  if (! o) {
    return NO;
  } else if ([o isKindOfClass:[NSNumber class]]) {
    double n = [(NSNumber *) o doubleValue];
    if (n == 0) return NO;
    else if (n == 1) return YES;
    else goto FAIL;
  } else if ([o isKindOfClass:[NSString class]]) {
    NSString *s = [((NSString *) o) lowercaseString];
    if ([s isEqualToString:@"true"] ||
        [s isEqualToString:@"yes"] ||
        [s isEqualToString:@"1"])
      return YES;
    else if ([s isEqualToString:@"false"] ||
             [s isEqualToString:@"no"] ||
             [s isEqualToString:@"0"] ||
             [s isEqualToString:@""])
      return NO;
    else
      goto FAIL;
  } else {
  FAIL:
    NSAssert2(0, @"%s = \"%@\" but should have been a boolean", name, o);
    abort();
  }
}


- (id) initWithName: (NSString *) name
            xrmKeys: (const XrmOptionDescRec *) opts
           defaults: (const char * const *) defs
{
  self = [self init];
  if (!self) return nil;

# ifndef USE_IPHONE
  userDefaults = [ScreenSaverDefaults defaultsForModuleWithName:name];
  globalDefaults = [[GlobalDefaults alloc] initWithDomain:@UPDATER_DOMAIN];
# else  // USE_IPHONE
  userDefaults = [NSUserDefaults standardUserDefaults];
  globalDefaults = userDefaults;
# endif // USE_IPHONE

  // Convert "org.jwz.xscreensaver.NAME" to just "NAME".
  NSRange r = [name rangeOfString:@"." options:NSBackwardsSearch];
  if (r.length)
    name = [name substringFromIndex:r.location+1];
  name = [name stringByReplacingOccurrencesOfString:@" " withString:@""];
  saver_name = name;

  [self registerXrmKeys:opts defaults:defs];
  return self;
}

@end
