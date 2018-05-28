char *
rewrite_shorthand_url (const char *url)
{
	const char *p;

	/* Look for a ':' or '/'.  The former signifies NcFTP syntax, the latter Netscape.  */
	for (p = url; *p && *p != ':' && *p != '/'; p++);
	
	/* ... */
}
