/* Debugging and testing support for path_simplify. */

/* Debug: run path_simplify on PATH and return the result in a new
   string.  Useful for calling from the debugger.  */

#include <stdio.h>
#include <string.h>

static int path_simplify (char *path);
   
static char *
ps (char *path)
{
  char *copy = strdup (path);
  path_simplify (copy);
  return copy;
}

static void
run_test (char *test, char *expected_result, int expected_change)
{
  char *test_copy = strdup (test);
  int modified = path_simplify (test_copy);

  if (0 != strcmp (test_copy, expected_result))
    {
      printf ("Failed path_simplify(\"%s\"): expected \"%s\", got \"%s\".\n",
	      test, expected_result, test_copy);
    }
  if (modified != expected_change)
    {
      if (expected_change == 1)
	printf ("Expected modification with path_simplify(\"%s\").\n",
		test);
      else
	printf ("Expected no modification with path_simplify(\"%s\").\n",
		test);
    }
  free (test_copy);
}

static void
test_path_simplify (void)
{
	static struct {
	char *test, *result;
	int should_modify;
	}
	tests[] = {
    { "",							"",			0 },
    { ".",							"",			1 },
    { "./",							"",			1 },
    { "..",							"..",		0 },
    { "../",						"../",		0 },
    { "foo",						"foo",		0 },
    { "foo/bar",					"foo/bar",	0 },
    { "foo///bar",					"foo///bar",0 },
    { "foo/.",						"foo/",		1 },
    { "foo/./",						"foo/",		1 },
    { "foo./",						"foo./",	0 },
    { "foo/../bar",					"bar",		1 },
    { "foo/../bar/",				"bar/",		1 },
    { "foo/bar/..",					"foo/",		1 },
    { "foo/bar/../x",				"foo/x",	1 },
    { "foo/bar/../x/",				"foo/x/",	1 },
    { "foo/..",						"",			1 },
    { "foo/../..",					"..",		1 },
    { "foo/../../..",				"../..",	1 },
    { "foo/../../bar/../../baz",	"../../baz",1 },
    { "a/b/../../c",				"c",		1 },
    { "./a/../b",					"b",		1 }
    };
  int i;

  for (i = 0; i < 22; i++)
    {
      char *test = tests[i].test;
      char *expected_result = tests[i].result;
      int   expected_change = tests[i].should_modify;
      run_test (test, expected_result, expected_change);
    }
}



/* Resolve "." and ".." elements of PATH by destructively modifying
   PATH and return non-zero if PATH has been modified, zero otherwise.

   The algorithm is in spirit similar to the one described in rfc1808,
   although implemented differently, in one pass.  To recap, path
   elements containing only "." are removed, and ".." is taken to mean
   "back up one element".  Single leading and trailing slashes are
   preserved.

   For example, "a/b/c/./../d/.." will yield "a/b/".  More exhaustive
   test examples are provided below.  If you change anything in this
   function, run test_path_simplify to make sure you haven't broken a
   test case.  */

static int path_simplify (char *path)
{
	char *h = path;		/* hare */
	char *t = path;		/* tortoise */
	char *beg = path;	/* boundary for backing the tortoise */
	char *end = path + strlen (path);

	while (h < end)
	{
		/* Hare should be at the beginning of a path element. */

		if (h[0] == '.' && (h[1] == '/' || h[1] == '\0'))
		{
			/* Ignore "./". */
			h += 2;
		}
		else if (h[0] == '.' && h[1] == '.' && (h[2] == '/' || h[2] == '\0'))
		{
			/* Handle "../" by retreating the tortoise by one path
			element -- but not past beggining.  */
			if (t > beg)
			{
				/* Move backwards until T hits the beginning of the
				previous path element or the beginning of path. */
				for (--t; t > beg && t[-1] != '/'; t--);
			}
			else
			{
				/* If we're at the beginning, copy the "../" literally
				move the beginning so a later ".." doesn't remove
				it.  */
				beg = t + 3;
				goto regular;
			}
			h += 3;
		}
		else
		{
			regular:
			/* A regular path element.  If H hasn't advanced past T,
			simply skip to the next path element.  Otherwise, copy
			the path element until the next slash.  */
			if (t == h)
			{
				/* Skip the path element, including the slash.  */
				while (h < end && *h != '/')
				{
					t++;
					h++;
				}
				if (h < end)
				{
					t++;
					h++;
				}
			}
			else
			{
				/* Copy the path element, including the final slash.  */
				while (h < end && *h != '/')
				{
					*t++ = *h++;
				}
				if (h < end)
				{
					*t++ = *h++;
				}
			}
		}
    }

	if (t != h)
	{
		*t = '\0';
	}

	return t != h;
}

int main(int argc, char **argv)
{
	const char path[128]={0};
	strcpy(path,"foo/../../bar/../../baz");
	printf("before: %s\nafter: ",path);
	path_simplify(path);
	printf("%s\n",path);
}
