# 
# Convert Brewer data to same RGB format used in color_names.
# See brewer_colors for input format.
#
BEGIN { 
  FS = ",";
}
/^[^#]/{
  if ($1 != "") {
    name = "colortmp/" $1 $2;
    gsub ("\"","",name);
  }
  printf ("%s %s %s %s\n", $5, $7, $8, $9) > name; 
}
