/* sonar.c --- Simulate a sonar screen.
 *
 * This is an implementation of a general purpose reporting tool in the
 * format of a Sonar display. It is designed such that a sensor is read
 * on every movement of a sweep arm and the results of that sensor are
 * displayed on the screen. The location of the display points (targets) on the
 * screen are determined by the current localtion of the sweep and a distance
 * value associated with the target. 
 *
 * Currently the only two sensors that are implemented are the simulator
 * (the default) and the ping sensor. The simulator randomly creates a set
 * of bogies that move around on the scope while the ping sensor can be
 * used to display hosts on your network.
 *
 * The ping code is only compiled in if you define HAVE_ICMP or HAVE_ICMPHDR,
 * because, unfortunately, different systems have different ways of creating
 * these sorts of packets.
 *
 * Also: creating an ICMP socket is a privileged operation, so the program
 * needs to be installed SUID root if you want to use the ping mode.  If you
 * check the code you will see that this privilige is given up immediately
 * after the socket is created.
 *
 * It should be easy to extend this code to support other sorts of sensors.
 * Some ideas:
 *   - search the output of "netstat" for the list of hosts to ping;
 *   - plot the contents of /proc/interrupts;
 *   - plot the process table, by process size, cpu usage, or total time;
 *   - plot the logged on users by idle time or cpu usage.
 *
 * Copyright (C) 1998 by Stephen Martin (smartin@canada.com).
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * $Revision: 1.12 $
 *
 * Version 1.0 April 27, 1998.
 * - Initial version
 * - Submitted to RedHat Screensaver Contest
 * 
 * Version 1.1 November 3, 1998.
 * - Added simulation mode.
 * - Added enhancements by Thomas Bahls <thommy@cs.tu-berlin.de>
 * - Fixed huge memory leak.
 * - Submitted to xscreensavers
 * 
 * Version 1.2
 * - All ping code is now ifdef-ed by the compile time symbol HAVE_PING;
 *   use -DHAVE_PING to include it when you compile.
 * - Sweep now uses gradients.
 * - Fixed portability problems with icmphdr on some systems.
 * - removed lowColor option/resource.
 * - changed copyright notice so that it could be included in the xscreensavers
 *   collection.
 *
 * Version 1.3 November 16, 1998.
 * - All ping code is now ifdef-ed by the compile time symbol PING use -DPING
 *   to include it when you compile.
 * - Sweep now uses gradients.
 * - Fixed portability problems with icmphdr on some systems.
 * - removed lowcolour option/resource.
 * - changed copyright notice so that it could be included in the xscreensavers
 *   collection.
 *
 * Version 1.4 November 18, 1998.
 * - More ping portability fixes.
 *
 * Version 1.5 November 19, 1998.
 * - Synced up with jwz's changes.
 * - Now need to define HAVE_PING to compile in the ping stuff.
 */

/* These are computed by configure now:
   #define HAVE_ICMP
   #define HAVE_ICMPHDR
 */


/* Include Files */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "screenhack.h"
#include "colors.h"
#include "hsv.h"

#if defined(HAVE_ICMP) || defined(HAVE_ICMPHDR)
# include <unistd.h>
# include <limits.h>
# include <signal.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/time.h>
# include <sys/ipc.h>
# include <sys/shm.h>
# include <sys/socket.h>
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>
# include <netinet/udp.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif /* HAVE_ICMP || HAVE_ICMPHDR */


/* Defines */

#undef MY_MIN
#define MY_MIN(a,b) ((a)<(b)?(a - 50):(b - 10))

#ifndef LINE_MAX
# define LINE_MAX 2048
#endif

/* Frigging icmp */

#if defined(HAVE_ICMP)
# define HAVE_PING
# define ICMP             icmp
# define ICMP_TYPE(p)     (p)->icmp_type
# define ICMP_CODE(p)     (p)->icmp_code
# define ICMP_CHECKSUM(p) (p)->icmp_cksum
# define ICMP_ID(p)       (p)->icmp_id
# define ICMP_SEQ(p)      (p)->icmp_seq
#elif defined(HAVE_ICMPHDR)
# define HAVE_PING
# define ICMP             icmphdr
# define ICMP_TYPE(p)     (p)->type
# define ICMP_CODE(p)     (p)->code
# define ICMP_CHECKSUM(p) (p)->checksum
# define ICMP_ID(p)       (p)->un.echo.id
# define ICMP_SEQ(p)      (p)->un.echo.sequence
#else
# undef HAVE_PING
#endif

/* Forward References */

#ifdef HAVE_PING
static u_short checksum(u_short *, int);
#endif
static long delta(struct timeval *, struct timeval *);

/* Data Structures */

/*
 * The Bogie.
 *
 * This represents an object that is visable on the scope.
 */

typedef struct Bogie {
    char *name;			/* The name of the thing being displayed */
    int distance;		/* The distance to this thing (0 - 100) */
    int tick;			/* The tick that it was found on */
    int ttl;			/* The time to live */
    int age;                    /* How long it's been around */
    struct Bogie *next;		/* The next one in the list */
} Bogie;

/*
 * Sonar Information.
 *
 * This contains all of the runtime information about the sonar scope.
 */

typedef struct {
    Display *dpy;		/* The X display */
    Window win;			/* The window */
    GC hi, 			/* The leading edge of the sweep */
	lo, 			/* The trailing part of the sweep */
	erase,			/* Used to erase things */
	grid,                   /* Used to draw the grid */
	text;			/* Used to draw text */
    Colormap cmap;		/* The colormap */
    XFontStruct *font;          /* The font to use for the labels */
    int text_steps;		/* How many steps to fade text. */
    XColor *text_colors;	/* Pixel values used to fade text */
    int sweep_degrees;		/* How much of the circle the sweep uses */
    int sweep_segs;		/* How many gradients in the sweep. */
    XColor *sweep_colors;        /* The sweep pixel values */
    int width, height;		/* Window dimensions */
    int minx, miny, maxx, maxy, /* Bounds of the scope */
	centrex, centrey, radius; /* Parts of the scope circle */
    Bogie *visable;		/* List of visable objects */
    int current;		/* Current position of sweep */

    int delay;			/* how long between each frame of the anim */

} sonar_info;

