/* -*- mode: C; tab-width: 4 -*-
 * Bumps, Copyright (c) 2002, 2006 Shane Smit <CodeWeaver@DigitalLoom.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * Module: "bumps.c"
 * Tab Size: 4
 *
 * Description:
 *  This is typical bump-mapping.  The actual bump map is generated by a screen
 *  grab.  The light source is represented by a spotlight of random color. This
 *  spotlight randomly traverses the bump map in a sinus pattern.
 *
 *  Essentially, it 3D-izes your desktop, based on color intensity.
 *
 * Modification History:
 *  [10/01/99] - Shane Smit: Creation
 *  [10/08/99] - Shane Smit: Port to C. (Ick)
 *  [03/08/02] - Shane Smit: New movement code.
 *  [09/12/02] - Shane Smit: MIT-SHM XImages.
 * 							 Thanks to Kennett Galbraith <http://www.Alpha-II.com/>
 * 							 for code optimization.
 */


#include <math.h>
#include <inttypes.h>
#include "screenhack.h"

#ifdef HAVE_XSHM_EXTENSION
#include "xshm.h"
#endif /* HAVE_XSHM_EXTENSION */


/* Defines: */
/* #define VERBOSE */
#define RANDOM() ((int) (random() & 0X7FFFFFFFL))

typedef unsigned char	BOOL;


/* Globals: */

static const char *bumps_defaults [] = {
  ".background: black",
  ".foreground: white",
  "*fpsSolid:	true",
  "*color:		random",
  "*colorcount:	64",
  "*delay:		30000",
  "*duration:	120",
  "*soften:		1",
  "*invert:		FALSE",
#ifdef __sgi    /* really, HAVE_READ_DISPLAY_EXTENSION */
  "*visualID:	Best",
#endif
#ifdef HAVE_XSHM_EXTENSION
  "*useSHM:		True",
#endif /* HAVE_XSHM_EXTENSION */
#ifdef USE_IPHONE
  "*ignoreRotation: True",
  "*rotateImages:   True",
#endif
  0
};

static XrmOptionDescRec bumps_options [] = {
  { "-color",		".color",		XrmoptionSepArg, 0 },
  { "-colorcount",	".colorcount",	XrmoptionSepArg, 0 },
  { "-duration",	".duration",	XrmoptionSepArg, 0 },
  { "-delay",		".delay",		XrmoptionSepArg, 0 },
  { "-soften",		".soften",		XrmoptionSepArg, 0 },
  { "-invert",		".invert",		XrmoptionNoArg, "TRUE" },
#ifdef HAVE_XSHM_EXTENSION
  { "-shm",			".useSHM",		XrmoptionNoArg, "True" },
  { "-no-shm",		".useSHM",		XrmoptionNoArg, "False" },
#endif /* HAVE_XSHM_EXTENSION */

  { 0, 0, 0, 0 }
};


/* This structure handles everything to do with the spotlight, and is designed to be
 * a member of TBumps. */
typedef struct
{
	uint8_t *aLightMap;
	uint16_t nFalloffDiameter, nFalloffRadius;
	uint16_t nLightDiameter, nLightRadius;
	float nAccelX, nAccelY;
	float nAccelMax;
	float nVelocityX, nVelocityY;
	float nVelocityMax;
	float nXPos, nYPos;
} SSpotLight;


/* The entire program's operation is contained within this structure. */
typedef struct
{
	/* XWindows specific variables. */
	Display *dpy;
	Window Win;
	Screen *screen;
        Pixmap source;
	GC GraphicsContext;
	XColor *xColors;
	unsigned long *aColors;
	XImage *pXImage;
#ifdef HAVE_XSHM_EXTENSION
	XShmSegmentInfo XShmInfo;
	Bool	bUseShm;
#endif /* HAVE_XSHM_EXTENSION */

	uint8_t nColorCount;				/* Number of colors used. */
	uint8_t bytesPerPixel;
	uint16_t iWinWidth, iWinHeight;
	uint16_t *aBumpMap;				/* The actual bump map. */
	SSpotLight SpotLight;

        int delay;
        int duration;
        time_t start_time;

        async_load_state *img_loader;
} SBumps;


