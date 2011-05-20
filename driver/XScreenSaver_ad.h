"#error Do not run app-defaults files through xrdb!",
"#error That does not do what you might expect.",
"#error Put this file in /usr/lib/X11/app-defaults/XScreenSaver instead.",
"*timeout:		10",
"*cycle:			10",
"*lockTimeout:		0",
"*passwdTimeout:		30",
"*nice:			10",
"*lock:			False",
"*lockVTs:		True",
"*verbose:		False",
"*timestamp:		False",
"*fade:			True",
"*unfade:		False",
"*fadeSeconds:		3",
"*fadeTicks:		20",
"*splash:		True",
"*splashDuration:	5",
"*visualID:		default",
"*captureStderr: 	True",
"*overlayTextForeground:	#FFFF00",
"*overlayTextBackground:	#000000",
"*overlayStderr:		True",
"*font:			*-medium-r-*-140-*-m-*",
"*sgiSaverExtension:	True",
"*mitSaverExtension:	False",
"*xidleExtension:	True",
"*procInterrupts:	True",
"*demoCommand: xscreensaver-demo",
"*prefsCommand: xscreensaver-demo -prefs",
"*helpURL: http://www.jwz.org/xscreensaver/man.html",
"*loadURL: netscape -remote 'openURL(%s)' || netscape '%s'",
"*manualCommand: xterm +sb -fg black -bg gray75 -T '%s manual' \
        -e /bin/sh -c 'man \"%s\" || read foo'",
