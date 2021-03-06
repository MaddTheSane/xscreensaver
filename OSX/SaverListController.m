/* xscreensaver, Copyright (c) 2012-2014 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * This implements the top-level screen-saver selection list in the iOS app.
 */

#ifdef USE_IPHONE  // whole file


#import "SaverListController.h"
#import "SaverRunner.h"
#import "yarandom.h"
#import "version.h"

#undef countof
#define countof(x) (sizeof((x))/sizeof((*x)))


@implementation SaverListController

- (void) titleTapped:(id) sender
{
  [[UIApplication sharedApplication]
    openURL:[NSURL URLWithString:@"http://www.jwz.org/xscreensaver/"]];
}


- (void)makeTitleBar
{
  // Extract the version number and release date from the version string.
  // Here's an area where I kind of wish I had "Two Problems".
  // I guess I could add custom key to the Info.plist for this.

  NSArray *a = [@(screensaver_id)
                 componentsSeparatedByCharactersInSet:
                   [NSCharacterSet
                     characterSetWithCharactersInString:@" ()-"]];
  NSString *vers = a[3];
  NSString *year = a[7];

  NSString *line1 = [@"XScreenSaver " stringByAppendingString: vers];
  NSString *line2 = [@"\u00A9 " stringByAppendingString:
                        [year stringByAppendingString:
                                @" Jamie Zawinski <jwz@jwz.org>"]];

  UIView *v = [[UIView alloc] initWithFrame:CGRectZero];

  // The "go to web page" button on the right

  UIImage *img = [UIImage imageWithContentsOfFile:
                            [[[NSBundle mainBundle] bundlePath]
                              stringByAppendingPathComponent:
                                @"iSaverRunner29t.png"]];
  UIButton *button = [[UIButton alloc] init];
  [button setFrame: CGRectMake(0, 0, img.size.width, img.size.height)];
  [button setBackgroundImage:img forState:UIControlStateNormal];
  [button addTarget:self
          action:@selector(titleTapped:)
          forControlEvents:UIControlEventTouchUpInside];
  self.navigationItem.rightBarButtonItem = 
    [[UIBarButtonItem alloc] initWithCustomView: button];

  // The title bar

  UILabel *label1 = [[UILabel alloc] initWithFrame:CGRectZero];
  UILabel *label2 = [[UILabel alloc] initWithFrame:CGRectZero];
  [label1 setText: line1];
  [label2 setText: line2];
  [label1 setBackgroundColor:[UIColor clearColor]];
  [label2 setBackgroundColor:[UIColor clearColor]];

  [label1 setFont: [UIFont boldSystemFontOfSize: 17]];
  [label2 setFont: [UIFont systemFontOfSize: 12]];
  [label1 sizeToFit];
  [label2 sizeToFit];

  CGRect r1 = [label1 frame];
  CGRect r2 = [label2 frame];
  CGRect r3 = r2;

  CGRect win = [self view].frame;
  if (win.size.width > 320) {					// iPad
    [label1 setTextAlignment: NSTextAlignmentLeft];
    [label2 setTextAlignment: NSTextAlignmentRight];
    label2.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
    r3.size.width = win.size.width;
    r1 = r3;
    r1.origin.x   += 6;
    r1.size.width -= 12;
    r2 = r1;

  } else {							// iPhone
    r3.size.width = 320; // force it to be flush-left
    [label1 setTextAlignment: NSTextAlignmentLeft];
    [label2 setTextAlignment: NSTextAlignmentLeft];
    r1.origin.y = -1;    // make it fit in landscape
    r2.origin.y = r1.origin.y + r1.size.height - 2;
    r3.size.height = r1.size.height + r2.size.height;
  }
  v.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  [label1 setFrame:r1];
  [label2 setFrame:r2];
  [v setFrame:r3];

  [v addSubview:label1];
  [v addSubview:label2];

  self.navigationItem.titleView = v;
}


- (id)initWithNames:(NSArray *)names descriptions:(NSDictionary *)descs;
{
  self = [self init];
  if (! self) return 0;
  [self reload:names descriptions:descs];
  [self makeTitleBar];
  return self;
}


- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tv
{
  int n = countof(list_by_letter);
  NSMutableArray *a = [NSMutableArray arrayWithCapacity: n];
  for (int i = 0; i < n; i++) {
    char s[2];
    s[0] = (i == 'Z'-'A'+1 ? '#' : i+'A');
    s[1] = 0;
    [a addObject: @(s)];
  }
  return a;
}


