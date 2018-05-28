char *
rewrite_shorthand_url (const char *url)
{
	/* ... */

	int digits = 0;
	const char *pp;
	for (pp = p + 1; ISDIGIT (*pp); pp++)
	{
		++digits;
	}
}
