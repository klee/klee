static int match_except_index (const char *s1, const char *s2)
{
	int i;
	const char *lng;

	/* Skip common substring. */
	for (i = 0; *s1 && *s2 && *s1 == *s2; s1++, s2++, i++);
	
	if (i == 0)
	{
		/* Strings differ at the very beginning -- bail out.  We need to
		check this explicitly to avoid `lng - 1' reading outside the
		array.  */
		return 0;
	}

	if (!*s1 && !*s2)
	{
		/* Both strings hit EOF -- strings are equal. */
		return 1;
	}
	else if (*s1 && *s2)
	{
		/* Strings are randomly different, e.g. "/foo/bar" and "/foo/qux". */
		return 0;
	}
	else if (*s1)
	{
		/* S1 is the longer one. */
		lng = s1;
	}
	else
	{
		/* S2 is the longer one. */
		lng = s2;
	}

	/* foo            */            /* foo/           */
	/* foo/index.html */  /* or */  /* foo/index.html */
	/*    ^           */            /*     ^          */

	if (*lng != '/')
	{
		/* The right-hand case. */
		--lng;
	}

	if (*lng == '/' && *(lng + 1) == '\0')
	{
		/* foo  */
		/* foo/ */
		return 1;
	}

	return 0 == strcmp (lng, "/index.html");
}

