char *escape_reason_phrase (const char *reason_phrase_ptr)
{
	char *buf, c;
	int i, j;

	/* We encode each byte using at most 4 bytes, + trailing '\0'. */
	buf = malloc (4 * strlen (reason_phrase_ptr) + 1);

	for (i=j=0; (c = reason_phrase_ptr[i]) != '\0'; ++i)
	{
		if ((c == '\t' || c == ' ') && (j == 0 || buf[j-1] != ' '))
		{
			buf[j++] = ' ';
		/* We must assure the closing ``<<<'' can't be spoofed. */
		}
		else if (c == '<' || c == '\\')
		{
			buf[j++] = '\\';
			buf[j++] = c;

		/*
		 * wget + <ctype.h> is broken in stable (see changelog for
		 * version 1.9-1).  Therefore no isprint().  Confining the
		 * range to printable ASCII.
		 */

		/* } else if (isprint (c)) { */
		}
		else if (c >= 32 && c < 127)
		{
			buf[j++] = c;
		}
		else
		{
			buf[j++] = '\\';
			buf[j++] = '0' + (c >> 6);
			buf[j++] = '0' + ((c & 0x3f) >> 3);
			buf[j++] = '0' + (c & 7);
		}
	}
	buf[j] = '\0';
	return (buf);
}
