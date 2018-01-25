function max(m,n) {
  return m > n ? m : n;
}
function value (r, g, b) {
  return max(r,max(g,b));
}
function putColor (n, r, g, b, v)
{
  if (length(n) > 4) p = "";
  else p = "&nbsp;&nbsp;&nbsp;";
  printf ("<td bgcolor=#%02x%02x%02x><a title=#%02x%02x%02x>",r,g,b,r,g,b); 
  if (v < 0.51) printf ("<font color=white>%s%s%s</font>", p,n,p);
  else printf ("%s%s%s", p,n,p);
  printf ("</a></td>\n");
}
BEGIN {
  colorsPerRow = 5;
  if (ARGV[1] == "-s") {
    ARGV[1] = "";
    name = ARGV[2];
    singleRow = 1;
  }
  else {
    name = "";
    singleRow = 0;
  }
  if (length(name) > 0) {
    sub(".*/","",name);
    printf ("%s color scheme<BR>\n", name);
  }
  printf ("<table border=1 align=center>\n");
}
{
  if (singleRow) idx = NR;
  else idx = NR % colorsPerRow;
  if (idx == 1) printf ("<tr align=center>\n");
  putColor($1,$2,$3,$4,value($2/255.0,$3/255.0,$4/255.0));
  if (idx == 0) printf ("</tr>\n");
}
END {
  printf ("</table><HR>\n");
}