"*dateFormat:		%d-%b-%y (%a); %I:%M %p",
"*installColormap:	True",
"*programs:								      \
		 \"Qix (solid)\" 	qix -root -solid -delay 0 -segments 100	    \\n\
	   \"Qix (transparent)\" 	qix -root -count 4 -solid -transparent	    \\n\
		\"Qix (linear)\" 	qix -root -count 5 -solid -transparent	      \
				  -linear -segments 250 -size 100	    \\n\
- mono: 	   \"Qix (xor)\" 	qix -root -linear -count 5 -size 200	      \
				  -spread 30 -segments 75 -solid -xor	    \\n\
									      \
	  \"Attraction (balls)\" 	attraction -root -mode balls		    \\n\
	  \"Attraction (lines)\" 	attraction -root -mode lines -points 3	      \
				  -segments 200				    \\n\
-	   \"Attraction (poly)\" 	attraction -root -mode polygons		    \\n\
	\"Attraction (splines)\" 	attraction -root -mode splines -segments      \
				  300					    \\n\
	\"Attraction (orbital)\" 	attraction -root -mode lines -radius 300      \
				  -orbit -vmult 0.5			    \\n\
									      \
				pyro -root				    \\n\
				rocks -root				    \\n\
				helix -root				    \\n\
				pedal -root				    \\n\
				rorschach -root -offset 7		    \\n\
				hopalong -root				    \\n\
				greynetic -root				    \\n\
				xroger -root				    \\n\
				imsmap -root				    \\n\
				slidescreen -root			    \\n\
				decayscreen -root			    \\n\
				jigsaw -root				    \\n\
				blitspin -root -grab			    \\n\
				slip -root				    \\n\
				distort -root				    \\n\
				spotlight -root				    \\n\
	      \"Ripples (oily)\"	ripples -root -oily -light 2		    \\n\
	      \"Ripples (stir)\"	ripples -root -oily -light 2 -stir	    \\n\
	   \"Ripples (desktop)\"	ripples -root -water -light 6		    \\n\
				hypercube -root				    \\n\
				halo -root				    \\n\
				maze -root				    \\n\
				noseguy -root				    \\n\
				flame -root				    \\n\
				lmorph -root				    \\n\
				deco -root				    \\n\
				moire -root				    \\n\
				moire2 -root				    \\n\
				lightning -root				    \\n\
				strange -root				    \\n\
				spiral -root				    \\n\
				laser -root				    \\n\
				grav -root				    \\n\
	       \"Grav (trails)\" 	grav -root -trail -decay		    \\n\
				drift -root				    \\n\
				ifs -root				    \\n\
				julia -root				    \\n\
				penrose -root				    \\n\
				sierpinski -root			    \\n\
				braid -root				    \\n\
				galaxy -root				    \\n\
				bouboule -root				    \\n\
				swirl -root				    \\n\
				flag -root				    \\n\
				sphere -root				    \\n\
				forest -root				    \\n\
				lisa -root				    \\n\
				lissie -root				    \\n\
				goop -root -max-velocity 0.5 -elasticity      \
				  0.9					    \\n\
				starfish -root				    \\n\
	     \"Starfish (blob)\" 	starfish -root -blob			    \\n\
				munch -root				    \\n\
				fadeplot -root				    \\n\
				coral -root -delay 0			    \\n\
				mountain -root				    \\n\
				triangle -root -delay 1			    \\n\
				worm -root				    \\n\
				rotor -root				    \\n\
				ant -root				    \\n\
				demon -root				    \\n\
				loop -root				    \\n\
				vines -root				    \\n\
				kaleidescope -root			    \\n\
				xjack -root				    \\n\
				xlyap -root -randomize			    \\n\
				cynosure -root				    \\n\
				flow -root				    \\n\
				epicycle -root				    \\n\
				interference -root			    \\n\
				truchet -root -randomize		    \\n\
				bsod -root				    \\n\
				crystal -root				    \\n\
				discrete -root				    \\n\
				kumppa -root				    \\n\
				rd-bomb -root				    \\n\
	    \"RD-Bomb (mobile)\" 	rd-bomb -root -speed 1 -size 0.1	    \\n\
				sonar -root				    \\n\
				t3d -root				    \\n\
				penetrate -root				    \\n\
				deluxe -root				    \\n\
				compass -root				    \\n\
				squiral -root				    \\n\
				xflame -root				    \\n\
				wander -root				    \\n\
	      \"Wander (spots)\" 	wander -root -advance 0 -size 10 -circles     \
				  True -length 10000 -reset 100000	    \\n\
				critical -root				    \\n\
				phosphor -root				    \\n\
				xmatrix -root				    \\n\
				petri -root -size 2 -count 20		    \\n\
		     \"Petri 2\" 	petri -root -minlifespeed 0.02		      \
				  -maxlifespeed 0.03 -minlifespan 1	      \
				  -maxlifespan 1 -instantdeathchan 0	      \
				  -minorchan 0 -anychan 0.3		    \\n\
				shadebobs -root				    \\n\
				ccurve -root				    \\n\
				blaster -root				    \\n\
				bumps -root				    \\n\
				xteevee -root				    \\n\
				xspirograph -root			    \\n\
  color: 			bubbles -root				    \\n\
  default-n:			webcollage -root			    \\n\
  default-n:  \"WebCollage (whacked)\"					      \
				webcollage -root -filter		      \
				  'vidwhacker -stdin -stdout'		    \\n\
- default-n:			vidwhacker -root			    \\n\
									      \
	   GL:			gears -root				    \\n\
	   GL:			superquadrics -root			    \\n\
	   GL:			morph3d -root				    \\n\
	   GL:			cage -root				    \\n\
	   GL:			moebius -root				    \\n\
	   GL:			stairs -root				    \\n\
	   GL:			pipes -root				    \\n\
	   GL:			sproingies -root			    \\n\
	   GL:			rubik -root				    \\n\
	   GL:			atlantis -root				    \\n\
	   GL:			lament -root				    \\n\
	   GL:			bubble3d -root				    \\n\
	   GL:			glplanet -root				    \\n\
	   GL:			pulsar -root				    \\n\
-	   GL:	   \"Pulsar (textures)\"					      \
				  pulsar -root -texture -mipmap		      \
				  -texture_quality -light -fog		    \\n\
	   GL:			extrusion -root				    \\n\
	   GL:			sierpinski3d -root			    \\n\
	   GL:			starwars -root				    \\n\
									      \
-				xdaliclock -root -builtin3 -cycle	    \\n\
- default-n:			xearth -nofork -nostars -ncolors 50	      \
				  -night 3 -wait 0 -timewarp 400.0 -pos	      \
				  sunrel/38/-30				    \\n\
-				ssystem -fullscreen :32			    \\n\
-				xmountains -b -M -Z 0 -r 1		    \\n\
-	\"XMountains (top)\"	xmountains -b -M -Z 0 -r 1 -m		    \\n\
-                               xaos -root -autopilot -incoloring -1	      \
                                  -nogui -outcoloring -1	            \\n\
-				xfishtank -d -s                             \\n\
-				xsnow                                       \\n\
-				goban -root                                 \\n\
-				electricsheep                               \\n",
"XScreenSaver.pointerPollTime:		5",
"XScreenSaver.initialDelay:		0",
"XScreenSaver.windowCreationTimeout:	30",
"XScreenSaver.bourneShell:		/bin/sh",
"*Dialog.headingFont:		*-times-bold-r-*-*-*-180-*-*-*-iso8859-1",
"*Dialog.bodyFont:		*-helvetica-bold-r-*-*-*-140-*-*-*-iso8859-1",
"*Dialog.labelFont:		*-helvetica-bold-r-*-*-*-140-*-*-*-iso8859-1",
"*Dialog.buttonFont:		*-helvetica-bold-r-*-*-*-140-*-*-*-iso8859-1",
"*Dialog.dateFont:		*-courier-medium-r-*-*-*-80-*-*-*-iso8859-1",
"*Dialog.foreground:		#000000",
"*Dialog.background:		#BFBFBF",
"*Dialog.Button.foreground:	#000000",
"*Dialog.Button.background:	#D0D0D0",
"*Dialog.text.foreground:	#000000",
"*Dialog.text.background:	#FFFFFF",
"*Dialog.logo.foreground:	#FF0000",
"*Dialog.logo.background:	#FFFFFF",
"*Dialog.topShadowColor:		#E7E7E7",
"*Dialog.bottomShadowColor:	#737373",
"*Dialog.logo.width:		200",
"*Dialog.logo.height:		200",
"*Dialog.internalBorderWidth:	30",
"*Dialog.borderWidth:		1",
"*Dialog.shadowThickness:	4",
"*passwd.heading.label:		XScreenSaver %s",
"*passwd.body.label:		This display is locked.",
"*passwd.user.label:		User:",
"*passwd.passwd.label:		Password:",
"*passwd.passwdFont:		*-courier-medium-r-*-*-*-140-*-*-*-iso8859-1",
"*passwd.thermometer.width:	8",
"*splash.heading.label:		XScreenSaver %s",
"*splash.body.label:		Copyright � 1991-1999 by",
"*splash.body2.label:		Jamie Zawinski <jwz@jwz.org>",
"*splash.demo.label:		Demo",
"*splash.prefs.label:		Prefs",
"*splash.help.label:		Help",
"*fontList:                       *-helvetica-medium-r-*-*-*-120-*-*-*-iso8859-1",
"*demoDialog*label1.fontList:     *-helvetica-medium-r-*-*-*-140-*-*-*-iso8859-1",
"*cmdText.fontList:                 *-courier-medium-r-*-*-*-120-*-*-*-iso8859-1",
"*label0.fontList:                  *-helvetica-bold-r-*-*-*-140-*-*-*-iso8859-1",
"XScreenSaver*doc.fontList:       *-helvetica-medium-r-*-*-*-100-*-*-*-iso8859-1",
"*foreground:			#000000",
"*background:			#C0C0C0",
"*XmTextField.foreground:	#000000",
"*XmTextField.background:	#FFFFFF",
"*list.foreground:		#000000",
"*list.background:		#FFFFFF",
"*ApplicationShell.title:	XScreenSaver",
"*warning.title:			XScreenSaver",
"*warning_popup.title:		XScreenSaver",
"*allowShellResize:		True",
"*autoUnmanage:			False",
"*menubar*file.labelString:	File",
"*menubar*file.mnemonic:		F",
"*file.blank.labelString:	Blank Screen Now",
"*file.blank.mnemonic:		B",
"*file.lock.labelString:		Lock Screen Now",
"*file.lock.mnemonic:		L",
"*file.kill.labelString:		Kill Daemon",
"*file.kill.mnemonic:		K",
"*file.restart.labelString:	Restart Daemon",
"*file.restart.mnemonic:		R",
"*file.exit.labelString:		Exit",
"*file.exit.mnemonic:		E",
"*menubar*edit.labelString:	Edit",
"*menubar*edit.mnemonic:		E",
"*edit.cut.labelString:		Cut",
"*edit.cut.mnemonic:		u",
"*edit.copy.labelString:		Copy",
"*edit.copy.mnemonic:		C",
"*edit.paste.labelString:	Paste",
"*edit.paste.mnemonic:		P",
"*menubar*help.labelString:	Help",
"*menubar*help.mnemonic:		H",
"*help.about.labelString:	About...",
"*help.about.mnemonic:		A",
"*help.docMenu.labelString:	Documentation...",
"*help.docMenu.mnemonic:		D",
"*demoTab.marginWidth:		10",
"*optionsTab.marginWidth:	10",
"*XmScrolledWindow.topOffset:	10",
"*XmScrolledWindow.leftOffset:	10",
"*demoTab.topOffset:		4",
"*form1.bottomOffset:		10",
"*form3.leftOffset:		10",
"*form3.rightOffset:		10",
"*frame.topOffset:		10",
"*frame.bottomOffset:		10",
"*enabled.topOffset:		10",
"*visLabel.topOffset:		10",
"*combo.topOffset:		10",
"*form4.bottomOffset:		4",
"*hr.bottomOffset:		4",
"*XmComboBox.marginWidth:	0",
"*XmComboBox.marginHeight:	0",
"*demo.marginWidth:		30",
"*demo.marginHeight:		4",
"*man.marginWidth:		10",
"*man.marginHeight:		4",
"*down.leftOffset:		40",
"*down.marginWidth:		4",
"*down.marginHeight:		4",
"*up.marginWidth:		4",
"*up.marginHeight:		4",
"*frame.traversalOn:		False",
"*list.automaticSelection:	True",
"*list.visibleItemCount:		20",
"*doc.columns:			60",
"*combo.columns:			11",
"*demoTab.labelString:		Graphics Demos",
"*optionsTab.labelString:	Screensaver Options",
"*down.labelString:		\\\\/ ",
"*up.labelString:		/\\\\ ",
"*frameLabel.labelString:	",
"*cmdLabel.labelString:		Command Line:",
"*cmdLabel.alignment:		ALIGNMENT_BEGINNING",
"*enabled.labelString:		Enabled",
"*visLabel.labelString:		Visual:",
"*visLabel.alignment:		ALIGNMENT_END",
"*visLabel.leftOffset:		20",
"*demo.labelString:		Demo",
"*man.labelString:		Documentation...",
"*done.labelString:		Quit",
"*preferencesLabel.labelString:	XScreenSaver Parameters",
"*timeoutLabel.labelString:	Saver Timeout",
"*cycleLabel.labelString:	Cycle Timeout",
"*fadeSecondsLabel.labelString:	Fade Duration",
"*fadeTicksLabel.labelString:	Fade Ticks",
"*lockLabel.labelString:		Lock Timeout",
"*passwdLabel.labelString:	Password Timeout",
"*preferencesForm*XmTextField.columns:	8",
"*verboseToggle.labelString:	Verbose",
"*cmapToggle.labelString:	Install Colormap",
"*fadeToggle.labelString:	Fade Colormap",
"*unfadeToggle.labelString:	Unfade Colormap",
"*lockToggle.labelString:	Require Password",
"*OK.marginWidth:		30",
"*OK.marginHeight:		4",
"*OK.leftOffset:			10",
"*OK.bottomOffset:		10",
"*Cancel.marginWidth:		30",
"*Cancel.marginHeight:		4",
"*Cancel.rightOffset:		10",
"*Cancel.bottomOffset:		10",
"*hacks.documentation.isInstalled: True",
"*hacks.qix.documentation:						\
This is the swiss army chainsaw of qix programs.  It bounces a series	\
of line segments around the screen, and uses variations on this basic	\
motion pattern to produce all sorts of different presentations: line	\
segments, filled polygons, overlapping translucent areas...  Written	\
by Jamie Zawinski.",
"*hacks.attraction.documentation:					\
Like qix, this uses a simple simple motion model to generate many	\
different display modes.  The control points attract each other up to	\
a certain distance, and then begin to repel each other.	 The		\
attraction/repulsion is proportional to the distance between any two	\
particles, similar to the strong and weak nuclear forces.		\
								    \\n\\n\
One of the most interesting ways to watch this hack is simply as	\
bouncing balls, because their motions and interactions with each	\
other are so odd.  Sometimes two balls will get into a tight orbit	\
around each other, to be interrupted later by a third, or by the edge	\
of the screen.	It looks quite chaotic.					\
								    \\n\\n\
Written by Jamie Zawinski, based on Lisp code by John Pezaris.",
"*hacks.pyro.documentation:						\
Pyro draws exploding fireworks.	 Blah blah blah.  Written by Jamie	\
Zawinski.",
"*hacks.helix.documentation:						\
This repeatedly generates spirally string-art-ish patterns.  Written	\
by Jamie Zawinski.",
"*hacks.pedal.documentation:						\
This is sort of a combination spirograph/string-art.  It generates a	\
large, complex polygon, and lets the X server do the bulk of the work	\
by giving it an even/odd winding rule.	Written by Dale Moore, based	\
on some ancient PDP-11 code.",
"*hacks.rorschach.documentation:						\
This generates random inkblot patterns.	 The algorithm is deceptively	\
simple for how well it works; it merely walks a dot around the screen	\
randomly, and then reflects the image horizontally, vertically, or	\
both.  Any deep-seated neurotic tendencies which this program reveals	\
are your own problem.  Written by Jamie Zawinski.",
"*hacks.hopalong.documentation:						\
This draws lacy fractal patterns, based on iteration in the imaginary	\
plane, from a 1986 Scientific American article.	 Mostly written by	\
Patrick Naughton.",
"*hacks.greynetic.documentation:						\
This draws random colored and stippled rectangles.  Written by Jamie	\
Zawinski.",
"*hacks.xroger.documentation:						\
The XScreenSaver logo. Don't you hate it?  So do I.  Would you like	\
to design a new logo for XScreenSaver?	If so, send jwz your		\
submissions.",
"*hacks.imsmap.name: IMSmap",
"*hacks.imsmap.documentation:						\
This generates random cloud-like patterns.  It looks quite different	\
in monochrome and color.  The basic idea is to take four points on	\
the edge of the image, and assign each a random ``elevation''.	Then	\
find the point between them, and give it a value which is the average	\
of the other four, plus some small random offset. Then coloration is	\
done based on elevation.						\
								    \\n\\n\
The color selection is done by binding the elevation to either hue,	\
saturation, or brightness, and assigning random values to the others.	\
The ``brightness'' mode tends to yield cloudlike patterns, and the	\
others tend to generate images that look like heat-maps or CAT-scans.	\
Written by Juergen Nickelsen and Jamie Zawinski.",
"*hacks.slidescreen.name: SlideScreen",
"*hacks.slidescreen.documentation:					\
This grabs an image of whatever is on your screen, divides it into a	\
grid, and then randomly shuffles the squares around as if it was one	\
of those annoying ``16-puzzle'' games, where there is a grid of		\
squares, one of which is missing.  I hate trying to solve those		\
puzzles, but watching one permute itself is more amusing.  Written by	\
Jamie Zawinski.",
"*hacks.decayscreen.name: DecayScreen",
"*hacks.decayscreen.documentation:					\
This grabs an image of whatever is on your screen, and makes it melt.	\
You've no doubt seen this effect before, but no screensaver would	\
really be complete without it.	It works best if there's something	\
colorful visible.  Warning, if the effect continues after the screen	\
saver is off, seek medical attention.  Written by David Wald and	\
Vivek Khera.								\
								    \\n\\n\
A number of these screenhacks have the ability to take an image of	\
your desktop and manipulate it in some way.  On SGI systems, these	\
programs are able to (at random) pull their source image from the	\
system's video input instead!  This works nicely if you leave some	\
some random television station plugged in.",
"*hacks.jigsaw.documentation:						\
This grabs a screen image, carves it up into a jigsaw puzzle,		\
shuffles it, and then solves the puzzle.  This works especially well	\
when you feed it an external video signal instead of letting it grab	\
the screen image (actually, I guess this is generally true...)	When	\
it is grabbing a video image, it is sometimes pretty hard to guess	\
what the image is going to look like once the puzzle is solved.		\
Written by Jamie Zawinski.",
"*hacks.blitspin.name: BlitSpin",
"*hacks.blitspin.documentation:						\
The ``blitspin'' hack repeatedly rotates a bitmap by 90 degrees by	\
using logical operations: the bitmap is divided into quadrants, and	\
the quadrants are shifted clockwise.  Then the same thing is done	\
again with progressively smaller quadrants, except that all		\
sub-quadrants of a given size are rotated in parallel.	Written by	\
Jamie Zawinski based on some cool SmallTalk code seen in in Byte	\
Magazine in 1981.							\
								    \\n\\n\
As you watch it, the image appears to dissolve into static and then	\
reconstitute itself, but rotated. You can provide the image to use,	\
as an XBM or XPM file, or tell it to grab a screen image and rotate	\
that.",
"*hacks.slip.documentation:						\
This program throws some random bits on the screen, then sucks them	\
through a jet engine and spews them out the other side.	 To avoid	\
turning the image completely to mush, every now and then it will and	\
then it interjects some splashes of color into the scene, or go into	\
a spin cycle, or stretch the image like taffy, or (this is my		\
addition) grab an image of your current desktop to chew on.		\
Originally written by Scott Draves; whacked on by Jamie Zawinski.",
"*hacks.distort.documentation:						\
This hack grabs an image of the screen, and then lets a transparent	\
lens wander around the screen, magnifying whatever is underneath.	\
Written by Jonas Munsin.",
"*hacks.spotlight.documentation:						\
Draws a spotlight scanning across a black screen, illumnating the	\
underlying desktop when it passes.  Written by Rick Schultz.",
"*hacks.hypercube.documentation:						\
This displays 2D projections of the sequence of 3D objects which are	\
the projections of the 4D analog to the cube: as a square is composed	\
of four lines, each touching two others; and a cube is composed of	\
six squares, each touching four others; a hypercube is composed of	\
eight cubes, each touching six others.	To make it easier to		\
visualize the rotation, it uses a different color for the edges of	\
each face.  Don't think about it too long, or your brain will melt.	\
Written by Joe Keane, Fritz Mueller, and Jamie Zawinski.",
"*hacks.halo.documentation:						\
This draws trippy psychedelic circular patterns that hurt to look at.	\
It can also animate the control-points, but that takes a lot of CPU	\
and bandwidth.	Written by Jamie Zawinski.",
"*hacks.maze.documentation:						\
This is the ancient X maze demo, modified to work with xscreensaver.	\
It generates a random maze, then solves it with visual feedback.	\
Originally by Jim Randell; modified by a cast of thousands.",
"*hacks.noseguy.documentation:						\
A little man with a big nose wanders around your screen saying		\
things.	 The things which he says can come from a file, or from an	\
external program like `zippy' or `fortune'.  This was extracted from	\
`xnlock' by Dan Heller.	 Colorized by Jamie Zawinski.",
"*hacks.flame.documentation:						\
Another iterative fractal generator.  Written by Scott Draves.",
"*hacks.lmorph.name: LMorph",
"*hacks.lmorph.documentation:						\
This generates random spline-ish line drawings and morphs between	\
them.  Written by Sverre H.  Huseby and Glenn T.  Lines.",
"*hacks.deco.documentation:						\
This one subdivides and colors rectangles randomly.  It looks kind of	\
like Brady-Bunch-era rec-room wall paneling.  (Raven says: ``this	\
screensaver is ugly enough to peel paint.'')  Written by Jamie		\
Zawinski, inspired by Java code by Michael Bayne.",
"*hacks.moire.documentation:						\
This one draws cool circular interference patterns.  Most of the	\
circles you see aren't explicitly rendered, but show up as a result	\
of interactions between the other pixels that were drawn.  Written by	\
Jamie Zawinski, inspired by Java code by Michael Bayne.	 As he		\
pointed out, the beauty of this one is that the heart of the display	\
algorithm can be expressed with just a pair of loops and a handful of	\
arithmetic, giving it a high ``display hack metric''.",
"*hacks.moire2.documentation:						\
Another example of the fun you can have with moire			\
interference patterns; this hack generates fields of concentric		\
circles or ovals, and combines the planes with various operations.	\
The planes are moving independently of one another, causing the		\
interference lines to ``spray.''  Written by Jamie Zawinski.",
"*hacks.lightning.documentation:						\
This one draws crackling fractal lightning bolts.  It's simple,		\
direct, and to the point.  If only it had sound... Written by Keith	\
Romberg.",
"*hacks.strange.documentation:						\
This draws strange attractors: it's a colorful,				\
unpredictably-animating field of dots that swoops and twists around.	\
The motion is very nice.  Written by Massimino Pascal.",
"*hacks.spiral.documentation:						\
Moving circular patterns, by Peter Schmitzberger.  Moving circular	\
patterns means moire; interference patterns, of course.",
"*hacks.laser.documentation:						\
Moving radiating lines, that look vaguely like scanning laser beams.	\
Written by Pascal Pensa.  (Frankie say: relax.)",
"*hacks.grav.documentation:						\
This program draws a simple orbital simulation.	 If you turn on		\
trails, it looks kind of like a cloud-chamber photograph.  Written	\
by Greg Bowering.",
"*hacks.drift.documentation:						\
How could one possibly describe this except as ``drifting recursive	\
fractal cosmic flames?''  Another fine hack from the Scott Draves	\
collection of fine hacks.",
"*hacks.ifs.name: IFS",
"*hacks.ifs.documentation:						\
This one draws spinning, colliding iterated-function-system images.	\
Written by Massimino Pascal.",
"*hacks.julia.documentation:						\
This one draws spinning, animating (are you detecting a pattern here	\
yet?)  explorations of the Julia set. You've probably seen static	\
images of this fractal form before, but it's a lot of fun to watch in	\
motion as well.	 One interesting thing is that there is a small		\
swinging dot passing in front of the image, which indicates the		\
control point from which the rest of the image was generated.		\
Written by Sean McCullough.",
"*hacks.penrose.documentation:						\
Draws quasiperiodic tilings; think of the implications on modern	\
formica technology.  Written by Timo Korvola.				\
								    \\n\\n\
In April 1997, Sir Roger Penrose, a British math professor who has	\
worked with Stephen Hawking on such topics as relativity, black		\
holes, and whether time has a beginning, filed a			\
copyright-infringement lawsuit against the Kimberly-Clark		\
Corporation, which Penrose said copied a pattern he created (a		\
pattern demonstrating that ``a nonrepeating pattern could exist in	\
nature'') for its Kleenex quilted toilet paper.	Penrose said he		\
doesn't like litigation but, ``When it comes to the population of	\
Great Britain being invited by a multinational to wipe their bottoms	\
on what appears to be the work of a Knight of the Realm, then a last	\
stand must be taken.''							\
								    \\n\\n\
As reported by News of the Weird #491, 4-jul-1997.",
"*hacks.sierpinski.documentation:					\
This draws the two-dimensional variant of the recursive Sierpinski	\
triangle fractal.  Written by Desmond Daignault.",
"*hacks.braid.documentation:						\
Draws random color-cycling inter-braided concentric circles.  Written	\
by John Neil.",
"*hacks.galaxy.documentation:						\
This draws spinning galaxies, which then collide and scatter their	\
stars to the, uh, four winds or something.  Originally an Amiga		\
program by Uli Siegmund.",
"*hacks.bouboule.documentation:						\
This draws what looks like a spinning, deforming baloon with		\
varying-sized spots painted on its invisible surface.  Written by	\
Jeremie Petit.",
"*hacks.swirl.documentation:						\
More flowing, swirly patterns. This version is by M.  Dobie and R.	\
Taylor, but you might have seen a Mac program similar to this called	\
FlowFazer.  There is also a cool Java applet of a similar concept",
"*hacks.flag.documentation:						\
This draws a waving colored flag, that undulates its way around the	\
screen.	 The trick is the flag can contain arbitrary text and images.	\
By default, it displays either the current system name and OS		\
type, or a picture of ``Bob,'' but you can replace the text or the	\
image with a command-line option.  Written by Charles Vidal and Jamie	\
Zawinski.",
"*hacks.sphere.documentation:						\
Another of the classic screenhacks of the distant past, this one	\
draws shaded spheres in multiple colors.  This hack traces its		\
lineage back to Tom Duff in 1982.",
"*hacks.forest.documentation:						\
This draws fractal trees.  Written by Peter Baumung.  Everybody loves	\
fractals, right?",
"*hacks.lisa.documentation:						\
This draws Lisajous loops, by Caleb Cullen.  Remember that device	\
they had the Phantom Zone prisoners in during their trial in		\
Superman?  I think that was one of these.",
"*hacks.lissie.documentation:						\
Another Lissajous figure.  This one draws the progress of circular	\
shapes along a path.  Written by Alexander Jolk.",
"*hacks.goop.documentation:						\
This draws set of animating, transparent, amoeba-like blobs.  The	\
blobs change shape as they wander around the screen, and they are	\
translucent, so you can see the lower blobs through the higher ones,	\
and when one passes over another, their colors merge.  Written by	\
Jamie Zawinski.	 I got the idea for this from a cool mouse pad I	\
have, which achieves the same kind of effect in real life by having	\
several layers plastic with colored oil between them.  Written by	\
Jamie Zawinski.",
"*hacks.starfish.documentation:						\
This generates a sequence of undulating, throbbing, star-like		\
patterns which pulsate, rotate, and turn inside out.  Another display	\
mode uses these shapes to lay down a field of colors, which are then	\
cycled.	 The motion is very organic.  Written by Jamie Zawinski.",
"*hacks.munch.documentation:					      \\n\
        DATAI 2							      \\n\
        ADDB 1,2						      \\n\
        ROTC 2,-22						      \\n\
        XOR 1,2							      \\n\
        JRST .-4						      \\n\
								      \\n\
As reported by HAKMEM, in 1962, Jackson Wright wrote the above PDP-1	\
code. That code still lives on in this screenhack, some 35 years	\
later.  The number of lines of enclosing code has increased		\
substantially, however.  This version is by Tim Showalter.",
"*hacks.fadeplot.name: FadePlot",
"*hacks.fadeplot.documentation:						\
Draws what looks like a waving ribbon following a sinusoidal path.	\
Written by Bas van Gaalen and Charles Vidal.",
"*hacks.coral.documentation:						\
Simulates coral growth, albeit somewhat slowly.	 This image doesn't	\
really do it justice.  Written by Frederick Roeber.",
"*hacks.mountain.documentation:						\
Generates random 3d plots that look vaguely mountainous.  Written by	\
Pascal Pensa.",
"*hacks.triangle.documentation:						\
Generates random mountain ranges using iterative subdivision of		\
triangles.  Written by Tobias Gloth.",
"*hacks.worm.documentation:						\
An ancient xlock hack that draws multicolored worms that crawl around	\
the screen.  Written by Brad Taylor, Dave Lemke, Boris Putanec, and	\
Henrik Theiling.",
"*hacks.rotor.documentation:						\
Another ancient xlock demo, this one by Tom Lawrence.  It draws a	\
line segment moving along a complex spiraling curve.  I tweaked this	\
to generate curvier lines, but still frames of it don't look like	\
much.",
"*hacks.ant.documentation:						\
A cellular automaton that is really a two-dimensional Turing machine:	\
as the heads (``ants'') walk along the screen, they change pixel	\
values in their path.  Then, as they pass over changed pixels, their	\
behavior is influenced.	 Written by David Bagley.",
"*hacks.demon.documentation:						\
A cellular automaton that starts with a random field, and organizes	\
it into stripes and spirals.  Written by David Bagley.",
"*hacks.loop.documentation:						\
This one produces loop-shaped colonies that spawn, age, and		\
eventually die.	 Written by David Bagley.",
"*hacks.vines.documentation:						\
This one generates a continuous sequence of small, curvy geometric	\
patterns.  It scatters them around your screen until it fills up,	\
then it clears the screen and starts over.  Written by Tracy Camp and	\
David Hansen.",
"*hacks.kaleidescope.documentation:					\
Another clone of an ancient meme, consisting largely of frenetic	\
rotational motion of colored lines.  This one is by Ron Tapia.	The	\
motion is nice, but I think it needs more solids, or perhaps just	\
brighter colors.  More variations in the rotational speed might help,	\
too.",
"*hacks.xjack.documentation:						\
This program behaves schizophrenically and makes a lot of typos.	\
Written by Jamie Zawinski.  If you haven't seen Stanley Kubrick's	\
masterpiece, ``The Shining,'' you won't get it.	 Those who have		\
describe this hack as ``inspired.''",
"*hacks.xlyap.documentation:						\
This generates pretty fractal pictures by doing funky math involving	\
the ``Lyapunov exponent.''  It has a cool interactive mode, too.	\
Written by Ron Record.",
"*hacks.cynosure.documentation:						\
A hack similar to `greynetic', but less frenetic.  The first		\
implementation was by Stephen Linhart; then Ozymandias G. Desiderata	\
wrote a Java applet clone.  That clone was discovered by Jamie		\
Zawinski, and ported to C for inclusion here.",
"*hacks.flow.documentation:						\
Another series of strange attractors: a flowing series of points,	\
making strange rotational shapes.  Written by Jeff Butterworth.",
"*hacks.epicycle.documentation:						\
This program draws the path traced out by a point on the edge of a	\
circle.	 That circle rotates around a point on the rim of another	\
circle, and so on, several times. These were the basis for the		\
pre-heliocentric model of planetary motion.  Written by James		\
Youngman.",
"*hacks.interference.documentation:					\
Another color-field hack, this one works by computing decaying		\
sinusoidal waves, and allowing them to interfere with each other as	\
their origins move.  Written by Hannu Mallat.",
"*hacks.truchet.documentation:						\
This draws line- and arc-based Truchet patterns that tile the screen.	\
Written by Adrian Likins.",
"*hacks.bsod.name: BSOD",
"*hacks.bsod.documentation:						\
BSOD stands for ``Blue Screen of Death.''  The finest in personal	\
computer emulation, this hack simulates popular screen savers from a	\
number of less robust operating systems.  Written by Jamie Zawinski.",
"*hacks.crystal.documentation:						\
Moving polygons, similar to a kaleidescope (more like a kaleidescope	\
than the hack called `kaleid,' actually.) This one by Jouk Jansen.",
"*hacks.discrete.documentation:						\
More ``discrete map'' systems, including new variants of Hopalong and	\
Julia, and a few others.  Written by Tim Auckland.",
"*hacks.kumppa.documentation:						\
Spiraling, spinning, and very, very fast splashes of color rush		\
toward the screen.  Written by Teemu Suutari.",
"*hacks.rd-bomb.name: RD-Bomb",
"*hacks.rd-bomb.documentation:						\
Another variation of the `Bomb' program by Scott Draves.  This draws	\
a grid of growing square-like shapes that, once they overtake each	\
other, react in unpredictable ways.  ``RD'' stands for			\
reaction-diffusion.",
"*hacks.sonar.documentation:						\
This program draws a simulation of a sonar screen.  Written by		\
default, it displays a random assortment of ``bogies'' on the screen,	\
but if compiled properly, it can ping (pun intended) your local		\
network, and actually plot the proximity of the other hosts on your	\
network to you.	 It would be easy to make it monitor other sources of	\
data, too.  (Processes?	 Active network connections?  CPU usage per	\
user?)	Written by Stephen Martin.",
"*hacks.t3d.name: T3D",
"*hacks.t3d.documentation:						\
This draws a working analog clock composed of floating, throbbing	\
bubbles.  Written by Bernd Paysan.",
"*hacks.penetrate.documentation:						\
This hack simulates the classic arcade game Missile Command.  Written	\
by Adam Miller.",
"*hacks.deluxe.documentation:						\
This draws a pulsing sequence of stars, circles, and lines.  It would	\
look better if it was faster, but as far as I can tell, there is no	\
way to make this be both: fast, and flicker-free.  Yet another reason	\
X sucks.  Written by Jamie Zawinski.",
"*hacks.compass.documentation:						\
This draws a compass, with all elements spinning about randomly, for	\
that ``lost and nauseous'' feeling. Written by Jamie Zawinski.",
"*hacks.squiral.documentation:						\
Draws a set of interacting, square-spiral-producing automata.  The	\
spirals grow outward until they hit something, then they go around	\
it.  Written by Jeff Epler.",
"*hacks.xflame.documentation:						\
Draws a simulation of pulsing fire.  It can also take an arbitrary	\
image and set it on fire too.  Written by Carsten Haitzler, hacked on	\
by many others.",
"*hacks.wander.documentation:						\
Draws a colorful random-walk, in various forms.	 Written by Rick	\
Campbell.",
"*hacks.critical.documentation:						\
Draws a system of self-organizing lines.  It starts out as random	\
squiggles, but after a few iterations, order begins to appear.		\
Written by Martin Pool.",
"*hacks.phosphor.documentation:						\
Draws a simulation of an old terminal, with large pixels and		\
long-sustain phosphor. It can run any program as a source of the text	\
it displays.  Written by Jamie Zawinski.",
"*hacks.xmatrix.documentation:						\
A rendition of the text scrolls seen in the movie ``The Matrix.''	\
Written by Jamie Zawinski.",
"*hacks.petri.documentation:						\
This simulates colonies of mold growing in a petri dish.  Growing	\
colored circles overlap and leave spiral interference in their wake.	\
Written by Dan Bornstein.",
"*hacks.shadebobs.name: ShadeBobs",
"*hacks.shadebobs.documentation:						\
This draws smoothly-shaded oscilating oval patterns, that look		\
something like vapor trails or neon tubes.  Written by Shane Smit.",
"*hacks.ccurve.name: C Curve",
"*hacks.ccurve.documentation:						\
Generates self-similar linear fractals, including the classic ``C	\
Curve.''  Written by Rick Campbell.",
"*hacks.blaster.documentation:						\
Draws a simulation of flying space-combat robots (cleverly disguised	\
as colored circles) doing battle in front of a moving star field.	\
Written by Jonathan Lin.",
"*hacks.bumps.documentation:						\
A bit like `Spotlight', except that instead of merely exposing part	\
of your desktop, it creates a bump map from it.	 Basically, it		\
3D-izes a roaming section of your desktop, based on color intensity.	\
Written by Shane Smit.",
"*hacks.xteevee.name: XTeeVee",
"*hacks.xteevee.documentation:						\
XTeeVee simulates various television problems, including static,	\
loss of vertical hold, and a test pattern.  By Greg Knauss.",
"*hacks.xspirograph.name: XSpiroGraph",
"*hacks.xspirograph.documentation:					\
Simulates that pen-in-nested-plastic-gears toy from your childhood.     \
By Rohit Singh.",
"*hacks.webcollage.name: WebCollage",
"*hacks.webcollage.documentation:					\
This program makes collages out of random images pulled off of the	\
World Wide Web.	 It finds these images by doing random web searches,	\
and then extracting images from the returned pages.  It can also be	\
set up to filter the images through the `VidWhacker' program, above,	\
which looks really great.						\
								    \\n\\n\
(Note that most of the images it finds are text, and not pictures.	\
This is because most of the web is pictures of text.  Which is pretty	\
sad.)  Written by Jamie Zawinski.",
"*hacks.vidwhacker.name: VidWhacker",
"*hacks.vidwhacker.documentation:					\
This is actually just a shell script that grabs a frame of video from	\
the system's video input, and then uses some PBM filters (chosen at	\
random) to manipulate and recombine the video frame in various ways	\
(edge detection, subtracting the image from a rotated version of	\
itself, etc.)  Then it displays that image for a few seconds, and	\
does it again.	This works really well if you just feed broadcast	\
television into it.							\
								    \\n\\n\
Currently, the three lines of the script that actually grab the		\
source picture are SGI specific, but it should be trivial to adapt it	\
to work on other systems that can grab video (please send me the	\
changes if you do this...)",
"*hacks.rocks.documentation:						\
This draws an animation of flight through an asteroid field, with	\
changes in rotation and direction.  It can also display 3D		\
separations for red/blue glasses!  Mostly written by Jamie Zawinski.",
"*hacks.bubbles.documentation:						\
This simulates the kind of bubble formation that happens when water	\
boils:small bubbles appear, and as they get closer to each other,	\
they combine to form larger bubbles, which eventually pop.  Written	\
by James Macnicol.",
"*hacks.gears.documentation:						\
This draws a set of turning, interlocking gears, rotating in three	\
dimensions.  Another GL hack, by Danny Sung, Brian Paul, and Ed		\
Mackey.",
"*hacks.superquadrics.documentation:					\
Ed Mackey reports that he wrote the first version of this program in	\
BASIC on a Commodore 64 in 1987, as a 320x200 black and white		\
wireframe.  Now it is GL and has specular reflections.",
"*hacks.morph3d.name: Morph3D",
"*hacks.morph3d.documentation:						\
Another 3d shape-changing GL hack, by Marcelo Vianna.  It has the	\
same shiny-plastic feel as Superquadrics, as many computer-generated	\
objects do...",
"*hacks.cage.documentation:						\
This draws Escher's ``Impossible Cage,'' a 3d analog of a moebius	\
strip, and rotates it in three dimensions.  Written by Marcelo		\
Vianna.",
"*hacks.moebius.documentation:						\
Another M.  C.	Escher hack by Marcelo Vianna, this one draws		\
``Moebius Strip II,'' a GL image of ants walking along the surface of	\
a moebius strip.",
"*hacks.stairs.documentation:						\
by Marcelo Vianna's third Escher GL hack, this one draws an		\
``infinite'' staircase.",
"*hacks.pipes.documentation:						\
If you've ever been in the same room with a Windows NT machine,		\
you've probably seen this GL hack.  This version is by Marcelo		\
Vianna.",
"*hacks.sproingies.documentation:					\
Q-Bert meets Marble Madness!  Written by Ed Mackey.",
"*hacks.rubik.documentation:						\
Draws a Rubik's Cube that rotates in three dimensions and repeatedly	\
shuffles and solves itself.  Another fine GL hack by Marcelo Vianna.",
"*hacks.atlantis.documentation:						\
This is xfishtank writ large: a GL animation of a number of sharks,	\
dolphins, and whales.  The swimming motions are great. Originally	\
written by Mark Kilgard.",
"*hacks.lament.documentation:						\
Animates a simulation of Lemarchand's Box, repeatedly solving itself.	\
Requires OpenGL, and a machine with fast hardware support for texture	\
maps.  Warning: occasionally opens doors.  Written by Jamie Zawinski.",
"*hacks.bubble3d.name: Bubble3D",
"*hacks.bubble3d.documentation:						\
Draws a stream of rising, undulating 3D bubbles, rising toward the	\
top of the screen, with nice specular reflections. Written by Richard	\
Jones.",
"*hacks.glplanet.name: GLPlanet",
"*hacks.glplanet.documentation:						\
Draws a planet bouncing around in space.  Written by David Konerding.	\
The built-in image is a map of the earth (extracted from `xearth'),	\
but you can wrap any texture around the sphere, e.g., the planetary	\
textures that come with `ssystem'.",
"*hacks.pulsar.documentation:						\
Draws some intersecting planes, making use of alpha blending, fog,	\
textures, and mipmaps, plus a ``frames per second'' meter so that you	\
can tell how fast your graphics card is...  Requires OpenGL.  Written	\
by David Konerding.",
"*hacks.extrusion.documentation:						\
Draws various rotating extruded shapes that twist around, lengthen,	\
and turn inside out.  Created by David Konerding from the samples	\
that come with the GL Extrusion library by Linas Vepstas.",
"*hacks.sierpinski3d.name: Sierpinski3D",
"*hacks.sierpinski3d.documentation:					\
This draws the three-dimensional variant of the recursive Sierpinski	\
triangle fractal, using GL.  Written by Tim Robinson and Jamie Zawinski.",
"*hacks.ripples.documentation:						\
This draws rippling interference patterns like splashing water.		\
With the -water option, it manipulates your desktop image to look	\
like something is dripping into it.  Written by Tom Hammersley.",
"*hacks.xdaliclock.name: XDaliClock",
"*hacks.xdaliclock.documentation:					\
XDaliClock draws a large digital clock, the numbers of which change by	\
``melting'' into their new shapes.  Written by Jamie Zawinski.  This	\
is not included with the XScreenSaver package, but if you don't have	\
it already, you can find it at <http://www.jwz.org/xdaliclock/>.",
"*hacks.xearth.documentation:						\
XEarth draws an image of the Earth, as seen from your favorite vantage	\
point in space, correctly shaded for the current position of the Sun.	\
Written by Kirk Johnson.  This is not included with the XScreenSaver	\
package, but if you don't have it already, you can find it at		\
<http://www.cs.colorado.edu/~tuna/xearth/>.",
"*hacks.ssystem.name: SSystem",
"*hacks.ssystem.documentation:						\
SSystem is a GL Solar System simulator.  It simulates flybys of Sun,	\
the nine planets and a few major satellites, with four camera modes.	\
Written by Raul Alonso.  This is not included with the XScreenSaver	\
package, but if you don't have it already, you can find it at		\
<http://www1.las.es/~amil/ssystem/>.",
"*hacks.xmountains.documentation:					\
XMountains generates realistic-looking fractal terrains of snow-capped	\
mountains near water, with either a top view or a side view.		\
Written by Stephen Booth.  This is not included with the XScreenSaver	\
package, but if you don't have it already, you can find it at		\
<http://www.epcc.ed.ac.uk/~spb/xmountains/>.			        \
								    \\n\\n\
Be sure to compile it with -DVROOT or it won't work right when launched	\
by the xscreensaver daemon.",
"*hacks.xaos.name: XaoS",
"*hacks.xaos.documentation:						\
XaoS generates fast fly-through animations of the Mandelbrot and other	\
fractal sets.  Written by Thomas Marsh and Jan Hubicka.    This is not	\
included with the XScreenSaver package, but if you don't have it	\
already, you can find it at <http://limax.paru.cas.cz/~hubicka/XaoS/>.",
"*hacks.xfishtank.name: XFishTank",
"*hacks.xfishtank.documentation:						\
Fish!  This is not included with the XScreenSaver package, but if you	\
don't have it already, you can find it at                               \
<http://metalab.unc.edu/pub/Linux/X11/demos/>.",
"*hacks.xsnow.documentation:						\
Draws falling snow and the occasional tiny Santa.  By Rick Jansen.      \
You can find it at <http://zoutmijn.bpa.nl/rick/Xsnow/>.",
"*hacks.goban.documentation:						\
Replays historical games of go (aka wei-chi and baduk) on the screen.   \
By Scott Draves.  You can find it at <http://www.draves.org/goban/>.",
"*hacks.electricsheep.name: ElectricSheep",
"*hacks.electricsheep.documentation:					\
ElectricSheep is an xscreensaver module that displays mpeg video of	\
an animated fractal flame.  In the background, it contributes render	\
cycles to the next animation.  Periodically it uploades completed	\
frames to the server, where they are compressed for distribution to	\
all clients.								\
								    \\n\\n\
This program is recommended only if you have a high bandwidth		\
connection to the Internet.						\
								    \\n\\n\
By Scott Draves.  You can find it at <http://www.electricsheep.org/>.   \
See that web site for configuration information.",