static void SetPalette(Display *, SBumps *, XWindowAttributes * );
static void InitBumpMap(Display *, SBumps *, XWindowAttributes * );
static void InitBumpMap_2(Display *, SBumps *);
static void SoftenBumpMap( SBumps * );




/* This function pointer will point to the appropriate PutPixel*() function below. */
static void (*MyPutPixel)( int8_t *, uint32_t );

static void PutPixel32( int8_t *pData, uint32_t pixel )
{
	*(uint32_t *)pData = pixel;
}

static void PutPixel24( int8_t *pData, uint32_t pixel )
{
	pData[ 2 ] = ( pixel & 0x00FF0000 ) >> 16;
	pData[ 1 ] = ( pixel & 0x0000FF00 ) >> 8;
	pData[ 0 ] = ( pixel & 0x000000FF );
}

static void PutPixel16( int8_t *pData, uint32_t pixel )
{
	*(uint16_t *)pData = (uint16_t)pixel;
}

static void PutPixel8( int8_t *pData, uint32_t pixel )
{
	*(uint8_t *)pData = (uint8_t)pixel;
}

/* Creates the light map, which is a circular image... going from black around the edges
 * to white in the center. */
static void CreateSpotLight( SSpotLight *pSpotLight, uint16_t iDiameter, uint16_t nColorCount )
{
	double nDist;
	int16_t iDistX, iDistY;
	uint8_t *pLOffset;
	
	pSpotLight->nFalloffDiameter = iDiameter;
	pSpotLight->nFalloffRadius = pSpotLight->nFalloffDiameter / 2;
	pSpotLight->nLightDiameter = iDiameter / 2;
	pSpotLight->nLightRadius = pSpotLight->nLightDiameter / 2;
#ifdef VERBOSE
	printf( "%s: Falloff Diameter: %d\n", progclass, pSpotLight->nFalloffDiameter );
	printf( "%s: Spot Light Diameter: %d\n", progclass, pSpotLight->nLightDiameter );
#endif

	pSpotLight->aLightMap = malloc( pSpotLight->nLightDiameter * pSpotLight->nLightDiameter * sizeof(uint8_t) );

	pLOffset = pSpotLight->aLightMap;
	for( iDistY=-pSpotLight->nLightRadius; iDistY<pSpotLight->nLightRadius; ++iDistY )
	{
		for( iDistX=-pSpotLight->nLightRadius; iDistX<pSpotLight->nLightRadius; ++iDistX )
		{
			nDist = sqrt( pow( iDistX+0.5F, 2 ) + pow( iDistY+0.5F, 2 ) );
			if( nDist / pSpotLight->nLightRadius <= 1.0f )
				*pLOffset = (uint8_t)(nColorCount - ( ( nDist / pSpotLight->nLightRadius ) * ( nColorCount - 1 ) ));
			else
				*pLOffset = 0;

			++pLOffset;
		}
	}
		
	/* Initialize movement variables.	*/
	pSpotLight->nAccelX = 0;
	pSpotLight->nAccelY = 0;
	pSpotLight->nVelocityX = ( RANDOM() % 2 ) ? pSpotLight->nVelocityMax : -pSpotLight->nVelocityMax;
	pSpotLight->nVelocityY = ( RANDOM() % 2 ) ? pSpotLight->nVelocityMax : -pSpotLight->nVelocityMax;
}


