#define SCHEME_CHAR(ch) (ISALNUM (ch) || (ch) == '-' || (ch) == '+')

/* Return 1 if the URL begins with any "scheme", 0 otherwise.  As
   currently implemented, it returns true if URL begins with
   [-+a-zA-Z0-9]+: .  */

int
url_has_scheme (const char *url)
{
  const char *p = url;

  /* The first char must be a scheme char. */
  if (!*p || !SCHEME_CHAR (*p))
    return 0;
  ++p;
  /* Followed by 0 or more scheme chars. */
  while (*p && SCHEME_CHAR (*p))
    ++p;
  /* Terminated by ':'. */
  return *p == ':';
}

