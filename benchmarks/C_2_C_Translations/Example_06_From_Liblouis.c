static char *strdupWrapper (const char *string)
{
	char *address = strdup (string);
	if (!address) return NULL;
	
	return address;
}

char *foo(void)
{
	while (1)
	{
		/* ... */
		if (length < size)
		{
			buffer[length] = 0;
			path = strdupWrapper (buffer);
			while (length > 0)
			{
				if (path[--length] == '\\')
				{
					break;
				}
			}
			strncpy (path, path, length + 1);
			path[length + 1] = '\0';
			break;
		}
	}
	return path;
}