- (void) reload:(NSArray *)names descriptions:(NSDictionary *)descs
{
  descriptions = descs;

  int n = countof(list_by_letter);
  for (int i = 0; i < n; i++) {
    list_by_letter[i] = [[NSMutableArray alloc] init];
  }

  for (NSString *name in names) {
    int index = ([name cStringUsingEncoding: NSASCIIStringEncoding])[0];
    if (index >= 'a' && index <= 'z')
      index -= 'a'-'A';
    if (index >= 'A' && index <= 'Z')
      index -= 'A';
    else
      index = n-1;
    [list_by_letter[index] addObject: name];
  }

  active_section_count = 0;
  letter_sections = [[NSMutableArray alloc] init];
  section_titles = [[NSMutableArray alloc] init];
  for (int i = 0; i < n; i++) {
    if ([list_by_letter[i] count] > 0) {
      active_section_count++;
      [letter_sections addObject: list_by_letter[i]];
      if (i <= 'Z'-'A')
        [section_titles addObject: [NSString stringWithFormat: @"%c", i+'A']];
      else
        [section_titles addObject: @"#"];
    }
  }
  [self.tableView reloadData];
}


- (NSString *)tableView:(UITableView *)tv
              titleForHeaderInSection:(NSInteger)section
{
  return section_titles[section];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tv
{
  return active_section_count;
}


- (NSInteger)tableView:(UITableView *)tv
                       numberOfRowsInSection:(NSInteger)section
{
  return [letter_sections[section] count];
}

- (NSInteger)tableView:(UITableView *)tv
             sectionForSectionIndexTitle:(NSString *)title
               atIndex:(NSInteger) index
{
  int i = 0;
  for (NSString *sectionTitle in section_titles) {
    if ([sectionTitle isEqualToString: title])
      return i;
    i++;
  }
  return -1;
}


- (UITableViewCell *)tableView:(UITableView *)tv
                     cellForRowAtIndexPath:(NSIndexPath *)ip
{
  NSString *id =
    letter_sections[[ip indexAtPosition: 0]][[ip indexAtPosition: 1]];
  NSString *desc = descriptions[id];

  UITableViewCell *cell = [tv dequeueReusableCellWithIdentifier: id];
  if (!cell) {
    cell = [[UITableViewCell alloc]
              initWithStyle: (desc
                              ? UITableViewCellStyleSubtitle
                              : UITableViewCellStyleDefault)
              reuseIdentifier: id];
    cell.textLabel.text = id;

    cell.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
    if (desc)
      cell.detailTextLabel.text = desc;
  }
  return cell;
}


/* Selecting a row launches the saver.
 */
- (void)tableView:(UITableView *)tv
        didSelectRowAtIndexPath:(NSIndexPath *)ip
{
  UITableViewCell *cell = [tv cellForRowAtIndexPath: ip];
  SaverRunner *s = 
    (SaverRunner *) [[UIApplication sharedApplication] delegate];
  if (! s) return;
  NSAssert ([s isKindOfClass:[SaverRunner class]], @"not a SaverRunner");
  [s loadSaver: cell.textLabel.text];
}

/* Selecting a row's Disclosure Button opens the preferences.
 */
- (void)tableView:(UITableView *)tv
        accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)ip
{
  UITableViewCell *cell = [tv cellForRowAtIndexPath: ip];
  SaverRunner *s = 
    (SaverRunner *) [[UIApplication sharedApplication] delegate];
  if (! s) return;
  NSAssert ([s isKindOfClass:[SaverRunner class]], @"not a SaverRunner");
  [s openPreferences: cell.textLabel.text];
}


- (void) scrollTo: (NSString *) name
{
  int i = 0;
  int j = 0;
  Bool ok = NO;
  for (NSArray *a in letter_sections) {
    j = 0;
    for (NSString *n in a) {
      ok = [n isEqualToString: name];
      if (ok) goto DONE;
      j++;
    }
    i++;
  }
 DONE:
  if (ok) {
    NSIndexPath *ip = [NSIndexPath indexPathForRow: j inSection: i];
    [self.tableView selectRowAtIndexPath:ip
                    animated:NO
                    scrollPosition: UITableViewScrollPositionMiddle];
  }
}


- (BOOL)shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation)o
{
  return YES;
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


/* Shake means load a random screen saver.
 */
- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event
{
  if (motion != UIEventSubtypeMotionShake)
    return;
  NSMutableArray *a = [NSMutableArray arrayWithCapacity: 200];
  for (NSArray *sec in letter_sections)
    for (NSString *s in sec)
      [a addObject: s];
  NSUInteger n = [a count];
  if (! n) return;
  NSString *which = a[(random() % n)];

  SaverRunner *s = 
    (SaverRunner *) [[UIApplication sharedApplication] delegate];
  if (! s) return;
  NSAssert ([s isKindOfClass:[SaverRunner class]], @"not a SaverRunner");
  [self scrollTo: which];
  [s loadSaver: which];
}

@end


#endif // USE_IPHONE -- whole file
