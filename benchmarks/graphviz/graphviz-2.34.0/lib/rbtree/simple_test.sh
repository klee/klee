#!/bin/sh
eval `dmalloc -l logfile -i 100 high`
./test_rb<<EOF
1
1
1
2
1
3
1
4
1
5
7
1
9
1 
11
1 
-87
3 
10
3 
-87
3 
6
4 
3
3 
99
5 
2
5 
99
5 
1
6
-1
10
2
2
2
4
7
8
EOF

grep "not freed" logfile | tee unfreed.txt

echo "DMALLOC_OPTIONS were :"
printenv DMALLOC_OPTIONS

exit 0