/* 
 * Variables to support the differnt Sonar modes.
 */

Bogie *(*sensor)(sonar_info *, void *);	/* The current sensor */
void *sensor_info;			/* Information about the sensor */

/*
 * A list of targets to ping.
 */

#ifdef HAVE_PING
typedef struct ping_target {
    char *name;			/* The name of the target */
    struct sockaddr address;	/* The address of the target */
    struct ping_target *next;	/* The next one in the list */
} ping_target;

/*
 * Ping Information.
 *
 * This contains the information for the ping sensor.
 */

typedef struct {
    int icmpsock;		/* Socket for sending pings */
    int pid;			/* Our process ID */
    int seq;			/* Packet sequence number */
    int timeout;		/* Timeout value for pings */
    ping_target *targets;	/* List of targets to ping */
    int numtargets;             /* The number of targets to ping */
} ping_info;

/* Flag to indicate that the timer has expired on us */

static int timer_expired;


#endif /* HAVE_PING */

/*
 * A list of targets for the simulator
 */

typedef struct sim_target {
    char *name;			/* The name of the target */
    int nexttick;		/* The next tick that this will be seen */
    int nextdist;		/* The distance on that tick */
    int movedlasttick;		/* Flag to indicate we just moved this one */
} sim_target;

/*
 * Simulator Information.
 *
 * This contains the information for the simulator mode.
 */

typedef struct {
    sim_target *teamA;		/* The bogies for the A team */
    int numA;			/* The number of bogies in team A */
    char *teamAID;		/* The identifier for bogies in team A */
    sim_target *teamB;		/* The bogies for the B team */
    int numB;			/* The number of bogies in team B */
    char *teamBID;		/* The identifier for bogies in team B */
} sim_info;

/* Name of the Screensaver hack */

char *progclass="sonar";

/* Application Defaults */

char *defaults [] = {
    ".background:      #000000",
    ".sweepColor:      #00FF00",
    "*delay:	       100000",
    "*scopeColor:      #003300",
    "*gridColor:       #00AA00",
    "*textColor:       #FFFF00",
    "*ttl:             90",
    "*mode:            default",
    "*font:            fixed",
    "*sweepDegrees:    30",

    "*textSteps:       80",	/* npixels */
    "*sweepSegments:   80",	/* npixels */

#ifdef HAVE_PING
    "*pingTimeout:     3000",
    "*pingSource:      file",
    "*pingFile:        /etc/hosts",
    "*pingList:        localhost",
#endif /* HAVE_PING */
    "*teamAName:       F18",
    "*teamBName:       MIG",
    "*teamACount:      4",
    "*teamBCount:      4",
    0
};

/* Options passed to this program */

XrmOptionDescRec options [] = {
    {"-background",   ".background",   XrmoptionSepArg, 0 },
    {"-sweep-color",  ".sweepColor",   XrmoptionSepArg, 0 },
    {"-scope-color",  ".scopeColor",   XrmoptionSepArg, 0 },
    {"-grid-color",   ".gridColor",    XrmoptionSepArg, 0 },
    {"-text-color",   ".textColor",    XrmoptionSepArg, 0 },
    {"-ttl",          ".ttl",          XrmoptionSepArg, 0 },
    {"-mode",         ".mode",         XrmoptionSepArg, 0 },
    {"-font",         ".font",         XrmoptionSepArg, 0 },
#ifdef HAVE_PING
    {"-ping-timeout", ".pingTimeout",  XrmoptionSepArg, 0 },
    {"-ping-source",  ".pingSource",   XrmoptionSepArg, 0 },
    {"-ping-file",    ".pingFile",     XrmoptionSepArg, 0 },
    {"-ping-list",    ".pingList",     XrmoptionSepArg, 0 },
#endif /* HAVE_PING */
    {"-team-a-name",   ".teamAName",   XrmoptionSepArg, 0 },
    {"-team-b-name",   ".teamBName",   XrmoptionSepArg, 0 },
    {"-team-a-count",  ".teamACount",  XrmoptionSepArg, 0 },
    {"-team-b-count",  ".teamBCount",  XrmoptionSepArg, 0 },
    { 0, 0, 0, 0 }
};

/*
 * The number of ticks that bogies are visable on the screen before they
 * fade away.
 */

static int TTL;

/*
 * Create a new Bogie and set some initial values.
 *
 * Args:
 *    name     - The name of the bogie.
 *    distance - The distance value.
 *    tick     - The tick value.
 *    ttl      - The time to live value.
 *
 * Returns:
 *    The newly allocated bogie or null if a memory problem occured.
 */

static Bogie *
newBogie(char *name, int distance, int tick, int ttl) 
{

    /* Local Variables */

    Bogie *new;

    /* Allocate a bogie and initialize it */

    if ((new = (Bogie *) calloc(1, sizeof(Bogie))) == NULL) {
	fprintf(stderr, "%s: Out of Memory\n", progname);
	return NULL;
    }
    new->name = name;
    new->distance = distance;
    new->tick = tick;
    new->ttl = ttl;
    new->age = 0;
    new->next = (Bogie *) 0;
    return new;
}

/*
 * Free a Bogie.
 *
 * Args:
 *    b - The bogie to free.
 */

static void
freeBogie(Bogie *b) 
{
    if (b->name != (char *) 0)
	free(b->name);
    free(b);
}

/*
 * Find a bogie by name in a list.
 *
 * This does a simple linear search of the list for a given name.
 *
 * Args:
 *    bl   - The Bogie list to search.
 *    name - The name to look for.
 *
 * Returns:
 *    The requested Bogie or null if it wasn't found.
 */

