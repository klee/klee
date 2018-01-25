#!/bin/sh
# Written by: John Ellson <ellson@research.att.com>

error() { echo "$0: $*" >&2; exit 1; }

for prog in gvim vim ""; do
	if test -x /usr/bin/$prog; then break; fi
	if which $prog >/dev/null 2>&1; then break; fi
done

if test -z "$prog"; then error "the editor not found"; fi

default="noname.gv"

if test -z "$1"; then
	if test -s "$default"; then
		error "$default already exists"
	else
		f="$default"
	fi
else
	f="$1"
fi

if ! test -f "$f"; then
	if ! test -w .; then error "directory `pwd` is not writable"; fi
	cat >"$f" <<EOF
digraph G {
	graph [layout=dot rankdir=LR]

// This is just an example for you to use as a template.
// Edit as you like. Whenever you save a legal graph
// the layout in the graphviz window will be updated.

	vim [href="http://www.vim.org/"]
	dot [href="http://www.graphviz.org/"]
	vimdot [href="file:///usr/bin/vimdot"]

	{vim dot} -> vimdot
}
EOF
fi
if ! test -w "$f"; then error "$f is not writable"; fi

# dot -Txlib watches the file $f for changes using inotify()
dot -Txlib "$f" &
# open an editor on the file $f (could be any editor; gvim &'s itself)
exec $prog "$f"
