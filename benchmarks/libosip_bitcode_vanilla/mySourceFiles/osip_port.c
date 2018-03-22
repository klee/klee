/*
  The oSIP library implements the Session Initiation Protocol (SIP -rfc3261-)
  Copyright (C) 2001-2012 Aymeric MOIZARD amoizard@antisip.com
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef _WIN32_WCE
#define _INC_TIME               /* for wce.h */
#include <windows.h>
#endif

#include <osipparser2/internal.h>

void klee_make_symbolic(void *addr,size_t count, const char *message){}

#include <osipparser2/osip_port.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#if defined(HAVE_STDARG_H)
#include <stdarg.h>
#define VA_START(a, f)  va_start(a, f)
#else
#if defined(HAVE_VARARGS_H)
#include <varargs.h>
#define VA_START(a, f) va_start(a)
#else
#include <stdarg.h>
#define VA_START(a, f)  va_start(a, f)
#endif
#endif

#if defined(__PALMOS__) && (__PALMOS__ < 0x06000000)
#	include <TimeMgr.h>
#	include <SysUtils.h>
#	include <SystemMgr.h>
#	include <StringMgr.h>
#else
#	include <time.h>
#endif

#if defined(__VXWORKS_OS__)
#include <selectLib.h>

/* needed for snprintf replacement */
#include <vxWorks.h>
#include <fioLib.h>
#include <string.h>

#elif defined(__PALMOS__)
#	if __PALMOS__ >= 0x06000000
#		include <sys/time.h>
#		include <SysThread.h>
#	endif
#elif (!defined(WIN32) && !defined(_WIN32_WCE))
#include <sys/time.h>
#elif defined(WIN32)
#include <windows.h>
#if (_MSC_VER >= 1700) && !defined(_USING_V110_SDK71_)
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#include <winsock2.h>
#elif defined(WINAPI_FAMILY) && WINAPI_FAMILY_ONE_PARTITION( WINAPI_FAMILY, WINAPI_PARTITION_APP )
#else
#ifdef WIN32_USE_CRYPTO
#include <Wincrypt.h>
#endif
#endif
#else
#ifdef WIN32_USE_CRYPTO
#include <Wincrypt.h>
#endif
#endif

#endif

#if defined (__rtems__)
#include <rtems.h>
#endif

#if defined (HAVE_SYS_UNISTD_H)
#include <sys/unistd.h>
#endif

#if defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined (HAVE_SYSLOG_H) && !defined(__arc__)
#include <syslog.h>
#endif

#if defined (HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#ifdef HAVE_PTH_PTHREAD_H
#include <pthread.h>
#endif

#if defined(__arc__)
#define HAVE_LRAND48
#endif

FILE *logfile = NULL;
int tracing_table[END_TRACE_LEVEL];
static int use_syslog = 0;
static osip_trace_func_t *trace_func = 0;

static unsigned int random_seed_set = 0;

#ifndef MINISIZE
#if !defined(WIN32) && !defined(_WIN32_WCE)
osip_malloc_func_t *osip_malloc_func = 0;
osip_realloc_func_t *osip_realloc_func = 0;
osip_free_func_t *osip_free_func = 0;
#endif
#endif

const char *osip_error_table[] = {
  "success",
  "undefined error",
  "bad parameter",
  "wrong state",
  "allocation failure",
  "syntax error",
  "not found",
  "api not initialized",
  "undefined",
  "undefined",
  "no network",                 /* -10 */
  "busy port",
  "unknown host",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "disk full",                  /* -30 */
  "no rights",
  "file not found",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",                  /* -40 */
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "time out",                   /* -50 */
  "too much call",
  "wrong format",
  "no common codec",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",
  "undefined",                  /* -60 */
};

const char *
osip_strerror (int err)
{
  if (err > 0)
    return osip_error_table[0];
  if (err > -60)
    return osip_error_table[-err];
  return osip_error_table[59];
}

#ifndef WIN32_USE_CRYPTO
unsigned int
osip_build_random_number ()
#else
static unsigned int
osip_fallback_random_number ()
#endif
{
  return rand ();
}

#ifdef WIN32_USE_CRYPTO

