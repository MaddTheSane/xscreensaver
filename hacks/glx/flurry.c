/* -*- Mode: C; tab-width: 4 c-basic-offset: 4 indent-tabs-mode: t -*- */
/*
 * vim: ts=8 sw=4 noet
 */

/*

Copyright (c) 2002, Calum Robinson
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the author nor the names of its contributors may be used
  to endorse or promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* flurry */

#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)flurry.c	4.07 97/11/24 xlockmore";

#endif

/*-
 * due to a Bug/feature in VMS X11/Intrinsic.h has to be placed before xlock.
 * otherwise caddr_t is not defined correctly
 */

#define DEF_PRESET     "classic"
#define DEF_BRIGHTNESS "8"

#include <X11/Intrinsic.h>

# define PROGCLASS		"Flurry"
# define HACK_INIT		init_flurry
# define HACK_DRAW		draw_flurry
# define HACK_RESHAPE		reshape_flurry
# define HACK_HANDLE_EVENT	flurry_handle_event
# define EVENT_MASK		PointerMotionMask
# define flurry_opts		xlockmore_opts
# define DEFAULTS		"*showFPS:      False   \n" \
				"*preset:	" DEF_PRESET "   \n"

# include "xlockmore.h"		/* from the xscreensaver distribution */

#ifdef USE_GL

#include "rotator.h"
#include "gltrackball.h"

static char *preset_str;

static XrmOptionDescRec opts[] = {
    { "-preset",     ".preset",     XrmoptionSepArg, 0 }
};

static argtype vars[] = {
    {(caddr_t *) &preset_str, "preset",     "Preset",     DEF_PRESET,     t_String},
};

#define countof(x) (sizeof((x))/sizeof((*x)))

ModeSpecOpt flurry_opts = {countof(opts), opts, countof(vars), vars, NULL};

#ifdef USE_MODULES
ModStruct   flurry_description = {
    "flurry",
    "init_flurry",
    "draw_flurry",
    "release_flurry",
    "draw_flurry",
    "init_flurry",
    NULL,
    &flurry_opts,
    1000, 1, 2, 1, 4, 1.0,
    "",
    "Flurry",
    0,
    NULL
};

#endif

#include <sys/time.h>
#include <sys/sysctl.h>

#include "flurry.h"

global_info_t *flurry_info = NULL;

static double gTimeCounter = 0.0;

