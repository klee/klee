BEGIN {
	FS = "[ ,()]*";
}
/^[ 	]*$/    { next; }
/^#/    { next; }
{
	printf ("%s %s %s %s\n", $1, $5, $6, $7);
}