unsigned int
osip_build_random_number ()
{
  HCRYPTPROV crypto;
  BOOL err;
  unsigned int num;

  if (!random_seed_set) {
    unsigned int ticks;
    LARGE_INTEGER lCount;

    QueryPerformanceCounter (&lCount);
    ticks = lCount.LowPart + lCount.HighPart;
    srand (ticks);
    random_seed_set = 1;
  }
  err = CryptAcquireContext (&crypto, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  if (err) {
    err = CryptGenRandom (crypto, sizeof (num), (BYTE *) & num);
    CryptReleaseContext (crypto, 0);
  }
  if (!err) {
    num = osip_fallback_random_number ();
  }
  return num;
}

#endif

#if defined(__linux)
#include <limits.h>
#endif


char *
osip_strncpy (char *dest, const char *src, size_t length)
{
  strncpy (dest, src, length);
  dest[length] = '\0';
  return dest;
}

#undef osip_strdup

char *
osip_strdup (const char *ch)
{
  char *copy;
  size_t length;

  if (ch == NULL)
    return NULL;
  length = strlen (ch);
  copy = (char *) osip_malloc (length + 1);
  if (copy == NULL)
    return NULL;
  osip_strncpy (copy, ch, length);
  return copy;
}

#ifndef MINISIZE
int
osip_atoi (const char *number)
{
#if defined(__linux) || defined(HAVE_STRTOL)
  int i;

  if (number == NULL)
    return OSIP_UNDEFINED_ERROR;
  i = strtol (number, (char **) NULL, 10);
  if (i == LONG_MIN || i == LONG_MAX)
    return OSIP_UNDEFINED_ERROR;
  return i;
#endif

  return atoi (number);
}

#endif

#if (_MSC_VER >= 1700) && !defined(_USING_V110_SDK71_)
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)
#define HAVE_WINDOWSPHONE_API
#elif defined(WINAPI_FAMILY) && WINAPI_FAMILY_ONE_PARTITION( WINAPI_FAMILY, WINAPI_PARTITION_APP )
#define HAVE_WINAPPSTORE_API
#endif
#endif

void
osip_usleep (int useconds)
{
#if defined(__PALMOS__) && (__PALMOS__ >= 0x06000000)
  /* This bit will work for the Protein API, but not the Palm 68K API */
  nsecs_t nanoseconds = useconds * 1000;

  SysThreadDelay (nanoseconds, P_ABSOLUTE_TIMEOUT);
#elif defined(__PALMOS__) && (__PALMOS__ < 0x06000000)
  UInt32 stoptime = TimGetTicks () + (useconds / 1000000) * SysTicksPerSecond ();

  while (stoptime > TimGetTicks ())
    /* I wish there was some type of yield function here */
    ;
#elif defined(HAVE_WINDOWSPHONE_API)
  struct timeval delay;
  int sec;

  sec = (int) useconds / 1000000;
  if (sec > 0) {
    delay.tv_sec = sec;
    delay.tv_usec = 0;
  }
  else {
    delay.tv_sec = 0;
    delay.tv_usec = useconds;
  }
  select (0, 0, 0, 0, &delay);
#elif defined(HAVE_WINAPPSTORE_API)
  TODO
#elif defined(WIN32)
  Sleep (useconds / 1000);
#elif defined(__rtems__)
  rtems_task_wake_after (RTEMS_MICROSECONDS_TO_TICKS (useconds));
#elif defined(__arc__)
  struct timespec req;
  struct timespec rem;

  req.tv_sec = (int) useconds / 1000000;
  req.tv_nsec = (int) (useconds % 1000000) * 1000;
  nanosleep (&req, &rem);
#else
  struct timeval delay;
  int sec;

  sec = (int) useconds / 1000000;
  if (sec > 0) {
    delay.tv_sec = sec;
    delay.tv_usec = 0;
  }
  else {
    delay.tv_sec = 0;
    delay.tv_usec = useconds;
  }
  select (0, 0, 0, 0, &delay);
#endif
}

char *
osip_strdup_without_quote (const char *ch)
{
  char *copy = (char *) osip_malloc (strlen (ch) + 1);

  if (copy == NULL)
    return NULL;

  /* remove leading and trailing " */
  if ((*ch == '\"')) {
    osip_strncpy (copy, ch + 1, strlen (ch + 1));
    osip_strncpy (copy + strlen (copy) - 1, "\0", 1);
  }
  else
    osip_strncpy (copy, ch, strlen (ch));
  return copy;
}

int
osip_tolower (char *word)
{
#if defined(HAVE_CTYPE_H) && !defined (_WIN32_WCE)

  for (; *word; word++)
    *word = (char) tolower (*word);
#else
  size_t i;
  size_t len = strlen (word);

  for (i = 0; i <= len - 1; i++) {
    if ('A' <= word[i] && word[i] <= 'Z')
      word[i] = word[i] + 32;
  }
#endif
  return OSIP_SUCCESS;
}

#ifndef MINISIZE
int
osip_strcasecmp (const char *s1, const char *s2)
{
#if defined(__VXWORKS_OS__) || defined( __PSOS__)
  while ((*s1 != '\0') && (tolower (*s1) == tolower (*s2))) {
    s1++;
    s2++;
  }
  return (tolower (*s1) - tolower (*s2));
#elif defined(__PALMOS__) && (__PALMOS__ < 0x06000000)
  return StrCaselessCompare (s1, s2);
#elif defined(__PALMOS__) || (!defined WIN32 && !defined _WIN32_WCE)
  return strcasecmp (s1, s2);
#else
  return _stricmp (s1, s2);
#endif
}