/* Calculates the position of the spot light on the screen. */
static void CalcLightPos( SBumps *pBumps )
{
	SSpotLight *pSpotLight = &pBumps->SpotLight;
	float nGravity;

	/* X */
	if( pSpotLight->nXPos < pSpotLight->nFalloffRadius )							nGravity = 1.0f;
	else if( pSpotLight->nXPos > pBumps->iWinWidth - pSpotLight->nFalloffRadius )	nGravity = -1.0f;
	else																			nGravity = ( ( RANDOM() % 201 ) / 100.0f ) - 1.0f;
		
	pSpotLight->nAccelX += nGravity * ( pSpotLight->nAccelMax / 5.0f );
	if( pSpotLight->nAccelX < -pSpotLight->nAccelMax )		pSpotLight->nAccelX = -pSpotLight->nAccelMax;
	else if( pSpotLight->nAccelX > pSpotLight->nAccelMax )	pSpotLight->nAccelX = pSpotLight->nAccelMax;

	pSpotLight->nVelocityX += pSpotLight->nAccelX;
	if( pSpotLight->nVelocityX < -pSpotLight->nVelocityMax )		pSpotLight->nVelocityX = -pSpotLight->nVelocityMax;
	else if( pSpotLight->nVelocityX > pSpotLight->nVelocityMax )	pSpotLight->nVelocityX = pSpotLight->nVelocityMax;

	pSpotLight->nXPos += pSpotLight->nVelocityX;

	/* Y */
	if( pSpotLight->nYPos < pSpotLight->nFalloffRadius )								nGravity = 1.0f;
	else if( pSpotLight->nYPos > pBumps->iWinHeight - pSpotLight->nFalloffRadius )	nGravity = -1.0f;
	else																			nGravity = ( ( RANDOM() % 201 ) / 100.0f ) - 1.0f;
		
	pSpotLight->nAccelY += nGravity * ( pSpotLight->nAccelMax / 5.0f );
	if( pSpotLight->nAccelY < -pSpotLight->nAccelMax )		pSpotLight->nAccelY = -pSpotLight->nAccelMax;
	else if( pSpotLight->nAccelY > pSpotLight->nAccelMax )	pSpotLight->nAccelY = pSpotLight->nAccelMax;

	pSpotLight->nVelocityY += pSpotLight->nAccelY;
	if( pSpotLight->nVelocityY < -pSpotLight->nVelocityMax )		pSpotLight->nVelocityY = -pSpotLight->nVelocityMax;
	else if( pSpotLight->nVelocityY > pSpotLight->nVelocityMax )	pSpotLight->nVelocityY = pSpotLight->nVelocityMax;

	pSpotLight->nYPos += pSpotLight->nVelocityY;
}


