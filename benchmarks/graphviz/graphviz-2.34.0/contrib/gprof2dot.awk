# Copyright (c) AT&T Corp. 1994, 1995.
# This code is licensed by AT&T Corp.  For the
# terms and conditions of the license, see
# http://www.research.att.com/orgs/ssr/book/reuse

# remove leading underscore, then translate leading dot to underscore
function canon(s) {
	gsub(/\./,"_",s);
	gsub(/^_*/,"",s);
	gsub(/_*$/,"",s);
	return s;
}

BEGIN			{ 
	maxhue = .65
	minsaturation = .2;
	brightness = 1.0;
	printf ("digraph prof {\n");
	printf ("	node [style=filled];\n");
	}

$1 ~ /\[/ && $0 !~ "as a"		{
					if (NF == 6) {
						tail = $5
						source = 1
					}
					else {
						tail = $6
						source = 0
					}
					mytotal = $4

					while (getline) {
						if (NF == 0) {
#							if (source == 1) printf("\t%s;\n",canon(tail));
							break
						}
						if ( $2 ~ /\./ ) {
							ntc = $3;
							gsub("/.*","",ntc);
							if (mytotal < 0.0000000001)
								color = 0.0
							else
								color = $2/(mytotal + .01);
							color = color/ntc;

							tailc = canon(tail);
							headc = canon($4);
							hue = maxhue * (1.0 - color);
							saturation = minsaturation
								+ (1.0 - minsaturation) * color; 
							brightness = .7 + .3*color;
							if ((tailc != "") && (headc != "")) {
								printf ("\t%s -> %s [color=\"%.3f %.3f %.3f\"];\n",
								# tailc, headc, hue, saturation, brightness);
								tailc, headc, hue, brightness, brightness);
								Degree[tailc]++; Degree[headc]++;
							}
							else { 		# Recursive call
								printf ("\t%s -> %s;\n ", canon(tail), canon($2))
							}
						}
				}
		}

$2 == "cumulative" {
	while ($1 != "time") getline;
	getline;
	scale = $1;
	brightness = 1.0;
	while (NF > 0) {
		if ($1 == "") exit;
		if ($(NF - 2) == "<cycle") func_name = $(NF - 3);
		else func_name = $(NF - 1);
		func_name = canon(func_name);
		if (Degree[func_name] > 0) {
			hue = maxhue * (1.0 - $1/scale);
			saturation = minsaturation + ((1.0 - minsaturation) * $1/scale);
			printf("%s [color=\"%.3f %.3f %.3f\"];\n",func_name,
				hue,saturation,brightness);
		}
		getline;
	}
	exit;
}

END				{ print "}" }