int
osip_strncasecmp (const char *s1, const char *s2, size_t len)
{
#if defined(__VXWORKS_OS__) || defined( __PSOS__)
  if (len == 0)
    return OSIP_SUCCESS;
  while ((len > 0) && (tolower (*s1) == tolower (*s2))) {
    len--;
    if ((len == 0) || (*s1 == '\0') || (*s2 == '\0'))
      break;
    s1++;
    s2++;
  }
  return tolower (*s1) - tolower (*s2);
#elif defined(__PALMOS__) && (__PALMOS__ < 0x06000000)
  return StrNCaselessCompare (s1, s2, len);
#elif defined(__PALMOS__) || (!defined WIN32 && !defined _WIN32_WCE)
  return strncasecmp (s1, s2, len);
#else
  return _strnicmp (s1, s2, len);
#endif
}

char *
osip_strcasestr (const char *haystack, const char *needle)
{
  char c, sc;
  size_t len;

  if ((c = *needle++) != 0) {
    c = tolower ((unsigned char) c);
    len = strlen (needle);
    do {
      do {
        if ((sc = *haystack++) == 0)
          return (NULL);
      } while ((char) tolower ((unsigned char) sc) != c);
    } while (osip_strncasecmp (haystack, needle, len) != 0);
    haystack--;
  }
  return (char *) haystack;
}

#endif

/* remove SPACE before and after the content */
int
osip_clrspace (char *word)
{
  char *pbeg;
  char *pend;
  size_t len;

  if (word == NULL)
    return OSIP_UNDEFINED_ERROR;
  if (*word == '\0')
    return OSIP_SUCCESS;
  len = strlen (word);

  pbeg = word;
  while ((' ' == *pbeg) || ('\r' == *pbeg) || ('\n' == *pbeg) || ('\t' == *pbeg))
    pbeg++;

  pend = word + len - 1;
  while ((' ' == *pend) || ('\r' == *pend) || ('\n' == *pend) || ('\t' == *pend)) {
    pend--;
    if (pend < pbeg) {
      *word = '\0';
      return OSIP_SUCCESS;
    }
  }

  /* Add terminating NULL only if we've cleared room for it */
  if (pend + 1 <= word + (len - 1))
    pend[1] = '\0';

  if (pbeg != word)
    memmove (word, pbeg, pend - pbeg + 2);

  return OSIP_SUCCESS;
}

/* __osip_set_next_token:
   dest is the place where the value will be allocated
   buf is the string where the value is searched
   end_separator is the character that MUST be found at the end of the value
   next is the final location of the separator + 1

   the element MUST be found before any "\r" "\n" "\0" and
   end_separator

   return -1 on error
   return 1 on success
*/
int
__osip_set_next_token (char **dest, char *buf, int end_separator, char **next)
{
  char *sep;                    /* separator */

  *next = NULL;

  sep = buf;
  while ((*sep != end_separator) && (*sep != '\0') && (*sep != '\r')
         && (*sep != '\n'))
    sep++;
  if ((*sep == '\r') || (*sep == '\n')) {       /* we should continue normally only if this is the separator asked! */
    if (*sep != end_separator)
      return OSIP_UNDEFINED_ERROR;
  }
  if (*sep == '\0')
    return OSIP_UNDEFINED_ERROR;        /* value must not end with this separator! */
  if (sep == buf)
    return OSIP_UNDEFINED_ERROR;        /* empty value (or several space!) */

  *dest = osip_malloc (sep - (buf) + 1);
  if (*dest == NULL)
    return OSIP_NOMEM;
  osip_strncpy (*dest, buf, sep - buf);

  *next = sep + 1;              /* return the position right after the separator */
  return OSIP_SUCCESS;
}

#if 0
/*  not yet done!!! :-)
 */
int
__osip_set_next_token_better (char **dest, char *buf, int end_separator, int *forbidden_tab[], int size_tab, char **next)
{
  char *sep;                    /* separator */

  *next = NULL;

  sep = buf;
  while ((*sep != end_separator) && (*sep != '\0') && (*sep != '\r')
         && (*sep != '\n'))
    sep++;
  if ((*sep == '\r') && (*sep == '\n')) {       /* we should continue normally only if this is the separator asked! */
    if (*sep != end_separator)
      return OSIP_UNDEFINED_ERROR;
  }
  if (*sep == '\0')
    return OSIP_UNDEFINED_ERROR;        /* value must not end with this separator! */
  if (sep == buf)
    return OSIP_UNDEFINED_ERROR;        /* empty value (or several space!) */

  *dest = osip_malloc (sep - (buf) + 1);
  if (*dest == NULL)
    return OSIP_NOMEM;
  osip_strncpy (*dest, buf, sep - buf);

  *next = sep + 1;              /* return the position right after the separator */
  return 1;
}
#endif