/* Main initialization function. */
static void CreateBumps( SBumps *pBumps, Display *dpy, Window NewWin )
{
	XWindowAttributes XWinAttribs;
	XGCValues GCValues;
	int32_t nGCFlags;
	uint16_t iDiameter;

	/* Make size and velocity a function of window size, so it appears the same at 100x60 as it does in 3200x1200. */
	XGetWindowAttributes( dpy, NewWin, &XWinAttribs );
	pBumps->iWinWidth = XWinAttribs.width;
	pBumps->iWinHeight = XWinAttribs.height;
	pBumps->SpotLight.nXPos = XWinAttribs.width / 2.0f;
	pBumps->SpotLight.nYPos = XWinAttribs.height / 2.0f;
	pBumps->SpotLight.nVelocityMax = ( ( XWinAttribs.width < XWinAttribs.height ) ? XWinAttribs.width : XWinAttribs.height ) / 140.0f;
	pBumps->SpotLight.nAccelMax = pBumps->SpotLight.nVelocityMax / 10.0f;
	pBumps->dpy = dpy;
	pBumps->Win = NewWin;
    pBumps->screen = XWinAttribs.screen;
	pBumps->pXImage = NULL;
	
	iDiameter = ( ( pBumps->iWinWidth < pBumps->iWinHeight ) ? pBumps->iWinWidth : pBumps->iWinHeight ) / 2;

    /* jwz: sometimes we get tearing if this lands on the wrong bounaary;
       constraining it to be a multiple of 8 seems to fix it. */
    iDiameter = ((iDiameter+7)/8)*8;

#ifdef HAVE_XSHM_EXTENSION
	pBumps->bUseShm = get_boolean_resource(dpy,  "useSHM", "Boolean" );

	if( pBumps->bUseShm )
	{
		pBumps->pXImage = create_xshm_image( pBumps->dpy, XWinAttribs.visual, XWinAttribs.depth,
											 ZPixmap, NULL, &pBumps->XShmInfo, iDiameter, iDiameter );
		if( !pBumps->pXImage )
		{
			fprintf( stderr, "%s: Unable to create XShmImage.\n", progname );
			pBumps->bUseShm = False;
		}
	}
#endif /* HAVE_XSHM_EXTENSION */
	if( !pBumps->pXImage )
	{
		pBumps->pXImage = XCreateImage( pBumps->dpy, XWinAttribs.visual, XWinAttribs.depth, 
									ZPixmap, 0, NULL, iDiameter, iDiameter, BitmapPad( pBumps->dpy ), 0 );
		pBumps->pXImage->data = malloc( pBumps->pXImage->bytes_per_line * pBumps->pXImage->height * sizeof(int8_t) );
	}

	/* For speed, access the XImage data directly using my own PutPixel routine. */
	switch( pBumps->pXImage->bits_per_pixel )
	{
		case 32:
			pBumps->bytesPerPixel = 4;
			MyPutPixel = PutPixel32;
			break;
		
		case 24:
			pBumps->bytesPerPixel = 3;
			MyPutPixel = PutPixel24;
			break;

		case 16:
			pBumps->bytesPerPixel = 2;
			MyPutPixel = PutPixel16;
			break;

		case 8:
			pBumps->bytesPerPixel = 1;
			MyPutPixel = PutPixel8;
			break;

		default:
			fprintf( stderr, "%s: Unknown XImage depth.", progname );
#ifdef HAVE_XSHM_EXTENSION
			if( pBumps->bUseShm )
				destroy_xshm_image( pBumps->dpy, pBumps->pXImage, &pBumps->XShmInfo );
			else
#endif /* HAVE_XSHM_EXTENSION */
				XDestroyImage( pBumps->pXImage );
			exit( 1 );
	}
	
	GCValues.function = GXcopy;
	GCValues.subwindow_mode = IncludeInferiors;
	nGCFlags = GCFunction;
	if( use_subwindow_mode_p( XWinAttribs.screen, pBumps->Win ) ) /* See grabscreen.c */
		nGCFlags |= GCSubwindowMode;
	pBumps->GraphicsContext = XCreateGC( pBumps->dpy, pBumps->Win, nGCFlags, &GCValues );
	
	SetPalette(dpy, pBumps, &XWinAttribs );
	CreateSpotLight( &pBumps->SpotLight, iDiameter, pBumps->nColorCount );
	InitBumpMap(dpy, pBumps, &XWinAttribs );
}


