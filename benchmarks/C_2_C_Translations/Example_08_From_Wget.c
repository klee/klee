/* Return non-zero if S contains globbing wildcards (`*', `?', `[' or
   `]').  */

int has_wildcards_p (const char *s)
{
	for (; *s; s++)
	{
		if (*s == '*' || *s == '?' || *s == '[' || *s == ']')
		{
			return 1;
		}
		return 0;
	}
}