/* in quoted-string, many characters can be escaped...   */
/* __osip_quote_find returns the next quote that is not escaped */
char *
__osip_quote_find (const char *qstring)
{
  char *quote;

  quote = strchr (qstring, '"');
  if (quote == qstring)         /* the first char matches and is not escaped... */
    return quote;

  if (quote == NULL)
    return NULL;                /* no quote at all... */

  /* this is now the nasty cases where '"' is escaped
     '" jonathan ros \\\""'
     |                  |
     '" jonathan ros \\"'
     |                |
     '" jonathan ros \""'
     |                |
     we must count the number of preceeding '\' */
  {
    int i = 1;

    for (;;) {
      if (0 == strncmp (quote - i, "\\", 1))
        i++;
      else {
        if (i % 2 == 1)         /* the '"' was not escaped */
          return quote;

        /* else continue with the next '"' */
        quote = strchr (quote + 1, '"');
        if (quote == NULL)
          return NULL;
        i = 1;
      }
      if (quote - i == qstring - 1)
        /* example: "\"john"  */
        /* example: "\\"jack" */
      {
        /* special case where the string start with '\' */
        if (*qstring == '\\')
          i++;                  /* an escape char was not counted */
        if (i % 2 == 0)         /* the '"' was not escaped */
          return quote;
        else {                  /* else continue with the next '"' */
          qstring = quote + 1;  /* reset qstring because
                                   (*quote+1) may be also == to '\\' */
          quote = strchr (quote + 1, '"');
          if (quote == NULL)
            return NULL;
          i = 1;
        }

      }
    }
    return NULL;
  }
}

char *
osip_enquote (const char *s)
{
  char *rtn;
  char *t;

  t = rtn = osip_malloc (strlen (s) * 2 + 3);
  if (rtn == NULL)
    return NULL;
  *t++ = '"';
  for (; *s != '\0'; s++) {
    switch (*s) {
    case '"':
    case '\\':
    case 0x7f:
      *t++ = '\\';
      *t++ = *s;
      break;
    case '\n':
    case '\r':
      *t++ = ' ';
      break;
    default:
      *t++ = *s;
      break;
    }
  }
  *t++ = '"';
  *t++ = '\0';
  return rtn;
}

void
osip_dequote (char *s)
{
  size_t len;

  if (*s == '\0')
    return;
  if (*s != '"')
    return;
  len = strlen (s);
  memmove (s, s + 1, len--);
  if (len > 0 && s[len - 1] == '"')
    s[--len] = '\0';
  for (; *s != '\0'; s++, len--) {
    if (*s == '\\')
      memmove (s, s + 1, len--);
  }
}

/**********************************************************/
/* only MACROS from osip/trace.h should be used by others */
/* TRACE_INITIALIZE(level,file))                          */
/* TRACE_ENABLE_LEVEL(level)                              */
/* TRACE_DISABLE_LEVEL(level)                             */
/* IS_TRACE_LEVEL_ACTIVATE(level)                         */
/**********************************************************/
#ifndef ENABLE_TRACE
void
osip_trace_initialize_func (osip_trace_level_t level, osip_trace_func_t * func)
{
}

void
osip_trace_initialize_syslog (osip_trace_level_t level, char *ident)
{
}

int
osip_trace_initialize (osip_trace_level_t level, FILE * file)
{
  return OSIP_UNDEFINED_ERROR;
}

void
osip_trace_enable_level (osip_trace_level_t level)
{
}

void
osip_trace_disable_level (osip_trace_level_t level)
{
}

int
osip_is_trace_level_activate (osip_trace_level_t level)
{
  return LOG_FALSE;
}

#else

/* initialize log */
/* all lower levels of level are logged in file. */
int
osip_trace_initialize (osip_trace_level_t level, FILE * file)
{
  osip_trace_level_t i = TRACE_LEVEL0;

  /* enable trace in log file by default */
  logfile = NULL;
  if (file != NULL)
    logfile = file;
#ifndef SYSTEM_LOGGER_ENABLED
  else
    logfile = stdout;
#endif

  /* enable all lower levels */
  while (i < END_TRACE_LEVEL) {
    if (i < level)
      tracing_table[i] = LOG_TRUE;
    else
      tracing_table[i] = LOG_FALSE;
    i++;
  }
  return 0;
}