/* Creates a specialized phong shade palette. */
static void SetPalette(Display *dpy, SBumps *pBumps, XWindowAttributes *pXWinAttribs )
{
	XColor BaseColor;
	XColor Color;
	char *sColor;			/* Spotlight Color */
	int16_t iColor;
	
	sColor = get_string_resource(dpy,  "color", "Color" );

	BaseColor.red = RANDOM() % 0xFFFF; 
	BaseColor.green = RANDOM() % 0xFFFF;
	BaseColor.blue = RANDOM() % 0xFFFF;
	
	/* Make one color full intesity to avoid dark spotlights.	*/
	switch( RANDOM() % 3 )
	{
		case 0:	BaseColor.red	= 0xFFFF;	break;
		case 1: BaseColor.green	= 0xFFFF;	break;
		case 2: BaseColor.blue	= 0xFFFF;	break;
	}

	if( strcasecmp( sColor, "random" ) && !XParseColor( pBumps->dpy, pXWinAttribs->colormap, sColor, &BaseColor ) )
		fprintf( stderr, "%s: color %s not found in database. Choosing random...\n", progname, sColor );

#ifdef VERBOSE
	printf( "%s: Spotlight color is <%d,%d,%d> RGB.\n", progclass, BaseColor.red, BaseColor.green, BaseColor.blue );
#endif  /*  VERBOSE */

	pBumps->nColorCount = get_integer_resource(dpy,  "colorcount", "Integer" );
	if( pBumps->nColorCount < 2 )	pBumps->nColorCount = 2;
	if( pBumps->nColorCount > 128 )	pBumps->nColorCount = 128;

	pBumps->aColors = malloc( pBumps->nColorCount * sizeof(unsigned long) );

	/* Creates a phong shade:                 / BaseColor  \                               Index/ColorCount 
	 *							PhongShade = | ------------ | Index + ( 65535 - BaseColor )^ 
	 *										  \ ColorCount /												*/
	pBumps->nColorCount--;
	for( iColor=0; iColor<=pBumps->nColorCount; iColor++ )
	{
		Color.red   = (uint16_t)( ( ( BaseColor.red   / (double)pBumps->nColorCount ) * iColor ) + pow( 0xFFFF - BaseColor.red,   iColor/(double)pBumps->nColorCount ) );
		Color.green = (uint16_t)( ( ( BaseColor.green / (double)pBumps->nColorCount ) * iColor ) + pow( 0xFFFF - BaseColor.green, iColor/(double)pBumps->nColorCount ) );
		Color.blue  = (uint16_t)( ( ( BaseColor.blue  / (double)pBumps->nColorCount ) * iColor ) + pow( 0xFFFF - BaseColor.blue,  iColor/(double)pBumps->nColorCount ) );

		if( !XAllocColor( pBumps->dpy, pXWinAttribs->colormap, &Color ) )
		{
			XFreeColors( pBumps->dpy, pXWinAttribs->colormap, pBumps->aColors, iColor, 0 );
			free( pBumps->aColors );
			pBumps->aColors = malloc( pBumps->nColorCount * sizeof(unsigned long) );
			pBumps->nColorCount--;
			iColor = -1;
		}
		else
			pBumps->aColors[ iColor ] = Color.pixel;
	}
	pBumps->nColorCount++;

#ifdef VERBOSE
	printf( "%s: Allocated %d colors.\n", progclass, pBumps->nColorCount );
#endif  /*  VERBOSE */

	XSetWindowBackground( pBumps->dpy, pBumps->Win, pBumps->aColors[ 0 ] );
}


/* Grabs the current contents of the window to use an intensity-based bump map. */
static void InitBumpMap(Display *dpy, SBumps *pBumps, XWindowAttributes *pXWinAttribs )
{
	pBumps->xColors = (XColor*)malloc( pBumps->iWinWidth * sizeof(XColor) );

    if (pBumps->source) abort();
    pBumps->source = XCreatePixmap(pBumps->dpy, pBumps->Win,
                                   pXWinAttribs->width, pXWinAttribs->height,
                                   pXWinAttribs->depth);
  pBumps->img_loader = load_image_async_simple (0, pXWinAttribs->screen,
                                            pBumps->Win, pBumps->source, 0, 0);
}

