#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include "emacssignal.h"

#define XMOUSEBUFSIZE 64

#define BLOCK_INPUT_DECLARE() SIGMASKTYPE BLOCK_INPUT_mask
#ifdef SIGIO
#define BLOCK_INPUT() BLOCK_INPUT_mask = sigblock (sigmask (SIGIO))
#define UNBLOCK_INPUT() sigsetmask (BLOCK_INPUT_mask)
#else /* not SIGIO */
#define BLOCK_INPUT() stop_polling ()
#define UNBLOCK_INPUT() start_polling ()
#endif /* SIGIO */

#define CLASS  "Emacs"	/* class id for GNU Emacs, used in .Xdefaults, etc. */