void
osip_trace_initialize_syslog (osip_trace_level_t level, char *ident)
{
  osip_trace_level_t i = TRACE_LEVEL0;

#if defined (HAVE_SYSLOG_H) && !defined(__arc__)
  openlog (ident, LOG_CONS | LOG_PID, LOG_DAEMON);
  use_syslog = 1;
#endif
  /* enable all lower levels */
  while (i < END_TRACE_LEVEL) {
    if (i < level)
      tracing_table[i] = LOG_TRUE;
    else
      tracing_table[i] = LOG_FALSE;
    i++;
  }
}

void
osip_trace_enable_until_level (osip_trace_level_t level)
{
  int i = 0;

  while (i < END_TRACE_LEVEL) {
    if (i < level)
      tracing_table[i] = LOG_TRUE;
    else
      tracing_table[i] = LOG_FALSE;
    i++;
  }
}

void
osip_trace_initialize_func (osip_trace_level_t level, osip_trace_func_t * func)
{
  int i = 0;

  trace_func = func;

  /* enable all lower levels */
  while (i < END_TRACE_LEVEL) {
    if (i < level)
      tracing_table[i] = LOG_TRUE;
    else
      tracing_table[i] = LOG_FALSE;
    i++;
  }
}

/* enable a special debugging level! */
void
osip_trace_enable_level (osip_trace_level_t level)
{
  tracing_table[level] = LOG_TRUE;
}

/* disable a special debugging level! */
void
osip_trace_disable_level (osip_trace_level_t level)
{
  tracing_table[level] = LOG_FALSE;
}

/* not so usefull? */
int
osip_is_trace_level_activate (osip_trace_level_t level)
{
  return tracing_table[level];
}
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)

#include <time.h>
#include <sys/timeb.h>

#elif defined(__linux)
#include <sys/time.h>
#define __osip_port_gettimeofday gettimeofday
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#ifndef MAX_LENGTH_TR
#ifdef SYSTEM_LOGGER_ENABLED
#define MAX_LENGTH_TR 2024      /* NEVER DEFINE MORE THAN 2024 */
#else
#define MAX_LENGTH_TR 512       /* NEVER DEFINE MORE THAN 2024 */
#endif
#endif

int
osip_trace (char *filename_long, int li, osip_trace_level_t level, FILE * f, char *chfr, ...)
{
#ifdef ENABLE_TRACE
  va_list ap;
  int relative_time = 0;

  char *fi = NULL;

#if (defined(WIN32)  && !defined(_WIN32_WCE)) || defined(__linux)
  static struct timeval start = { 0, 0 };
  struct timeval now;

  int myInt1;
  int myInt2;
  int myInt3;
  int myInt4;

  klee_make_symbolic(&myInt1,sizeof(myInt1),"myInt1");
  klee_make_symbolic(&myInt2,sizeof(myInt2),"myInt2");
  klee_make_symbolic(&myInt3,sizeof(myInt3),"myInt3");
  klee_make_symbolic(&myInt4,sizeof(myInt4),"myInt4");

  if (start.tv_sec == 0 && start.tv_usec == 0) {
    // __osip_port_gettimeofday (&start, NULL);
    start.tv_sec  = myInt1;
    start.tv_usec = myInt2;
  }
  // __osip_port_gettimeofday (&now, NULL);
  now.tv_sec  = myInt3;
  now.tv_usec = myInt4;

  relative_time = 1000 * (now.tv_sec - start.tv_sec);
  if (now.tv_usec - start.tv_usec > 0)
    relative_time = relative_time + ((now.tv_usec - start.tv_usec) / 1000);
  else
    relative_time = relative_time - 1 + ((now.tv_usec - start.tv_usec) / 1000);
#endif

  fprintf(stdout,">>>>>>>>>>>>>>>>>>>>>>>>>>> NOW INSIDE OSIP TRACE FUNCTION (%s)\n\n\n",filename_long);

  if (filename_long != NULL) {
    fi = strrchr (filename_long, '/');
    if (fi == NULL)
      fi = strrchr (filename_long, '\\');
    if (fi != NULL)
      fi++;
    if (fi == NULL)
      fi = filename_long;
  }

#if !defined(WIN32) && !defined(SYSTEM_LOGGER_ENABLED)
  if (logfile == NULL && use_syslog == 0 && trace_func == NULL) {       /* user did not initialize logger.. */
    return 1;
  }
#endif

  if (tracing_table[level] == LOG_FALSE)
    return OSIP_SUCCESS;

  if (f == NULL && trace_func == NULL)
    f = logfile;

  VA_START (ap, chfr);

#if  defined(__VXWORKS_OS__) || defined(__rtems__)
  /* vxworks can't have a local file */
  f = stdout;
#endif

  if (0) {
  }
#ifdef ANDROID
  else if (trace_func == 0) {
    int lev;

    switch (level) {
    case OSIP_INFO3:
      lev = ANDROID_LOG_DEBUG;
      break;
    case OSIP_INFO4:
      lev = ANDROID_LOG_DEBUG;
      break;
    case OSIP_INFO2:
      lev = ANDROID_LOG_INFO;
      break;
    case OSIP_INFO1:
      lev = ANDROID_LOG_INFO;
      break;
    case OSIP_WARNING:
      lev = ANDROID_LOG_WARN;
      break;
    case OSIP_ERROR:
      lev = ANDROID_LOG_ERROR;
      break;
    case OSIP_BUG:
      lev = ANDROID_LOG_FATAL;
      break;
    case OSIP_FATAL:
      lev = ANDROID_LOG_FATAL;
      break;
    default:
      lev = ANDROID_LOG_DEFAULT;
      break;
    }
    __android_log_vprint (lev, "osip2", chfr, ap);
  }
#elif defined(__APPLE__)  && defined(__OBJC__)
  else if (trace_func == 0) {
    char buffer[MAX_LENGTH_TR];
    int in = 0;

    memset (buffer, 0, sizeof (buffer));
    if (level == OSIP_FATAL)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| FATAL | <%s: %i> ", fi, li);
    else if (level == OSIP_BUG)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "|  BUG  | <%s: %i> ", fi, li);
    else if (level == OSIP_ERROR)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| ERROR | <%s: %i> ", fi, li);
    else if (level == OSIP_WARNING)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "|WARNING| <%s: %i> ", fi, li);
    else if (level == OSIP_INFO1)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO1 | <%s: %i> ", fi, li);
    else if (level == OSIP_INFO2)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO2 | <%s: %i> ", fi, li);
    else if (level == OSIP_INFO3)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO3 | <%s: %i> ", fi, li);
    else if (level == OSIP_INFO4)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO4 | <%s: %i> ", fi, li);

    vsnprintf (buffer + in, MAX_LENGTH_TR - 1 - in, chfr, ap);
    NSLog (@"%s", buffer);
  }
