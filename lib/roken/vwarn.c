#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "err.h"

RCSID("$Id$");

void
vwarn(const char *fmt, va_list ap)
{
  int sverrno;
  
  sverrno = errno;
#if 0
  fprintf(stderr, "%s: ", __progname);
#endif
  if (fmt != NULL) {
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, ": ");
  }
  fprintf(stderr, "%s\n", strerror(sverrno));
}