static void InitBumpMap_2(Display *dpy, SBumps *pBumps)
{
	XImage *pScreenImage;
	XColor *pColor;
	uint8_t nSoften;
	uint16_t iWidth, iHeight;
	uint32_t nAverager;
	uint16_t	*pBump;
	uint16_t maxHeight;
	double softenMultiplier = 1.0f;
	BOOL bInvert = (BOOL)get_boolean_resource(dpy,  "invert", "Boolean" );
    XWindowAttributes XWinAttribs;
    XGetWindowAttributes( pBumps->dpy, pBumps->Win, &XWinAttribs );

    pBumps->start_time = time ((time_t *) 0);

	pScreenImage = XGetImage( pBumps->dpy, pBumps->source, 0, 0, 
                              pBumps->iWinWidth, pBumps->iWinHeight,
                              ~0L, ZPixmap );
/*    XFreePixmap (pBumps->dpy, pBumps->source);
    pBumps->source = 0;*/

	XSetWindowBackground( pBumps->dpy, pBumps->Win, pBumps->aColors[ 0 ] );
	XClearWindow (pBumps->dpy, pBumps->Win);
	XSync (pBumps->dpy, 0);

	pBumps->aBumpMap = malloc( pBumps->iWinWidth * pBumps->iWinHeight * sizeof(uint16_t) );
	
	nSoften = get_integer_resource(dpy,  "soften", "Integer" );
	while( nSoften-- )
		softenMultiplier *= 1.0f + ( 1.0f / 3.0f );	/* Softening takes the max height down, so scale up to compensate. */
	maxHeight = pBumps->SpotLight.nLightRadius * softenMultiplier;
	nAverager = maxHeight ? ( 3 * 0xFFFF ) / maxHeight : 0;

	pBump = pBumps->aBumpMap;
	if( bInvert )	/* Funny, it's actually the 'else' that inverts the bump map... */
	{
		for( iHeight=0; iHeight<pBumps->iWinHeight; iHeight++ )
		{
			pColor = pBumps->xColors;
			for( iWidth=0; iWidth<pBumps->iWinWidth; iWidth++ )
				(pColor++)->pixel = XGetPixel( pScreenImage, iWidth, iHeight );

			XQueryColors( pBumps->dpy, XWinAttribs.colormap, pBumps->xColors, pBumps->iWinWidth );

			pColor = pBumps->xColors;
			for( iWidth=pBumps->iWinWidth; iWidth; --iWidth, ++pColor, ++pBump )
			  *pBump = ( nAverager ? ( pColor->red + pColor->green + pColor->blue ) / nAverager : 0 );
		}
	}
	else
	{
		for( iHeight=0; iHeight<pBumps->iWinHeight; iHeight++ )
		{
			pColor = pBumps->xColors;
			for( iWidth=0; iWidth<pBumps->iWinWidth; iWidth++ )
				(pColor++)->pixel = XGetPixel( pScreenImage, iWidth, iHeight );

			XQueryColors( pBumps->dpy, XWinAttribs.colormap, pBumps->xColors, pBumps->iWinWidth );
	
			pColor = pBumps->xColors;
			for( iWidth=pBumps->iWinWidth; iWidth; --iWidth, ++pColor, ++pBump )
			  *pBump = ( maxHeight - ( nAverager ? ( pColor->red + pColor->green + pColor->blue ) / nAverager : 0 ) );
		}
	}

	XDestroyImage( pScreenImage );

	nSoften = get_integer_resource(dpy,  "soften", "Integer" );
#ifdef VERBOSE
	if( nSoften )	printf( "%s: Softening Bump Map %d time(s)...\n", progclass, nSoften );
#endif
	while( nSoften-- )
		SoftenBumpMap( pBumps );

/*	free( pBumps->xColors );
    pBumps->xColors = 0;*/
}

/* Soften the bump map.  This is to avoid pixelated-looking ridges.
 * |-----|-----|-----|
 * |  0% |12.5%|  0% |	The adjacent pixels are averaged together
 * |-----|-----|-----|	first.  Then than value is averaged with
 * |12.5%| 50% |12.5%|	the pixel is question. This essentially weights
 * |-----|-----|-----|  each pixel as shown on the left.
 * |  0% |12.5%|  0% |
 * |-----|-----|-----|
 */
static void SoftenBumpMap( SBumps *pBumps )
{
	uint16_t *pOffset, *pTOffset;
	uint32_t nHeight;
	uint32_t iWidth, iHeight;
	uint16_t *aTempBuffer = malloc( pBumps->iWinWidth * pBumps->iWinHeight * sizeof(uint16_t) );

	pOffset = pBumps->aBumpMap;
	pTOffset = aTempBuffer;
	for( iHeight=pBumps->iWinHeight; iHeight; --iHeight )
	{
		for( iWidth=pBumps->iWinWidth; iWidth; --iWidth, ++pOffset, ++pTOffset )
		{
			if( iHeight==pBumps->iWinHeight || iHeight==1 ||
				iWidth==pBumps->iWinWidth || iWidth==1 )
			{
				*pTOffset = 0;
				continue;
			}

			nHeight = pOffset[ -pBumps->iWinWidth ];
			nHeight += pOffset[ 1 ];
			nHeight += pOffset[ pBumps->iWinWidth ];
			nHeight += pOffset[ -1 ];
			nHeight >>= 2;
			nHeight += pOffset[ 0 ];
			nHeight >>= 1;
			*pTOffset = nHeight;
		}
	}						

	memcpy( pBumps->aBumpMap, aTempBuffer, pBumps->iWinWidth * pBumps->iWinHeight * sizeof(uint16_t) );
	free( aTempBuffer );
}


