"*timeout:		10",
"*cycle:			10",
"*lockTimeout:		0",
"*passwdTimeout:		30",
"*nice:			10",
"*lock:			False",
"*verbose:		False",
"*fade:			True",
"*unfade:		False",
"*fadeSeconds:		3",
"*fadeTicks:		20",
"*captureStderr: 	True",
"*captureStdout: 	True",
"*textForeground:	Yellow",
"*textBackground:	Black",
"*overlayStderr:		True",
"*font:			*-medium-r-*-140-*-m-*",
"*installColormap:	True",
"*programs:	qix -root						\\n\
		qix -root -solid -delay 0 -segments 100			\\n\
		qix -root -linear -count 10 -size 100 -segments 200	\\n\
		attraction -root -mode balls				\\n\
		attraction -root -mode lines -points 3 -segments 200	\\n\
		attraction -root -mode splines -segments 300		\\n\
		attraction -root -mode lines -radius 300		  \
			-orbit -vmult 0.5				\\n\
		pyro -root						\\n\
		helix -root						\\n\
		pedal -root						\\n\
		rorschach -root -offset 7				\\n\
		hopalong -root						\\n\
		greynetic -root						\\n\
		xroger -root						\\n\
		imsmap -root						\\n\
		slidescreen -root					\\n\
		decayscreen -root					\\n\
		hypercube -root						\\n\
		halo -root						\\n\
		maze -root						\\n\
		noseguy -root						\\n\
		flame -root						\\n\
		lmorph -root						\\n\
		deco -root						\\n\
		moire -root						\\n\
		kaleidescope -root					\\n\
		lightning -root						\\n\
		strange -root						\\n\
		fract -root						\\n\
		spiral -root						\\n\
		laser -root						\\n\
		grav -root						\\n\
		grav -root -trail -decay				\\n\
		drift -root						\\n\
		ifs -root						\\n\
		julia -root						\\n\
		penrose -root						\\n\
		sierpinski -root					\\n\
		braid -root						\\n\
		galaxy -root						\\n\
		slip -root						\\n\
		bouboule -root						\\n\
		swirl -root						\\n\
		flag -root						\\n\
		sphere -root						\\n\
		forest -root						\\n\
		lisa -root						\\n\
		goop -root						\\n\
		starfish -root						\\n\
		starfish -root -blob					\\n\
		munch -root						\\n\
		fadeplot -root						\\n\
		coral -root						\\n\
									  \
	mono:	rocks -root						\\n\
	color:	rocks -root -fg darksalmon				\\n\
									  \
	mono:	qix -root -linear -count 5 -size 200 -spread 30		  \
			-segments 75 -solid -xor			\\n\
									  \
	color:	attraction -root -mode polygons				\\n\
	color:	attraction -root -mode filled-splines -segments 0	\\n\
	color:	attraction -root -glow -points 10			\\n\
	color:	bubbles -root						\\n\
									  \
        color:  rd-bomb -root						\\n\
        color:  rd-bomb -root -speed 1 -size 0.1			\\n\
									  \
  PseudoColor:	qix -root -count 4 -solid -transparent			\\n\
  PseudoColor:	qix -root -count 5 -solid -transparent -linear		  \
			-segments 250 -size 100				\\n\
									  \
		gears -root					  	\\n\
		superquadrics -root				  	\\n\
		morph3d -root					  	\\n\
		escher -root					  	\\n\
		pipes -root					  	\\n\
		sproingies -root				  	\\n\
		rubik -root					  	\\n",
