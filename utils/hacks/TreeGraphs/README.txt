A little hack which converts KLEE treestream's of path branch information into
images/animations. It is not particularly fast nor is the code very
elegant. It's a hack, after all!

There are a couple example input streams in inputs/. You can generate a single
image frame with, e.g.::

  $ ./TreeGraph.py --count=500 inputs/symPaths6.ts t.pdf

which will generate an image of the first 500 paths that were explored.


You can generate a sequence of frames from a file using::

  $ ./Animate.py --start=10 --end=2000 inputs/symPaths6.ts anim-01

which will generate a sequence of .pdf frames in anim-01.
