/* Return the location of STR's suffix (file extension).  Examples:
   suffix ("foo.bar")       -> "bar"
   suffix ("foo.bar.baz")   -> "baz"
   suffix ("/foo/bar")      -> NULL
   suffix ("/foo.bar/baz")  -> NULL  */
char *suffix (const char *str)
{
	int i;

	for (i = strlen (str); i && str[i] != '/' && str[i] != '.'; i--);

	if (str[i++] == '.')
	{
		return (char *)str + i;
	}
	else
	{
		return NULL;
	}
}