double currentTime(void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

void OTSetup (void) {
    if (gTimeCounter == 0.0) {
        gTimeCounter = currentTime();
    }
}

double TimeInSecondsSinceStart (void) {
    return currentTime() - gTimeCounter;
}

#ifdef __ppc__
static int IsAltiVecAvailable(void)
{
    return 0;
}
#endif

void delete_flurry_info(flurry_info_t *flurry)
{
    int i;

    free(flurry->s);
    free(flurry->star);
    for (i=0;i<MAX_SPARKS;i++)
    {
	free(flurry->spark[i]);
    }
    free(flurry);
}

flurry_info_t *new_flurry_info(global_info_t *global, int streams, ColorModes colour, float thickness, float speed, double bf)
{
    int i,k;
    flurry_info_t *flurry = (flurry_info_t *)malloc(sizeof(flurry_info_t));

    if (!flurry) return NULL;

    flurry->flurryRandomSeed = RandFlt(0.0, 300.0);

    flurry->fOldTime = TimeInSecondsSinceStart() + flurry->flurryRandomSeed;

    flurry->numStreams = streams;
    flurry->streamExpansion = thickness;
    flurry->currentColorMode = colour;
    flurry->briteFactor = bf;

    flurry->s = malloc(sizeof(SmokeV));
    InitSmoke(flurry->s);

    flurry->star = malloc(sizeof(Star));
    InitStar(flurry->star);
    flurry->star->rotSpeed = speed;

    for (i = 0;i < MAX_SPARKS; i++)
    {
	flurry->spark[i] = malloc(sizeof(Spark));
	InitSpark(flurry->spark[i]);
	flurry->spark[i]->mystery = 1800 * (i + 1) / 13; /* 100 * (i + 1) / (flurry->numStreams + 1); */
	UpdateSpark(global, flurry, flurry->spark[i]);
    }

    for (i=0;i<NUMSMOKEPARTICLES/4;i++) {
	for(k=0;k<4;k++) {
	    flurry->s->p[i].dead.i[k] = 1;
	}
    }

    flurry->next = NULL;

    return flurry;
}

void GLSetupRC(global_info_t *global)
{
    /* setup the defaults for OpenGL */
    glDisable(GL_DEPTH_TEST);
    glAlphaFunc(GL_GREATER,0.0f);
    glEnable(GL_ALPHA_TEST);
    glShadeModel(GL_FLAT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    glViewport(0,0,(int) global->sys_glWidth,(int) global->sys_glHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0,global->sys_glWidth,0,global->sys_glHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0,0.0,0.0,1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnableClientState(GL_COLOR_ARRAY);	
    glEnableClientState(GL_VERTEX_ARRAY);	
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
#ifdef __ppc__
    global->optMode = OPT_MODE_SCALAR_FRSQRTE;

#ifdef __VEC__
    if (IsAltiVecAvailable()) global->optMode = OPT_MODE_VECTOR_UNROLLED;
#endif

#else
    global->optMode = OPT_MODE_SCALAR_BASE;
#endif
}

void GLRenderScene(global_info_t *global, flurry_info_t *flurry, double b)
{
    int i;

    flurry->dframe++;

    flurry->fOldTime = flurry->fTime;
    flurry->fTime = TimeInSecondsSinceStart() + flurry->flurryRandomSeed;
    flurry->fDeltaTime = flurry->fTime - flurry->fOldTime;

    flurry->drag = (float) pow(0.9965,flurry->fDeltaTime*85.0);

    UpdateStar(global, flurry, flurry->star);

#ifdef DRAW_SPARKS
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
#endif

    for (i=0;i<flurry->numStreams;i++) {
	flurry->spark[i]->color[0]=1.0;
	flurry->spark[i]->color[1]=1.0;
	flurry->spark[i]->color[2]=1.0;
	flurry->spark[i]->color[2]=1.0;
	UpdateSpark(global, flurry, flurry->spark[i]);
#ifdef DRAW_SPARKS
	DrawSpark(global, flurry, flurry->spark[i]);
#endif
    }

    switch(global->optMode) {
	case OPT_MODE_SCALAR_BASE:
	    UpdateSmoke_ScalarBase(global, flurry, flurry->s);
	    break;
#ifdef __ppc__
	case OPT_MODE_SCALAR_FRSQRTE:
	    UpdateSmoke_ScalarFrsqrte(global, flurry, flurry->s);
	    break;
#endif
#ifdef __VEC__
	case OPT_MODE_VECTOR_SIMPLE:
	    UpdateSmoke_VectorBase(global, flurry, flurry->s);
	    break;
	case OPT_MODE_VECTOR_UNROLLED:
	    UpdateSmoke_VectorUnrolled(global, flurry, flurry->s);
	    break;
#endif
	default:
	    break;
    }

    /* glDisable(GL_BLEND); */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glEnable(GL_TEXTURE_2D);

    switch(global->optMode) {
	case OPT_MODE_SCALAR_BASE:
#ifdef __ppc__
	case OPT_MODE_SCALAR_FRSQRTE:
#endif
	    DrawSmoke_Scalar(global, flurry, flurry->s, b);
	    break;
#ifdef __VEC__
	case OPT_MODE_VECTOR_SIMPLE:
	case OPT_MODE_VECTOR_UNROLLED:
	    DrawSmoke_Vector(global, flurry, flurry->s, b);
	    break;
#endif
	default:
	    break;
    }    

    glDisable(GL_TEXTURE_2D);
}

void GLResize(global_info_t *global, float w, float h)
{
    global->sys_glWidth = w;
    global->sys_glHeight = h;
}

/* new window size or exposure */
void reshape_flurry(ModeInfo *mi, int width, int height)
{
    global_info_t *global = flurry_info + MI_SCREEN(mi);

    glXMakeCurrent(MI_DISPLAY(mi), global->window, *(global->glx_context));

    glViewport(0.0, 0.0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glFlush();

    GLResize(global, (float)width, (float)height);
}

Bool
flurry_handle_event (ModeInfo *mi, XEvent *event)
{
    global_info_t *global = flurry_info + MI_SCREEN(mi);

    if (event->xany.type == ButtonPress && event->xbutton.button & Button1) {
	global->button_down_p = True;
	gltrackball_start (global->trackball,
		event->xbutton.x, event->xbutton.y,
		MI_WIDTH (mi), MI_HEIGHT (mi));
	return True;
    }
    else if (event->xany.type == ButtonRelease && event->xbutton.button & Button1) {
	global->button_down_p = False;
	return True;
    }
    else if (event->xany.type == MotionNotify && global->button_down_p) {
	gltrackball_track (global->trackball,
		event->xmotion.x, event->xmotion.y,
		MI_WIDTH (mi), MI_HEIGHT (mi));
	return True;
    }

    return False;
}

void
init_flurry(ModeInfo * mi)
{
    int screen = MI_SCREEN(mi);
    int i;
    global_info_t *global;
    enum {
	PRESET_INSANE = -1,
        PRESET_WATER = 0,
        PRESET_FIRE,
        PRESET_PSYCHEDELIC,
        PRESET_RGB,
        PRESET_BINARY,
        PRESET_CLASSIC,
        PRESET_MAX
    } preset_num;

    if (flurry_info == NULL) {
	OTSetup();
	if ((flurry_info = (global_info_t *) calloc(MI_NUM_SCREENS(mi),
			sizeof (global_info_t))) == NULL)
	    return;
    }

    global = &flurry_info[screen];

    global->window = MI_WINDOW(mi);

    global->rot = make_rotator(1, 1, 1, 1, 0, True);
    global->trackball = gltrackball_init();

    global->flurry = NULL;

    if (!preset_str || !*preset_str) preset_str = DEF_PRESET;
    if (!strcmp(preset_str, "random")) {
        preset_num = random() % PRESET_MAX;
    } else if (!strcmp(preset_str, "water")) {
        preset_num = PRESET_WATER;
    } else if (!strcmp(preset_str, "fire")) {
        preset_num = PRESET_FIRE;
    } else if (!strcmp(preset_str, "psychedelic")) {
        preset_num = PRESET_PSYCHEDELIC;
    } else if (!strcmp(preset_str, "rgb")) {
        preset_num = PRESET_RGB;
    } else if (!strcmp(preset_str, "binary")) {
        preset_num = PRESET_BINARY;
    } else if (!strcmp(preset_str, "classic")) {
        preset_num = PRESET_CLASSIC;
    } else if (!strcmp(preset_str, "insane")) {
        preset_num = PRESET_INSANE;
    } else {
        fprintf(stderr, "%s: unknown preset %s\n", progname, preset_str);
        exit(1);
    }

    switch (preset_num) {
    case PRESET_WATER: {
	for (i = 0; i < 9; i++) {
	    flurry_info_t *flurry;

	    flurry = new_flurry_info(global, 1, blueColorMode, 100.0, 2.0, 2.0);
	    flurry->next = global->flurry;
	    global->flurry = flurry;
	}
        break;
    }
    case PRESET_FIRE: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 12, slowCyclicColorMode, 10000.0, 0.0, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	
        break;
    }
    case PRESET_PSYCHEDELIC: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 10, rainbowColorMode, 200.0, 2.0, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	
        break;
    }
    case PRESET_RGB: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 3, redColorMode, 100.0, 0.8, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	

	flurry = new_flurry_info(global, 3, greenColorMode, 100.0, 0.8, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	

	flurry = new_flurry_info(global, 3, blueColorMode, 100.0, 0.8, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;	
        break;
    }
    case PRESET_BINARY: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 16, tiedyeColorMode, 1000.0, 0.5, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;

	flurry = new_flurry_info(global, 16, tiedyeColorMode, 1000.0, 1.5, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;
        break;
    }
    case PRESET_CLASSIC: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 5, tiedyeColorMode, 10000.0, 1.0, 1.0);
	flurry->next = global->flurry;
	global->flurry = flurry;
        break;
    }
    case PRESET_INSANE: {
	flurry_info_t *flurry;

	flurry = new_flurry_info(global, 64, tiedyeColorMode, 1000.0, 0.5, 0.5);
	flurry->next = global->flurry;
	global->flurry = flurry;

        break;
    }
    default: {
        fprintf(stderr, "%s: unknown preset %s\n", progname, preset_str);
        exit(1);
    }
    } 

    if ((global->glx_context = init_GL(mi)) != NULL) {
	reshape_flurry(mi, MI_WIDTH(mi), MI_HEIGHT(mi));
	GLSetupRC(global);
    } else {
	MI_CLEARWINDOW(mi);
    }
}

void
draw_flurry(ModeInfo * mi)
{
    static int first = 1;
    static double oldFrameTime = -1;
    double newFrameTime;
    double deltaFrameTime = 0;
    double brite;
    GLfloat alpha;

    global_info_t *global = flurry_info + MI_SCREEN(mi);
    flurry_info_t *flurry;
    Display    *display = MI_DISPLAY(mi);
    Window      window = MI_WINDOW(mi);

    newFrameTime = currentTime();
    if (oldFrameTime == -1) {
	/* special case the first frame -- clear to black */
	alpha = 1.0;
    } else {
	/* 
	 * this clamps the speed at below 60fps and, here
	 * at least, produces a reasonably accurate 50fps.
	 * (probably part CPU speed and part scheduler).
	 *
	 * Flurry is designed to run at this speed; much higher
	 * than that and the blending causes the display to
	 * saturate, which looks really ugly.
	 */
	if (newFrameTime - oldFrameTime < 1/60.0) {
	    usleep(MAX_(1,(int)(20000 * (newFrameTime - oldFrameTime))));
	    return;

	}
	deltaFrameTime = newFrameTime - oldFrameTime;
	alpha = 5.0 * deltaFrameTime;
    }
    oldFrameTime = newFrameTime;

    if (alpha > 0.2) alpha = 0.2;

    if (!global->glx_context)
	return;

    if (first) {
	MakeTexture();
	first = 0;
    }
    glDrawBuffer(GL_BACK);
    glXMakeCurrent(display, window, *(global->glx_context));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0, 0.0, 0.0, alpha);
    glRectd(0, 0, global->sys_glWidth, global->sys_glHeight);

    brite = pow(deltaFrameTime,0.75) * 10;
    for (flurry = global->flurry; flurry; flurry=flurry->next) {
	GLRenderScene(global, flurry, brite * flurry->briteFactor);
    }

    if (mi->fps_p) do_fps (mi);

    glFinish();
    glXSwapBuffers(display, window);
}

void
release_flurry(ModeInfo * mi)
{
    if (flurry_info != NULL) {
	int screen;

	for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
	    global_info_t *global = &flurry_info[screen];
	    flurry_info_t *flurry;

	    if (global->glx_context) {
		glXMakeCurrent(MI_DISPLAY(mi), global->window, *(global->glx_context));
	    }

	    for (flurry = global->flurry; flurry; flurry=flurry->next) {
		delete_flurry_info(flurry);
	    }
	}
	(void) free((void *) flurry_info);
	flurry_info = NULL;
    }
    FreeAllGL(mi);
}

#endif