" ",
"*fontList:                       *-helvetica-medium-r-*-*-*-120-*-*-*-iso8859-1",
"*demoDialog*label1.fontList:     *-helvetica-medium-r-*-*-*-140-*-*-*-iso8859-1",
"*passwdDialog*fontList:          *-helvetica-medium-r-*-*-*-140-*-*-*-iso8859-1",
"*XmList.fontList:                  *-courier-medium-r-*-*-*-120-*-*-*-iso8859-1",
"*XmTextField.fontList:             *-courier-medium-r-*-*-*-120-*-*-*-iso8859-1",
"*passwdDialog.passwdText.fontList: *-courier-medium-r-*-*-*-120-*-*-*-iso8859-1",
"*XmDialogShell*foreground:		black",
"*XmDialogShell*background:		gray90",
"*XmDialogShell*XmTextField.foreground:	black",
"*XmDialogShell*XmTextField.background:	white",
"*XmDialogShell*demoList.foreground:	black",
"*XmDialogShell*demoList.background:	white",
"*XmDialogShell*rogerLabel.foreground:	red3",
"*XmDialogShell*rogerLabel.background:	white",
"*XmDialogShell.title:		XScreenSaver",
"*allowShellResize:		True",
"*autoUnmanage:			False",
"*demoDialog.maxWidth:		600",
"*label1.labelString:		XScreenSaver %s",
"*label1.label:			XScreenSaver %s",
"*label2.labelString: Copyright � 1991-1997 by Jamie Zawinski <jwz@netscape.com>",
"*label2.label:	     Copyright � 1991-1997 by Jamie Zawinski <jwz@netscape.com>",
"*demoList.visibleItemCount:	10",
"*demoList.automaticSelection:	True",
"*next.labelString:		Run Next",
"*prev.labelString:		Run Previous",
"*edit.labelString:		Edit Parameters",
"*done.labelString:		Exit Demo Mode",
"*restart.labelString:		Reinitialize",
"*resourcesLabel.labelString:	XScreenSaver Parameters",
"*timeoutLabel.labelString:	Saver Timeout",
"*cycleLabel.labelString:	Cycle Timeout",
"*fadeSecondsLabel.labelString:	Fade Duration",
"*fadeTicksLabel.labelString:	Fade Ticks",
"*lockLabel.labelString:		Lock Timeout",
"*passwdLabel.labelString:	Password Timeout",
"*resourcesForm*XmTextField.columns:	8",
"*verboseToggle.labelString:	Verbose",
"*cmapToggle.labelString:	Install Colormap",
"*fadeToggle.labelString:	Fade Colormap",
"*unfadeToggle.labelString:	Unfade Colormap",
"*lockToggle.labelString:	Require Password",
"*resourcesDone.labelString:	Done",
"*resourcesCancel.labelString:	Cancel",
"*passwdDialog.title:		Password",
"*passwdLabel1.labelString:	XScreenSaver %s",
"*passwdLabel2.labelString:	This display is locked.",
"*passwdLabel3.labelString:	Please type %s's password to unlock it.",
"*passwdDone.labelString:	Done",
"*passwdCancel.labelString:	Cancel",
"*passwdLabel1.alignment:	ALIGNMENT_BEGINNING",
"*passwdLabel2.alignment:	ALIGNMENT_BEGINNING",
"*passwdLabel3.alignment:	ALIGNMENT_BEGINNING",
"*rogerLabel.width:		150",
"*demo_dialog*font:		*-helvetica-bold-r-*-*-*-120-*-*-*-iso8859-1",
"*resources_dialog*font:		*-helvetica-bold-r-*-*-*-120-*-*-*-iso8859-1",
"*passwd_dialog*font:		*-helvetica-bold-r-*-*-*-120-*-*-*-iso8859-1",
"*demo_dialog*label1.font:	*-helvetica-bold-r-*-*-*-140-*-*-*-iso8859-1",
"*resources_dialog*label1.font:	*-helvetica-bold-r-*-*-*-140-*-*-*-iso8859-1",
"*demo_dialog*List.font:		*-courier-medium-r-*-*-*-120-*-*-*-iso8859-1",
"*passwd_dialog*passwd_form.value*font:		*nil*",
"*demo_dialog*foreground:			black",
"*demo_dialog*background:			gray90",
"*demo_dialog*List.background:			white",
"*demo_dialog*Scrollbar.background:		gray85",
"*demo_dialog*Command.background:		gray85",
"*resources_dialog*foreground:			black",
"*resources_dialog*background:			gray90",
"*resources_dialog*Command.background:		gray85",
"*resources_dialog*Toggle.background:		gray85",
"*resources_dialog*Text*background:		white",
"*resources_dialog*Dialog.value.translations: #override\\n\
	<Key>Return: beginning-of-line()\\n",
"*passwd_dialog*foreground:			black",
"*passwd_dialog*background:			gray90",
"*passwd_dialog*Text*background:			white",
"*demo_dialog*viewport.width:			400",
"*demo_dialog*viewport.height:			200",
"*Form.borderWidth:				0",
"*Box.borderWidth:				0",
"*Label.borderWidth:				0",
"*resources_dialog*Dialog.borderWidth:		0",
"*demo_dialog*next.label:			Run Next",
"*demo_dialog*prev.label:			Run Previous",
"*demo_dialog*edit.label:			Edit Parameters",
"*demo_dialog*done.label:			Exit Demo Mode",
"*demo_dialog*restart.label:			Reinitialize",
"*resources_dialog*timeout.label:		Saver Timeout:",
"*resources_dialog*cycle.label:			Cycle Timeout:",
"*resources_dialog*fade.label:			Fade Duration:",
"*resources_dialog*ticks.label:			Fade Ticks:",
"*resources_dialog*lockTime.label:		Lock Timeout:",
"*resources_dialog*passwdTime.label:		Password Timeout:",
"*resources_dialog*label1.label:			XScreenSaver Parameters",
"*resources_dialog*buttonbox.verbose.label:	Verbose",
"*resources_dialog*buttonbox.cmap.label:		Install Colormap",
"*resources_dialog*buttonbox.fade.label:		Fade Colormap",
"*resources_dialog*buttonbox.unfade.label:	Unfade Colormap",
"*resources_dialog*buttonbox.lock.label:		Require Password",
"*resources_dialog*done.label:			Done",
"*resources_dialog*cancel.label:			Cancel",
"*passwd_dialog*label1.label:			XScreenSaver %s",
"*passwd_dialog*label2.label:			This display is locked.",
"*passwd_dialog*label3.label:		Please type %s's password to unlock it.",
"*passwd_dialog*ok.label:			Done",
"*passwd_dialog*cancel.label:			Cancel",
"*passwd_dialog*passwd_form*label.label:		Enter password:",
"*passwd_dialog*Dialog.label:			Enter password:",
"*passwd_dialog*passwd_form*Text.width:		200",
"*passwd_dialog*roger.width:			150",
"*passwd_dialog*roger.height:			150",
"*passwd_dialog*roger.foreground:		red3",
"*passwd_dialog*roger.background:		white",
"*passwd_dialog*roger.borderWidth:		1",
"*pointerPollTime:	5",
"*initialDelay:		30",
"*windowCreationTimeout:	30",
"*bourneShell:		/bin/sh",