static Bogie *
findNode(Bogie *bl, char *name) 
{

    /* Local Variables */

    Bogie *p;

    /* Abort if the list is empty or no name is given */

    if ((name == NULL) || (bl == NULL))
	return NULL;

    /* Search the list for the desired name */

    p = bl;
    while (p != NULL) {
	if (strcmp(p->name, name) == 0)
	    return p;
	p = p->next;
    }

    /* Not found */

    return NULL;
}

#ifdef HAVE_PING

/*
 * Lookup the address for a ping target;
 *
 * Args:
 *    target - The ping_target fill in the address for.
 *
 * Returns:
 *    1 if the host was successfully resolved, 0 otherwise.
 */

static int
lookupHost(ping_target *target) 
{

    /* Local Variables */

    struct sockaddr_in *iaddr;

    /* Set up the target address we first assume that the name is the
       IP address as a string */

    iaddr = (struct sockaddr_in *) &(target->address);
    iaddr->sin_family = AF_INET;
    if ((iaddr->sin_addr.s_addr = inet_addr(target->name)) == -1) {

	/* Conversion of IP address failed, try to look the host up by name */

	struct hostent *hent = gethostbyname(target->name);
	if (hent == NULL) {
	    fprintf(stderr, "%s: could not resolve host %s\n",
                    progname, target->name);
	    return 0;
	}
	memcpy(&iaddr->sin_addr, hent->h_addr_list[0],
	       sizeof(iaddr->sin_addr));
    }

    /* Done */

    return 1;
}

/*
 * Create a target for a host.
 *
 * Args:
 *    name - The name of the host.
 *
 * Returns:
 *    A newly allocated target or null if the host could not be resolved.
 */

static ping_target *
newHost(char *name) 
{

    /* Local Variables */

    ping_target *target = NULL;

    /* Create the target */

    if ((target = calloc(1, sizeof(ping_target))) == NULL) {
	fprintf(stderr, "%s: Out of Memory\n", progname);
	goto target_init_error;
    }
    if ((target->name = strdup(name)) == NULL) {
	fprintf(stderr, "%s: Out of Memory\n", progname);
	goto target_init_error;
    }

    /* Lookup the host */

    if (! lookupHost(target))
	goto target_init_error;

    /* Done */

    return target;

    /* Handle errors here */

target_init_error:
    if (target != NULL)
	free(target);
    return NULL;
}

/*
 * Generate a list of ping targets from the entries in a file.
 *
 * Args:
 *    fname - The name of the file. This file is expected to be in the same
 *            format as /etc/hosts.
 *
 * Returns:
 *    A list of targets to ping or null if an error occured.
 */

static ping_target *
readPingHostsFile(char *fname) 
{

    /* Local Variables */

    FILE *fp;
    char buf[LINE_MAX];
    char *p;
    ping_target *list = NULL;
    char *addr, *name;
    ping_target *new;

    /* Make sure we in fact have a file to process */

    if ((fname == NULL) || (fname[0] == '\0')) {
	fprintf(stderr, "%s: invalid ping host file name\n", progname);
	return NULL;
    }

    /* Open the file */

    if ((fp = fopen(fname, "r")) == NULL) {
	char msg[1024];
	sprintf(msg, "%s: unable to open host file %s", progname, fname);
	perror(msg);
	return NULL;
    }

    /* Read the file line by line */

    while ((p = fgets(buf, LINE_MAX, fp)) != NULL) {

	/*
	 * Parse the line skipping those that start with '#'.
	 * The rest of the lines in the file should be in the same
	 * format as a /etc/hosts file. We are only concerned with
	 * the first two field, the IP address and the name
	 */

	while ((*p == ' ') || (*p == '\t'))
	    p++;
	if (*p == '#')
	    continue;

	/* Get the name and address */

	name = addr = NULL;
	if ((addr = strtok(buf, " \t\n")) != NULL)
	    name = strtok(NULL, " \t\n");
	else
	    continue;

        /* Check to see if the addr looks like an addr.  If not, assume
           the addr is a name and there is no addr.  This way, we can
           handle files whose lines have "xx.xx.xx.xx hostname" as their
           first two tokens, and also files that have a hostname as their
           first token (like .ssh/known_hosts and .rhosts.)
         */
        {
          int i; char c;
          if (4 != sscanf(addr, "%d.%d.%d.%d%c", &i, &i, &i, &i, &c))
            {
              name = addr;
              addr = NULL;
            }
        }
        printf ("\"%s\" \"%s\"\n", name, addr);

	/* Create a new target using first the name then the address */

	new = NULL;
	if (name != NULL)
	    new = newHost(name);
	if (new == NULL && addr != NULL)
	    new = newHost(addr);

	/* Add it to the list if we got one */

	if (new != NULL) {
	    new->next = list;
	    list = new;
	}
    }

    /* Close the file and return the list */

    fclose(fp);
    return list;
}

/*
 * Generate a list of ping targets from the entries in a string.
 *
 * Args:
 *    list - A list of comma separated host names.
 *
 * Returns:
 *    A list of targets to ping or null if an error occured.
 */

static ping_target *
readPingHostsList(char *list) 
{

    /* Local Variables */

    char *host;
    ping_target *hostlist = NULL;
    ping_target *new;

    /* Check that there is a list */

    if ((list == NULL) || (list[0] == '\0'))
	return NULL;

    /* Loop through the hosts and add them to the list to return */

    host = strtok(list, ",");
    while (host != NULL) {
	new = newHost(host);
	if (new != NULL) {
	    new->next = hostlist;
	    hostlist = new;
	}
	host = strtok(NULL, ",");
    }

    /* Done */

    return hostlist;
}

/*
 * Generate a list ping targets consisting of all of the entries on
 * the same subnet.
 *
 * Returns:
 *    A list of all of the hosts on this net.
 */

