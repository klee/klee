static char *construct_relative (const char *basefile, const char *linkfile)
{
	char *link;
	int basedirs;
	const char *b, *l;
	int i, start;

	/* First, skip the initial directory components common to both
	files.  */
	start = 0;
	for (b = basefile, l = linkfile; *b == *l && *b != '\0'; ++b, ++l)
	{
		if (*b == '/')
		{
			start = (b - basefile) + 1;
		}
	}
	basefile += start;
	linkfile += start;

	/* With common directories out of the way, the situation we have is
	as follows:
	b - b1/b2/[...]/bfile
	l - l1/l2/[...]/lfile

	The link we're constructing needs to be:
		lnk - ../../l1/l2/[...]/lfile

	Where the number of ".."'s equals the number of bN directory
	components in B.  */

	/* Count the directory components in B. */
	basedirs = 0;
	for (b = basefile; *b; b++)
	{
		if (*b == '/')
		{
			++basedirs;
		}
    }

	/* Construct LINK as explained above. */
	link = (char *)xmalloc (3 * basedirs + strlen (linkfile) + 1);
	for (i = 0; i < basedirs; i++)
	{
		memcpy (link + 3 * i, "../", 3);
	}
	strcpy (link + 3 * i, linkfile);
	return link;
}