/* This is where we slap down some pixels... */
static void Execute( SBumps *pBumps )
{
	int32_t nLightXPos, nLightYPos;
	int32_t iScreenX, iScreenY;
	int32_t iLightX, iLightY;
	uint16_t *pBOffset;
	int8_t *pDOffset;
	int32_t nX, nY;
	uint16_t nColor;
	int32_t nLightOffsetFar = pBumps->SpotLight.nFalloffDiameter - pBumps->SpotLight.nLightRadius;

	CalcLightPos( pBumps );
	
	/* Offset to upper left hand corner. */
	nLightXPos = pBumps->SpotLight.nXPos - pBumps->SpotLight.nFalloffRadius;
	nLightYPos = pBumps->SpotLight.nYPos - pBumps->SpotLight.nFalloffRadius;
	
	for( iScreenY=nLightYPos, iLightY=-pBumps->SpotLight.nLightRadius; iLightY<nLightOffsetFar; ++iScreenY, ++iLightY )
	{
		if( iScreenY < 0 )							continue;
		else if( iScreenY >= pBumps->iWinHeight )	break;

    /* warning: pointer targets in assignment differ in signedness
       Should pDOffset be a int8?  I can't tell.  -jwz, 22-Jul-2003 */
		pDOffset = (int8_t *) &pBumps->pXImage->data[ (iLightY+pBumps->SpotLight.nLightRadius) * pBumps->pXImage->bytes_per_line ];
		pBOffset = pBumps->aBumpMap + ( iScreenY * pBumps->iWinWidth ) + nLightXPos;
		for( iScreenX=nLightXPos, iLightX=-pBumps->SpotLight.nLightRadius; iLightX<nLightOffsetFar; ++iScreenX, ++iLightX, ++pBOffset, pDOffset+=pBumps->bytesPerPixel )
		{
			if( iScreenX < 0 )							continue;
			else if( iScreenX >= pBumps->iWinWidth )	break;
			else if( iScreenY == 0 || iScreenY >= pBumps->iWinHeight-2 ||
					 iScreenX == 0 || iScreenX >= pBumps->iWinWidth-2 )
			{
				MyPutPixel( pDOffset, pBumps->aColors[ 0 ] );
				continue;
			}

			/* That's right folks, all the magic of bump mapping occurs in these two lines.  (kinda disappointing, isn't it?) */
			nX = ( pBOffset[ 1 ] - pBOffset[ 0 ] ) + iLightX;
			nY = ( pBOffset[ pBumps->iWinWidth ] - pBOffset[ 0 ] ) + iLightY;

			if( nX<0 || nX>=pBumps->SpotLight.nLightDiameter
			 || nY<0 || nY>=pBumps->SpotLight.nLightDiameter )
			{
				MyPutPixel( pDOffset, pBumps->aColors[ 0 ] );
				continue;
			}
				
			nColor = pBumps->SpotLight.aLightMap[ ( nY * pBumps->SpotLight.nLightDiameter ) + nX ];
			MyPutPixel( pDOffset, pBumps->aColors[ nColor ] );
		}
	}	

	/* Allow the spotlight to go *slightly* off the screen by clipping the XImage. */
	iLightX = iLightY = 0;	/* Use these for XImages X and Y now.	*/
	nX = nY = pBumps->SpotLight.nFalloffDiameter;	/* Use these for XImage width and height now.	*/
	if( nLightXPos < 0 )
	{
		iLightX = -nLightXPos;
		nX -= iLightX;
		nLightXPos = 0;
	}
	else if( nLightXPos + nX >= pBumps->iWinWidth )
	{
		nX -= ( nLightXPos + nX ) - pBumps->iWinWidth;
	}
	
	if( nLightYPos < 0 )
	{
		iLightY = -nLightYPos;
		nY -= iLightY;
		nLightYPos = 0;
	}
	else if( nLightYPos + nY >= pBumps->iWinHeight )
	{
		nY -= ( nLightYPos + nY ) - pBumps->iWinHeight;
	}
	
#ifdef HAVE_XSHM_EXTENSION
	if( pBumps->bUseShm )
		XShmPutImage( pBumps->dpy, pBumps->Win, pBumps->GraphicsContext, pBumps->pXImage, iLightX, iLightY, nLightXPos, nLightYPos,
					  nX, nY, False);
	else
#endif /* HAVE_XSHM_EXTENSION */
		XPutImage( pBumps->dpy, pBumps->Win, pBumps->GraphicsContext, pBumps->pXImage, iLightX, iLightY, nLightXPos, nLightYPos,
				   nX, nY );
}