static ping_target *
subnetHostsList(void) 
{

    /* Local Variables */

    char hostname[BUFSIZ];
    char address[BUFSIZ];
    struct hostent *hent;
    char *p;
    int i;
    ping_target *new;
    ping_target *list = NULL;

    /* Get our hostname */

    if (gethostname(hostname, BUFSIZ)) {
	fprintf(stderr, "%s: unable to get local hostname\n", progname);
	return NULL;
    }

    /* Get our IP address and convert it to a string */

    if ((hent = gethostbyname(hostname)) == NULL) {
	fprintf(stderr, "%s: unable to lookup our IP address\n", progname);
	return NULL;
    }
    strcpy(address, inet_ntoa(*((struct in_addr *)hent->h_addr_list[0])));

    /* Get a pointer to the last "." in the string */

    if ((p = strrchr(address, '.')) == NULL) {
	fprintf(stderr, "%s: can't parse IP address %s\n", progname, address);
	return NULL;
    }
    p++;

    /* Construct targets for all addresses in this subnet */

    /* #### jwz: actually, this is wrong, since it assumes a
       netmask of 255.255.255.0.  But I'm not sure how to find
       the local netmask.
     */
    for (i = 254; i > 0; i--) {
	sprintf(p, "%d", i);
	new = newHost(address);
	if (new != NULL) {
	    new->next = list;
	    list = new;
	}
    }
  
    /* Done */

    return list;
}

/*
 * Initialize the ping sensor.
 *
 * Returns:
 *    A newly allocated ping_info structure or null if an error occured.
 */

