/* kpasswd.c --- verify kerberos passwords.
 * written by Nat Lanza (magus@cs.cmu.edu) for
 * xscreensaver, Copyright (c) 1993-1997, 1998, 2000
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef NO_LOCKING  /* whole file */

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_KERBEROS5
# include <kerberosIV/krb.h>
# include <kerberosIV/des.h>
#else /* !HAVE_KERBEROS5 (meaning Kerberos 4) */
# include <krb.h>
# include <des.h>
#endif /* !HAVE_KERBEROS5 */

#if !defined(VMS) && !defined(HAVE_ADJUNCT_PASSWD)
# include <pwd.h>
#endif


#ifdef __bsdi__
# include <sys/param.h>
# if _BSDI_VERSION >= 199608
#  define BSD_AUTH
# endif
#endif /* __bsdi__ */

/* blargh */
#undef  Bool
#undef  True
#undef  False
#define Bool  int
#define True  1
#define False 0

/* The user information we need to store */
static char realm[REALM_SZ];
static char  name[ANAME_SZ];
static char  inst[INST_SZ];
static char *tk_file;


/* Called at startup to grab user, instance, and realm information
   from the user's ticketfile (remember, name.inst@realm). Since we're
   using tf_get_pname(), this should work even if your kerberos username
   isn't the same as your local username. We grab the ticket at startup
   time so that even if your ticketfile dies while the screen's locked
   we'll still have the information to unlock it.

   Problems: the password dialog currently displays local username, so if
     you have some non-standard name/instance when you run xscreensaver,
     you'll need to remember what it was when unlocking, or else you lose.

     Also, we use des_string_to_key(), so if you have an AFS password
     (encrypted with ka_StringToKey()), you'll lose. Get a kerberos password;
     it isn't that hard.

   Like the original lock_init, we return false if something went wrong.
   We don't use the arguments we're given, though.
 */
Bool
kerberos_lock_init (int argc, char **argv, Bool verbose_p)
{
    int k_errno;
    
    memset(name, 0, sizeof(name));
    memset(inst, 0, sizeof(inst));
    
    /* find out where the user's keeping his tickets.
       squirrel it away for later use. */
    tk_file = tkt_string();

    /* open ticket file or die trying. */
    if ((k_errno = tf_init(tk_file, R_TKT_FIL))) {
	return False;
    }

    /* same with principal and instance names */
    if ((k_errno = tf_get_pname(name)) ||
	(k_errno = tf_get_pinst(inst))) {
	return False;
    }

    /* close the ticketfile to release the lock on it. */
    tf_close();

    /* figure out what realm we're authenticated to. this ought
       to be the local realm, but it pays to be sure. */
    if ((k_errno = krb_get_tf_realm(tk_file, realm))) {
	return False;
    }

    /* last-minute sanity check on what we got. */
    if ((strlen(name)+strlen(inst)+strlen(realm)+3) >
	(REALM_SZ + ANAME_SZ + INST_SZ + 3)) {
	return False;
    }

    /* success */
    return True;
}


/* des_string_to_key() wants this. If C didn't suck, we could have an
   anonymous function do this. Even a local one. But it does, so here
   we are. Calling it ive_got_your_local_function_right_here_buddy()
   would have been rude.
 */
static int 
key_to_key(char *user, char *instance, char *realm, char *passwd, C_Block key)
{
  memcpy(key, passwd, sizeof(des_cblock));
  return (0);
}

/* Called to see if the user's typed password is valid. We do this by asking
   the kerberos server for a ticket and checking to see if it gave us one.
   We need to move the ticketfile first, or otherwise we end up updating the
   user's tkfile with new tickets. This would break services like zephyr that
   like to stay authenticated, and it would screw with AFS authentication at
   some sites. So, we do a quick, painful hack with a tmpfile.
 */
Bool
kerberos_passwd_valid_p (const char *typed_passwd, Bool verbose_p)
{
    C_Block mitkey;
    Bool success;
    char *newtkfile;

    /* temporarily switch to a new ticketfile.
       I'm not using tmpnam() because it isn't entirely portable.
       this could probably be fixed with autoconf. */
    newtkfile = malloc(80 * sizeof(char));
    memset(newtkfile, 0, sizeof(newtkfile));

    sprintf(newtkfile, "/tmp/xscrn-%i", getpid());

    krb_set_tkt_string(newtkfile);

    /* encrypt the typed password. if you have an AFS password instead
       of a kerberos one, you lose *right here*. If you want to use AFS
       passwords, you can use ka_StringToKey() instead. As always, ymmv. */
    des_string_to_key(typed_passwd, mitkey);

    if (krb_get_in_tkt(name, inst, realm, "krbtgt", realm, DEFAULT_TKT_LIFE,
		       key_to_key, NULL, mitkey) != 0) {
	success = False;
    } else {
	success = True;
    }

    /* quickly block out the tempfile and password to prevent snooping,
       then restore the old ticketfile and cleean up a bit. */
    
    dest_tkt();
    krb_set_tkt_string(tk_file);
    free(newtkfile);
    memset(mitkey, 0, sizeof(mitkey));
    

    /* Did we verify successfully? */
    return success;
}

#endif /* NO_LOCKING -- whole file */