static void DestroySpotLight( SSpotLight *pSpotLight ) { free( pSpotLight->aLightMap ); }

/* Clean up */
static void DestroyBumps( SBumps *pBumps )
{
	DestroySpotLight( &pBumps->SpotLight );
	free( pBumps->aColors );
	free( pBumps->aBumpMap );
#ifdef HAVE_XSHM_EXTENSION
	if( pBumps->bUseShm )
		destroy_xshm_image( pBumps->dpy, pBumps->pXImage, &pBumps->XShmInfo );
	else
#endif /* HAVE_XSHM_EXTENSION */
		XDestroyImage( pBumps->pXImage );
}


/* All messages to the screensaver are processed here. */
static void *
bumps_init (Display *dpy, Window Win)
{
	SBumps *Bumps = (SBumps *) calloc (1, sizeof(SBumps));

#ifdef VERBOSE
	time_t Time = time( NULL );
	uint16_t iFrame = 0;
#endif  /*  VERBOSE */
	
	CreateBumps( Bumps, dpy, Win );
	Bumps->delay = get_integer_resource(dpy,  "delay", "Integer" );
    Bumps->duration = get_integer_resource (dpy, "duration", "Seconds");
    if (Bumps->delay < 0) Bumps->delay = 0;
    if (Bumps->duration < 1) Bumps->duration = 1;
    Bumps->start_time = time ((time_t *) 0);
    return Bumps;
}

static unsigned long
bumps_draw (Display *dpy, Window window, void *closure)
{
  SBumps *Bumps = (SBumps *) closure;

  if (Bumps->img_loader)   /* still loading */
    {
      Bumps->img_loader = load_image_async_simple (Bumps->img_loader, 0, 0, 0, 0, 0);
      if (! Bumps->img_loader)  /* just finished */
        InitBumpMap_2(dpy, Bumps);
      return Bumps->delay;
    }

  if (!Bumps->img_loader &&
      Bumps->start_time + Bumps->duration < time ((time_t *) 0)) {
    Bumps->img_loader = load_image_async_simple (0, Bumps->screen,
                                                 Bumps->Win, Bumps->source, 
                                                 0, 0);
  }

  Execute( Bumps );

#ifdef VERBOSE
  iFrame++;
  if( Time - time( NULL ) )
    {
      printf( "FPS: %d\n", iFrame );
      Time = time( NULL );
      iFrame = 0;
    }
#endif  /*  VERBOSE */

  return Bumps->delay;
}

static void
bumps_reshape (Display *dpy, Window window, void *closure, 
                 unsigned int w, unsigned int h)
{
}

static Bool
bumps_event (Display *dpy, Window window, void *closure, XEvent *event)
{
  SBumps *Bumps = (SBumps *) closure;
  if (screenhack_event_helper (dpy, window, event))
    {
      Bumps->start_time = 0;
      return True;
    }

  return False;
}

static void
bumps_free (Display *dpy, Window window, void *closure)
{
  SBumps *Bumps = (SBumps *) closure;
  DestroyBumps( Bumps );
}


XSCREENSAVER_MODULE ("Bumps", bumps)

/* vim: ts=4
 */