static ping_info *
init_ping(void) 
{

    /* Local Variables */

    ping_info *pi = NULL;		/* The new ping_info struct */
    char *src;				/* The source of the ping hosts */
    ping_target *pt;			/* Used to count the targets */

    /* Create the ping info structure */

    if ((pi = (ping_info *) calloc(1, sizeof(ping_info))) == NULL) {
	fprintf(stderr, "%s: Out of memory\n", progname);
	goto ping_init_error;
    }

    /* Create the ICMP socket and turn off SUID */

    if ((pi->icmpsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
	char msg[1024];
	sprintf(msg, "%s: can't create ICMP socket", progname);
	perror(msg);
	fprintf(stderr,
         "%s: this program must be setuid to root for `ping mode' to work.\n",
                progname);
	goto ping_init_error;
    }
    setuid(getuid());
    pi->pid = getpid() & 0xFFFF;
    pi->seq = 0;
    pi->timeout = get_integer_resource("pingTimeout", "PingTimeout");

    /* Generate a list of targets */

    src = get_string_resource("pingSource", "PingSource");
    if (strcmp(src, "file") == 0) {

	/*
         * The list of ping targets is to come from a file in
	 * /etc/hosts format
	 */

	pi->targets = readPingHostsFile(get_string_resource("pingFile",
							    "PingFile"));

    } else if (strcmp(src, "list") == 0) {

	/* The list of hosts is to come from the pinghostlist resource */

	pi->targets = readPingHostsList(get_string_resource("pingList",
							    "PingList"));

    } else if (strcmp(src, "subnet") == 0) {

	pi->targets = subnetHostsList();

    } else {

	/* Unknown source */

	fprintf(stderr,
               "%s: pingSource must be `file', `list', or `subnet', not: %s\n",
                progname, src);
        exit (1);
    }

    /* Make sure there is something to ping */

    if (pi->targets == NULL) {
	fprintf(stderr, "%s: nothing to ping", progname);
	goto ping_init_error;
    }

    /* Count the targets */

    pt = pi->targets;
    pi->numtargets = 0;
    while (pt != NULL) {
	pi->numtargets++;
	pt = pt->next;
    }

    /* Done */

    return pi;

    /* Handle initialization errors here */

ping_init_error:
    if (pi != NULL)
	free(pi);
    return NULL;
}

/*
 * Ping a host.
 *
 * Args:
 *    pi   - The ping information strcuture.
 *    host - The name or IP address of the host to ping (in ascii).
 */

static void
sendping(ping_info *pi, ping_target *pt) 
{

    /* Local Variables */

    u_char *packet;
    struct ICMP *icmph;
    int result;

    /*
     * Note, we will send the character name of the host that we are
     * pinging in the packet so that we don't have to keep track of the
     * name or do an address lookup when it comes back.
     */

    int pcktsiz = sizeof(struct ICMP) + sizeof(struct timeval) +
	strlen(pt->name) + 1;

    /* Create the ICMP packet */

    if ((packet = (u_char *) malloc(pcktsiz)) == (void *) 0)
	return;  /* Out of memory */
    icmph = (struct ICMP *) packet;
    ICMP_TYPE(icmph) = ICMP_ECHO;
    ICMP_CODE(icmph) = 0;
    ICMP_CHECKSUM(icmph) = 0;
    ICMP_ID(icmph) = pi->pid;
    ICMP_SEQ(icmph) = pi->seq++;
    gettimeofday((struct timeval *) &packet[sizeof(struct ICMP)],
		 (struct timezone *) 0);
    strcpy((char *) &packet[sizeof(struct ICMP) + sizeof(struct timeval)],
	   pt->name);
    ICMP_CHECKSUM(icmph) = checksum((u_short *)packet, pcktsiz);

    /* Send it */

    if ((result = sendto(pi->icmpsock, packet, pcktsiz, 0, 
			 &pt->address, sizeof(pt->address))) !=  pcktsiz) {
#if 0
        char errbuf[BUFSIZ];
        sprintf(errbuf, "%s: error sending ping to %s", progname, pt->name);
	perror(errbuf);
#endif
    }
}

/*
 * Catch a signal and do nothing.
 *
 * Args:
 *    sig - The signal that was caught.
 */

static void
sigcatcher(int sig)
{
    timer_expired = 1;
}

/*
 * Compute the checksum on a ping packet.
 *
 * Args:
 *    packet - A pointer to the packet to compute the checksum for.
 *    size   - The size of the packet.
 *
 * Returns:
 *    The computed checksum
 *    
 */

static u_short
checksum(u_short *packet, int size) 
{

    /* Local Variables */

    register int nleft = size;
    register u_short *w = packet;
    register int sum = 0;
    u_short answer = 0;

    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */

    while (nleft > 1)  {
	sum += *w++;
	nleft -= 2;
    }

    /* mop up an odd byte, if necessary */

    if (nleft == 1) {
	*(u_char *)(&answer) = *(u_char *)w ;
        *(1 + (u_char *)(&answer)) = 0;
	sum += answer;
    }

    /* add back carry outs from top 16 bits to low 16 bits */

    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = ~sum;                          /* truncate to 16 bits */

    /* Done */

    return(answer);
}

/*
 * Look for ping replies.
 *
 * Retrieve all outstanding ping replies.
 *
 * Args:
 *    si - Information about the sonar.
 *    pi - Ping information.
 *    ttl - The time each bogie is to live on the screen
 *
 * Returns:
 *    A Bogie list of all the machines that replied.
 */

static Bogie *
getping(sonar_info *si, ping_info *pi, int ttl) 
{

    /* Local Variables */

    struct sockaddr from;
    int fromlen;
    int result;
    u_char packet[1024];
    struct timeval now;
    struct timeval *then;
    struct ip *ip;
    int iphdrlen;
    struct ICMP *icmph;
    Bogie *bl = NULL;
    Bogie *new;
    char *name;
    struct sigaction sa;
    struct itimerval it;

    /* Set up a signal to interupt our wait for a packet */

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigcatcher;
    if (sigaction(SIGALRM, &sa, 0) == -1) {
	char msg[1024];
	sprintf(msg, "%s: unable to trap SIGALRM", progname);
	perror(msg);
	exit(1);
    }

    /* Set up a timer to interupt us if we don't get a packet */

    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = pi->timeout;
    timer_expired = 0;
    setitimer(ITIMER_REAL, &it, NULL);

    /* Wait for a result packet */

    fromlen = sizeof(from);
    while (! timer_expired &&
	   (result = recvfrom(pi->icmpsock, packet, sizeof(packet),
			      0, &from, &fromlen)) > 0) {

	/* Check the packet */

	gettimeofday(&now, (struct timezone *) 0);
	ip = (struct ip *) packet;

	iphdrlen = ip->ip_hl << 2;
        /* On DEC OSF1 4.0, the preceeding line needs to be
           iphdrlen = (ip->ip_vhl & 0x0F) << 2;
           but I don't know how to do this portably.  -- jwz.
         */

	icmph = (struct ICMP *) &packet[iphdrlen];

	/* Was the packet a reply?? */

	if (ICMP_TYPE(icmph) != ICMP_ECHOREPLY) {
	    /* Ignore anything but ICMP Replies */
	    continue; /* Nope */
	}

	/* Was it for us? */

	if (ICMP_ID(icmph) != pi->pid) {
	    /* Ignore packets not set from us */
	    continue; /* Nope */
	}

	/* Copy the name of the bogie */

	if ((name =
	     strdup((char *) &packet[iphdrlen + 
				    + sizeof(struct ICMP)
				    + sizeof(struct timeval)])) == NULL) {
	    fprintf(stderr, "%s: Out of memory\n", progname);
	    return bl;
	}

        /* If the name is an IP addr, try to resolve it. */
        {
          int iip[4];
          char c;
          if (4 == sscanf(name, " %d.%d.%d.%d %c",
                          &iip[0], &iip[1], &iip[2], &iip[3], &c))
            {
              unsigned char ip[4];
              struct hostent *h;
              ip[0] = iip[0]; ip[1] = iip[1]; ip[2] = iip[2]; ip[3] = iip[3];
              h = gethostbyaddr ((char *) ip, 4, AF_INET);
              if (h && h->h_name && *h->h_name)
                {
                  free (name);
                  name = strdup (h->h_name);
                }
            }
        }

	/* Create the new Bogie and add it to the list we are building */

	if ((new = newBogie(name, 0, si->current, ttl)) == NULL)
	    return bl;
	new->next = bl;
	bl = new;

	/* Compute the round trip time */

	then =  (struct timeval *) &packet[iphdrlen +
					  sizeof(struct ICMP)];
	new->distance = delta(then, &now) / 100;
	if (new->distance == 0)
		new->distance = 2; /* HACK */
    }

    /* Done */

    return bl;
}

/*
 * Ping hosts.
 *
 * Args:
 *    si - Sonar Information.
 *    pi - Ping Information.
 *
 * Returns:
 *    A list of hosts that replied to pings or null if there were none.
 */

static Bogie *
ping(sonar_info *si, void *vpi) 
{

    ping_info *pi = (ping_info *) vpi;
    static ping_target *ptr = NULL;

    int tick = si->current * -1 + 1;
    if ((ptr == NULL) && (tick == 1))
	ptr = pi->targets;

    if (pi->numtargets <= 90) {
	int xdrant = 90 / pi->numtargets;
	if ((tick % xdrant) == 0) {
	    if (ptr != (ping_target *) 0) {
		sendping(pi, ptr);
		ptr = ptr->next;
	    }
	}

    } else if (pi->numtargets > 90) {
	if (ptr != (ping_target *) 0) {
	    sendping(pi, ptr);
	    ptr = ptr->next;
	}
    }

    /* Get the results */

    return getping(si, pi, TTL);
}

#endif /* HAVE_PING */

/*
 * Calculate the difference between two timevals in microseconds.
 *
 * Args:
 *    then - The older timeval.
 *    now  - The newer timeval.
 *
 * Returns:
 *   The difference between the two in microseconds.
 */

static long
delta(struct timeval *then, struct timeval *now) 
{
    return (((now->tv_sec - then->tv_sec) * 1000000) + 
	       (now->tv_usec - then->tv_usec));  
}

/*
 * Initialize the simulation mode.
 */

static sim_info *
init_sim(void) 
{

    /* Local Variables */

    sim_info *si;
    int i;

    /* Create the simulation info structure */

    if ((si = (sim_info *) calloc(1, sizeof(sim_info))) == NULL) {
	fprintf(stderr, "%s: Out of memory\n", progname);
	return NULL;
    }

    /* Team A */

    si->numA = get_integer_resource("teamACount", "TeamACount");
    if ((si->teamA = (sim_target *)calloc(si->numA, sizeof(sim_target)))
	== NULL) {
	free(si);
	fprintf(stderr, "%s: Out of Memory\n", progname);
	return NULL;
    }
    si->teamAID = get_string_resource("teamAName", "TeamAName");
    for (i = 0; i < si->numA; i++) {
	if ((si->teamA[i].name = (char *) malloc(strlen(si->teamAID) + 4))
	    == NULL) {
	    free(si);
	    fprintf(stderr, "%s: Out of Memory\n", progname);
	    return NULL;
	}
	sprintf(si->teamA[i].name, "%s%03d", si->teamAID, i+1);
	si->teamA[i].nexttick = (int) (90.0 * random() / RAND_MAX);
	si->teamA[i].nextdist = (int) (100.0 * random() / RAND_MAX);
    }

    /* Team B */

    si->numB = get_integer_resource("teamBCount", "TeamBCount");
    if ((si->teamB = (sim_target *)calloc(si->numB, sizeof(sim_target)))
	== NULL) {
	free(si);
	fprintf(stderr, "%s: Out of Memory\n", progname);
	return NULL;
    }
    si->teamBID = get_string_resource("teamBName", "TeamBName");
    for (i = 0; i < si->numB; i++) {
	if ((si->teamB[i].name = (char *) malloc(strlen(si->teamBID) + 4))
	    == NULL) {
	    free(si);
	    fprintf(stderr, "%s: Out of Memory\n", progname);
	    return NULL;
	}
	sprintf(si->teamB[i].name, "%s%03d", si->teamBID, i+1);
	si->teamB[i].nexttick = (int) (90.0 * random() / RAND_MAX);
	si->teamB[i].nextdist = (int) (100.0 * random() / RAND_MAX);
    }

    /* Done */

    return si;
}

/*
 * Initialize the Sonar.
 *
 * Args:
 *    dpy - The X display.
 *    win - The X window;
 *
 * Returns:
 *   A sonar_info strcuture or null if memory allocation problems occur.
 */

static sonar_info *
init_sonar(Display *dpy, Window win) 
{

    /* Local Variables */

    XGCValues gcv;
    XWindowAttributes xwa;
    sonar_info *si;
    XColor start, end;
    int h1, h2;
    double s1, s2, v1, v2;

    /* Create the Sonar information structure */

    if ((si = (sonar_info *) calloc(1, sizeof(sonar_info))) == NULL) {
	fprintf(stderr, "%s: Out of memory\n", progname);
	return NULL;
    }

    /* Initialize the structure for the current environment */

    si->dpy = dpy;
    si->win = win;
    si->visable = NULL;
    XGetWindowAttributes(dpy, win, &xwa);
    si->cmap = xwa.colormap;
    si->width = xwa.width;
    si->height = xwa.height;
    si->centrex = si->width / 2;
    si->centrey = si->height / 2;
    si->maxx = si->centrex + MY_MIN(si->centrex, si->centrey) - 10;
    si->minx = si->centrex - MY_MIN(si->centrex, si->centrey) + 10;
    si->maxy = si->centrey + MY_MIN(si->centrex, si->centrey) - 10;
    si->miny = si->centrey - MY_MIN(si->centrex, si->centrey) + 10;
    si->radius = si->maxx - si->centrex;
    si->current = 0;

    /* Get the font */

    if (((si->font = XLoadQueryFont(dpy, get_string_resource ("font", "Font")))
	 == NULL) &&
	((si->font = XLoadQueryFont(dpy, "fixed")) == NULL)) {
	fprintf(stderr, "%s: can't load an appropriate font\n", progname);
	return NULL;
    }

    /* Get the delay between animation frames */

    si->delay = get_integer_resource ("delay", "Integer");
    if (si->delay < 0) si->delay = 0;

    /* Create the Graphics Contexts that will be used to draw things */

    gcv.foreground = 
	get_pixel_resource ("sweepColor", "SweepColor", dpy, si->cmap);
    si->hi = XCreateGC(dpy, win, GCForeground, &gcv);
    gcv.font = si->font->fid;
    si->text = XCreateGC(dpy, win, GCForeground|GCFont, &gcv);
    gcv.foreground = get_pixel_resource("scopeColor", "ScopeColor",
					dpy, si->cmap);
    si->erase = XCreateGC (dpy, win, GCForeground, &gcv);
    gcv.foreground = get_pixel_resource("gridColor", "GridColor",
					dpy, si->cmap);
    si->grid = XCreateGC (dpy, win, GCForeground, &gcv);

    /* Compute pixel values for fading text on the display */

    XParseColor(dpy, si->cmap, 
		get_string_resource("textColor", "TextColor"), &start);
    XParseColor(dpy, si->cmap, 
		get_string_resource("scopeColor", "ScopeColor"), &end);

    rgb_to_hsv (start.red, start.green, start.blue, &h1, &s1, &v1);
    rgb_to_hsv (end.red, end.green, end.blue, &h2, &s2, &v2);

    si->text_steps = get_integer_resource("textSteps", "TextSteps");
    if (si->text_steps < 0 || si->text_steps > 255)
      si->text_steps = 10;

    si->text_colors = (XColor *) calloc(si->text_steps, sizeof(XColor));
    make_color_ramp (dpy, si->cmap,
                     h1, s1, v1,
                     h2, s2, v2,
                     si->text_colors, &si->text_steps,
                     False, True, False);

    /* Compute the pixel values for the fading sweep */

    XParseColor(dpy, si->cmap, 
                get_string_resource("sweepColor", "SweepColor"), &start);

    rgb_to_hsv (start.red, start.green, start.blue, &h1, &s1, &v1);

    si->sweep_degrees = get_integer_resource("sweepDegrees", "Degrees");
    if (si->sweep_degrees <= 0) si->sweep_degrees = 20;
    if (si->sweep_degrees > 350) si->sweep_degrees = 350;

    si->sweep_segs = get_integer_resource("sweepSegments", "SweepSegments");
    if (si->sweep_segs < 1 || si->sweep_segs > 255)
      si->sweep_segs = 255;

    si->sweep_colors = (XColor *) calloc(si->sweep_segs, sizeof(XColor));
    make_color_ramp (dpy, si->cmap,
                     h1, s1, v1,
                     h2, s2, v2,
                     si->sweep_colors, &si->sweep_segs,
                     False, True, False);

    /* Done */

    return si;
}

/*
 * Update the location of a simulated bogie.
 */

static void
updateLocation(sim_target *t) 
{

    int xdist, xtick;

    t->movedlasttick = 1;
    xtick = (int) (3.0 * random() / RAND_MAX) - 1;
    xdist = (int) (11.0 * random() / RAND_MAX) - 5;
    if (((t->nexttick + xtick) < 90) && ((t->nexttick + xtick) >= 0))
	t->nexttick += xtick;
    else
	t->nexttick -= xtick;
    if (((t->nextdist + xdist) < 100) && ((t->nextdist+xdist) >= 0))
	t->nextdist += xdist;
    else
	t->nextdist -= xdist;
}

/*
 * The simulator. This uses information in the sim_info to simulate a bunch
 * of bogies flying around on the screen.
 */

/*
 * TODO: It would be cool to have the two teams chase each other around and
 *       shoot it out.
 */

static Bogie *
simulator(sonar_info *si, void *vinfo) 
{

    /* Local Variables */

    int i;
    Bogie *list = NULL;
    Bogie *new;
    sim_target *t;
    sim_info *info = (sim_info *) vinfo;

    /* Check team A */

    for (i = 0; i < info->numA; i++) {
	t = &info->teamA[i];
	if (!t->movedlasttick && (t->nexttick == (si->current * -1))) {
	    new = newBogie(strdup(t->name), t->nextdist, si->current, TTL);
	    if (list != NULL)
		new->next = list;
	    list = new;
	    updateLocation(t);
	} else
	    t->movedlasttick = 0;
    }

    /* Team B */

    for (i = 0; i < info->numB; i++) {
	t = &info->teamB[i];
	if (!t->movedlasttick && (t->nexttick == (si->current * -1))) {
	    new = newBogie(strdup(t->name), t->nextdist, si->current, TTL);
	    if (list != NULL)
		new->next = list;
	    list = new;
	    t->movedlasttick = 1;
	    updateLocation(t);
	} else
	    t->movedlasttick = 0;
    }

    /* Done */

    return list;
}

/*
 * Compute the X coordinate of the label.
 *
 * Args:
 *    si - The sonar info block.
 *    label - The label that will be drawn.
 *    x - The x coordinate of the bogie.
 *
 * Returns:
 *    The x coordinate of the start of the label.
 */

static int
computeStringX(sonar_info *si, char *label, int x) 
{

    int width = XTextWidth(si->font, label, strlen(label));
    return x - (width / 2);
}

/*
 * Compute the Y coordinate of the label.
 *
 * Args:
 *    si - The sonar information.
 *    y - The y coordinate of the bogie.
 *
 * Returns:
 *    The y coordinate of the start of the label.
 */

/* TODO: Add smarts to keep label in sonar screen */

static int
computeStringY(sonar_info *si, int y) 
{

    int fheight = si->font->ascent + si->font->descent;
    return y + 5 + fheight;
}

/*
 * Draw a Bogie on the radar screen.
 *
 * Args:
 *    si       - Sonar Information.
 *    draw     - A flag to indicate if the bogie should be drawn or erased.
 *    name     - The name of the bogie.
 *    degrees  - The number of degrees that it should apprear at.
 *    distance - The distance the object is from the centre.
 *    ttl      - The time this bogie has to live.
 *    age      - The time this bogie has been around.
 */

static void
DrawBogie(sonar_info *si, int draw, char *name, int degrees, 
	  int distance, int ttl, int age) 
{

    /* Local Variables */

    int x, y;
    GC gc;
    int ox = si->centrex;
    int oy = si->centrey;
    int index, delta;

    /* Compute the coordinates of the object */

    distance = (log((double) distance) / 10.0) * si->radius;
    x = ox + ((double) distance * cos(4.0 * ((double) degrees)/57.29578));
    y = oy - ((double) distance * sin(4.0 * ((double) degrees)/57.29578));

    /* Set up the graphics context */

    if (draw) {

	/* Here we attempt to compute the distance into the total life of
	 * object that we currently are. This distance is used against
	 * the total lifetime to compute a fraction which is the index of
	 * the color to draw the bogie.
	 */

	if (si->current <= degrees)
	    delta = (si->current - degrees) * -1;
	else
	    delta = 90 + (degrees - si->current);
	delta += (age * 90);
	index = (si->text_steps - 1) * ((float) delta / (90.0 * (float) ttl));
	gc = si->text;
	XSetForeground(si->dpy, gc, si->text_colors[index].pixel);

    } else
	gc = si->erase;

  /* Draw (or erase) the Bogie */

    XFillArc(si->dpy, si->win, gc, x, y, 5, 5, 0, 360 * 64);
    XDrawString(si->dpy, si->win, gc,
		computeStringX(si, name, x),
		computeStringY(si, y), name, strlen(name));
}


/*
 * Draw the sonar grid.
 *
 * Args:
 *    si - Sonar information block.
 */

static void
drawGrid(sonar_info *si) 
{

    /* Local Variables */

    int i;
    int width = si->maxx - si->minx;
    int height = si->maxy - si->miny;
  
    /* Draw the circles */

    XDrawArc(si->dpy, si->win, si->grid, si->minx - 10, si->miny - 10, 
	     width + 20, height + 20,  0, (360 * 64));

    XDrawArc(si->dpy, si->win, si->grid, si->minx, si->miny, 
	     width, height,  0, (360 * 64));

    XDrawArc(si->dpy, si->win, si->grid, 
	     (int) (si->minx + (.166 * width)), 
	     (int) (si->miny + (.166 * height)), 
	     (unsigned int) (.666 * width), (unsigned int)(.666 * height),
	     0, (360 * 64));

    XDrawArc(si->dpy, si->win, si->grid, 
	     (int) (si->minx + (.333 * width)),
	     (int) (si->miny + (.333 * height)), 
	     (unsigned int) (.333 * width), (unsigned int) (.333 * height),
	     0, (360 * 64));

    /* Draw the radial lines */

    for (i = 0; i < 360; i += 10)
	if (i % 30 == 0)
	    XDrawLine(si->dpy, si->win, si->grid, si->centrex, si->centrey,
		      (int) (si->centrex +
		      (si->radius + 10) * (cos((double) i / 57.29578))),
		      (int) (si->centrey -
		      (si->radius + 10)*(sin((double) i / 57.29578))));
	else
	    XDrawLine(si->dpy, si->win, si->grid, 
		      (int) (si->centrex + si->radius *
			     (cos((double) i / 57.29578))),
		      (int) (si->centrey - si->radius *
			     (sin((double) i / 57.29578))),
		      (int) (si->centrex +
		      (si->radius + 10) * (cos((double) i / 57.29578))),
		      (int) (si->centrey - 
		      (si->radius + 10) * (sin((double) i / 57.29578))));
}

/*
 * Update the Sonar scope.
 *
 * Args:
 *    si - The Sonar information.
 *    bl - A list  of bogies to add to the scope.
 */

static void
Sonar(sonar_info *si, Bogie *bl) 
{

    /* Local Variables */

    Bogie *bp, *prev;
    int i;

    /* Check for expired tagets and remove them from the visable list */

    prev = NULL;
    for (bp = si->visable; bp != NULL; bp = bp->next) {

	/*
	 * Remove it from the visable list if it's expired or we have
	 * a new target with the same name.
	 */

	bp->age ++;

	if (((bp->tick == si->current) && (++bp->age >= bp->ttl)) ||
	    (findNode(bl, bp->name) != NULL)) {
	    DrawBogie(si, 0, bp->name, bp->tick,
		      bp->distance, bp->ttl, bp->age);
	    if (prev == NULL)
		si->visable = bp->next;
	    else
		prev->next = bp->next;
	    freeBogie(bp);
	} else
	    prev = bp;
    }

    /* Draw the sweep */

    {
      int seg_deg = (si->sweep_degrees * 64) / si->sweep_segs;
      int start_deg = si->current * 4 * 64;
      if (seg_deg <= 0) seg_deg = 1;
      for (i = 0; i < si->sweep_segs; i++) {
	XSetForeground(si->dpy, si->hi, si->sweep_colors[i].pixel);
	XFillArc(si->dpy, si->win, si->hi, si->minx, si->miny, 
		 si->maxx - si->minx, si->maxy - si->miny,
                 start_deg + (i * seg_deg),
                 seg_deg);
      }

      /* Remove the trailing wedge the sonar */
      XFillArc(si->dpy, si->win, si->erase, si->minx, si->miny, 
               si->maxx - si->minx, si->maxy - si->miny, 
               start_deg + (i * seg_deg),
               (4 * 64));
    }

    /* Move the new targets to the visable list */

    for (bp = bl; bp != (Bogie *) 0; bp = bl) {
	bl = bl->next;
	bp->next = si->visable;
	si->visable = bp;
    }

    /* Draw the visable targets */

    for (bp = si->visable; bp != NULL; bp = bp->next) {
	if (bp->age < bp->ttl)		/* grins */
	   DrawBogie(si, 1, bp->name, bp->tick, bp->distance, bp->ttl,bp->age);
    }

    /* Redraw the grid */

    drawGrid(si);
}

/*
 * Main screen saver hack.
 *
 * Args:
 *    dpy - The X display.
 *    win - The X window.
 */

void 
screenhack(Display *dpy, Window win) 
{

    /* Local Variables */

    sonar_info *si;
    struct timeval start, finish;
    Bogie *bl;
    long sleeptime;
    char *mode;

    /* 
     * Initialize 
     * Adding new sensors would involve supporting more modes other than
     * ping and initiailizing the sensor in the same way.
     */

    mode = get_string_resource("mode", "Mode");

    if (!mode || !*mode || !strcmp(mode, "default")) /* Pick a good default. */
      {
#ifdef HAVE_PING
        if (geteuid() == 0)	/* we're root or setuid -- ping will work. */
          mode = "ping";
        else
#endif
          mode = "simulation";
      }

#ifdef HAVE_PING
    if (strcmp(mode, "ping") == 0) {
	sensor = ping;
	if ((sensor_info = (void *) init_ping()) == (void *) 0)
          {
            fprintf (stderr, "%s: running in `simulation mode' instead.\n",
                     progname);
	    goto SIM;
          }
    } else
#endif /* HAVE_PING */
    if (strcmp(mode, "simulation") == 0) {
#ifdef HAVE_PING
    SIM:
#endif
	sensor = simulator;
	if ((sensor_info = (void *) init_sim()) == NULL)
	    exit(1);
    } else {
	fprintf(stderr, "%s: unsupported Sonar mode: %s\n", progname, mode);
	fprintf(stderr,
                "\tCurrently supported modes are `ping' and `simulation'\n");
	exit(1);
    }
    if ((si = init_sonar(dpy, win)) == (sonar_info *) 0)
	exit(1);
  
    /* Sonar loop */

    TTL = get_integer_resource("ttl", "TTL");

    while (1) {

	/* Call the sensor and display the results */

	gettimeofday(&start, (struct timezone *) 0);
	bl = sensor(si, sensor_info);
	Sonar(si, bl);

        /* Set up and sleep for the next one */

	si->current = (si->current - 1) % 90;
	XSync (dpy, False);
	gettimeofday(&finish, (struct timezone *) 0);
	sleeptime = si->delay - delta(&start, &finish);
        screenhack_handle_events (dpy);
	if (sleeptime > 0L)
	    usleep(sleeptime);
    }
}