#endif
  else if (f && use_syslog == 0) {
    if (level == OSIP_FATAL)
      fprintf (f, "| FATAL | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_BUG)
      fprintf (f, "|  BUG  | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_ERROR)
      fprintf (f, "| ERROR | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_WARNING)
      fprintf (f, "|WARNING| %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_INFO1)
      fprintf (f, "| INFO1 | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_INFO2)
      fprintf (f, "| INFO2 | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_INFO3)
      fprintf (f, "| INFO3 | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_INFO4)
      fprintf (f, "| INFO4 | %i <%s: %i> ", relative_time, fi, li);

    vfprintf (f, chfr, ap);

    fflush (f);
  }
  else if (trace_func) {
    trace_func (fi, li, level, chfr, ap);
  }

#if defined (HAVE_SYSLOG_H) && !defined(__arc__)
  else if (use_syslog == 1) {
    char buffer[MAX_LENGTH_TR];
    int in = 0;

    memset (buffer, 0, sizeof (buffer));
    if (level == OSIP_FATAL)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| FATAL | <%s: %i> ", fi, li);
    else if (level == OSIP_BUG)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "|  BUG  | <%s: %i> ", fi, li);
    else if (level == OSIP_ERROR)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| ERROR | <%s: %i> ", fi, li);
    else if (level == OSIP_WARNING)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "|WARNING| <%s: %i> ", fi, li);
    else if (level == OSIP_INFO1)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO1 | <%s: %i> ", fi, li);
    else if (level == OSIP_INFO2)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO2 | <%s: %i> ", fi, li);
    else if (level == OSIP_INFO3)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO3 | <%s: %i> ", fi, li);
    else if (level == OSIP_INFO4)
      in = snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO4 | <%s: %i> ", fi, li);

    vsnprintf (buffer + in, MAX_LENGTH_TR - 1 - in, chfr, ap);
    if (level == OSIP_FATAL)
      syslog (LOG_ERR, "%s", buffer);
    else if (level == OSIP_BUG)
      syslog (LOG_ERR, "%s", buffer);
    else if (level == OSIP_ERROR)
      syslog (LOG_ERR, "%s", buffer);
    else if (level == OSIP_WARNING)
      syslog (LOG_WARNING, "%s", buffer);
    else if (level == OSIP_INFO1)
      syslog (LOG_INFO, "%s", buffer);
    else if (level == OSIP_INFO2)
      syslog (LOG_INFO, "%s", buffer);
    else if (level == OSIP_INFO3)
      syslog (LOG_DEBUG, "%s", buffer);
    else if (level == OSIP_INFO4)
      syslog (LOG_DEBUG, "%s", buffer);
  }
#endif
#ifdef SYSTEM_LOGGER_ENABLED
  else {
    char buffer[MAX_LENGTH_TR];
    int in = 0;

#ifdef DISPLAY_TIME
    int relative_time;
#endif

    memset (buffer, 0, sizeof (buffer));
    if (level == OSIP_FATAL)
      in = _snprintf (buffer, MAX_LENGTH_TR - 1, "| FATAL | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_BUG)
      in = _snprintf (buffer, MAX_LENGTH_TR - 1, "|  BUG  | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_ERROR)
      in = _snprintf (buffer, MAX_LENGTH_TR - 1, "| ERROR | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_WARNING)
      in = _snprintf (buffer, MAX_LENGTH_TR - 1, "|WARNING| %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_INFO1)
      in = _snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO1 | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_INFO2)
      in = _snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO2 | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_INFO3)
      in = _snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO3 | %i <%s: %i> ", relative_time, fi, li);
    else if (level == OSIP_INFO4)
      in = _snprintf (buffer, MAX_LENGTH_TR - 1, "| INFO4 | %i <%s: %i> ", relative_time, fi, li);

    _vsnprintf (buffer + in, MAX_LENGTH_TR - 1 - in, chfr, ap);
#ifdef UNICODE
    {
      WCHAR wUnicode[MAX_LENGTH_TR * 2];
      int size;

      size = MultiByteToWideChar (CP_UTF8, 0, buffer, -1, wUnicode, MAX_LENGTH_TR * 2);
      wUnicode[size - 2] = '\n';
      wUnicode[size - 1] = '\0';
      OutputDebugString (wUnicode);
    }
#else
    OutputDebugString (buffer);
#endif
  }
#endif

  va_end (ap);
#endif
  return OSIP_SUCCESS;
}



#if defined(WIN32) || defined(_WIN32_WCE)

#undef osip_malloc
void *
osip_malloc (size_t size)
{
  void *ptr = malloc (size);

  if (ptr != NULL)
    memset (ptr, 0, size);
  return ptr;
}

#undef osip_realloc
void *
osip_realloc (void *ptr, size_t size)
{
  return realloc (ptr, size);
}

#undef osip_free
void
osip_free (void *ptr)
{
  if (ptr == NULL)
    return;
  free (ptr);
}

#else

#ifndef MINISIZE
void
osip_set_allocators (osip_malloc_func_t * malloc_func, osip_realloc_func_t * realloc_func, osip_free_func_t * free_func)
{
  osip_malloc_func = malloc_func;
  osip_realloc_func = realloc_func;
  osip_free_func = free_func;
}
#endif

#endif

#if defined(__VXWORKS_OS__)

typedef struct {
  char *str;
  int max;
  int len;
} _context;

STATUS _cb_snprintf (char *buffer, int nc, int arg);

STATUS
_cb_snprintf (char *buffer, int nc, int arg)
{
  _context *ctx = (_context *) arg;

  if (ctx->max - ctx->len - nc < 1) {   /* retain 1 pos for terminating \0 */
    nc = ctx->max - ctx->len - 1;
  }

  if (nc > 0) {
    memcpy (ctx->str + ctx->len, buffer, nc);
    ctx->len += nc;
  }

  ctx->str[ctx->len] = '\0';

  return OK;
}


int
osip_vsnprintf (char *buf, int max, const char *fmt, va_list ap)
{
  _context ctx;

  ctx.str = buf;
  ctx.max = max;
  ctx.len = 0;

  if (fioFormatV (fmt, ap, _cb_snprintf, (int) &ctx) != OK) {
    return OSIP_UNDEFINED_ERROR;
  }

  return ctx.len;
}

int
osip_snprintf (char *buf, int max, const char *fmt, ...)
{
  int retval;
  va_list ap;

  va_start (ap, fmt);
  retval = osip_vsnprintf (buf, max, fmt, ap);
  va_end (ap);
  return retval;
}

#endif


#if defined(__PSOS__)

int
osip_snprintf (char *buf, int max, const char *fmt, ...)
{
  static char buffer[1024];
  int retval;
  va_list ap;

  buffer[0] = '\n';
  va_start (ap, fmt);
  vsprintf (&(buffer[strlen (buffer)]), fmt, ap);
  va_end (ap);
  retval = strlen (buffer);
  memmove (buf, buffer, max);
  if (retval > max)
    return OSIP_UNDEFINED_ERROR;
  return retval;
}

#endif

#ifdef DEBUG_MEM

/*
  This is a debug facility for detecting memory leaks.
  I recommend to use external tools such as mpatrol
  when possible. On some fancy platform, you may not
  have any usefull tools: in this case, use this code!
 */

void *
_osip_malloc (size_t size, char *file, unsigned short line)
{
  void *mem;

  mem = osip_malloc_func (size + 20);
  if (mem != NULL) {
    char *s;

    memcpy (mem, &line, 2);
    for (s = file + strlen (file); s != file; s--) {
      if (*s == '\\' || *s == '/') {
        s++;
        break;
      }
    }
    strncpy ((char *) mem + 2, s, 18);
    return (void *) ((char *) mem + 20);
  }
  return NULL;
}

void
_osip_free (void *ptr)
{
  if (ptr != NULL) {
    osip_free_func ((char *) ptr - 20);
  }
}

void *
_osip_realloc (void *ptr, size_t size, char *file, unsigned short line)
{
  void *mem;

  mem = osip_realloc_func ((char *) ptr - 20, size + 20);
  if (mem != NULL) {
    char *s;

    memcpy (mem, &line, 2);

    for (s = file + strlen (file); s != file; s--) {
      if (*s == '\\' || *s == '/') {
        s++;
        break;
      }
    }
    strncpy ((char *) mem + 2, s, 18);
    return (char *) mem + 20;
  }
  return NULL;
}

#endif

#if 0                           /* for windows test purpose */
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
/*
  This is a debug facility for detecting memory leaks.
  I recommend to use external tools such as mpatrol
  when possible. On some fancy platform, you may not
  have any usefull tools: in this case, use this code!
 */

void *
_osip_malloc (size_t size, char *file, unsigned short line)
{
  return _malloc_dbg (size, _NORMAL_BLOCK, file, line);
}

void
_osip_free (void *ptr)
{
  _free_dbg (ptr, _NORMAL_BLOCK);
}

void *
_osip_realloc (void *ptr, size_t size, char *file, unsigned short line)
{
  return _realloc_dbg (ptr, size, _NORMAL_BLOCK, file, line);
}
#endif

/* ---For better performance---
   Calculates a hash value for the given string */
unsigned long
osip_hash (const char *str)
{
  unsigned int hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;

  return hash & 0xFFFFFFFFu;
}

/* ---For better performance---
   Appends src-string to dst-string.
   
   This was introduced to replace the 
   inefficient constructions like:
   
   osip_strncpy (tmp, src, strlen(src) );
   tmp = tmp + strlen (src);
   
   This function returns a pointer to the
   end of the destination string
   
   Pre: src is null terminated */
char *
osip_str_append (char *dst, const char *src)
{
  while (*src != '\0') {
    *dst = *src;
    src++;
    dst++;
  }
  *dst = '\0';
  return dst;
}

/* ---For better performance---
   Same as above, only this time we know the length */
char *
osip_strn_append (char *dst, const char *src, size_t len)
{
  memmove ((void *) dst, (void *) src, len);
  dst += len;
  *dst = '\0';
  return dst;
}


/* ---For better performance---
   This is to replace this construction:
   osip_strncpy (  dest, source, length);
   osip_clrspace ( dest ); */
#ifndef MINISIZE
char *
osip_clrncpy (char *dst, const char *src, size_t len)
{
  const char *pbeg;
  const char *pend;
  char *p;
  size_t spaceless_length;

  if (src == NULL)
    return NULL;

  /* find the start of relevant text */
  pbeg = src;
  while ((' ' == *pbeg) || ('\r' == *pbeg) || ('\n' == *pbeg) || ('\t' == *pbeg))
    pbeg++;


  /* find the end of relevant text */
  pend = src + len - 1;
  while ((' ' == *pend) || ('\r' == *pend) || ('\n' == *pend) || ('\t' == *pend)) {
    pend--;
    if (pend < pbeg) {
      *dst = '\0';
      return dst;
    }
  }

  /* if pend == pbeg there is only one char to copy */
  spaceless_length = pend - pbeg + 1;   /* excluding any '\0' */
  memmove (dst, pbeg, spaceless_length);
  p = dst + spaceless_length;

#if defined(__GNUC__)
  /* terminate the string and pad dest with zeros until len.
     99% of the time (SPACELESS_LENGTH == LEN) or
     (SPACELESS_LENGHT + 1 == LEN). We handle these cases efficiently.  */
  *p = '\0';
  if (__builtin_expect (++spaceless_length < len, 0)) {
    do
      *++p = '\0';
    while (++spaceless_length < len);
  }
#else
  /* terminate the string and pad dest with zeros until len */
  do {
    *p = '\0';
    p++;
    spaceless_length++;
  }
  while (spaceless_length < len);
#endif

  return dst;
}

#else

char *
osip_clrncpy (char *dst, const char *src, size_t len)
{
  osip_strncpy (dst, src, len);
  osip_clrspace (dst);
  return dst;
}

#endif
