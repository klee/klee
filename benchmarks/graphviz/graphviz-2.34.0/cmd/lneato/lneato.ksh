
FILES=""
MLEVEL="0"
LMODE="async"
FLAGS=

function usage {
    print "usage: lneato [-V] [-lm (sync|async)] [-el (0|1)] <filename>"
}

function processoptions {
    while [[ $# > 0 ]] do
        case $1 in
        -V)
            print "lneato version 96c (09-24-96)"
			FLAGS=$FLAGS" -V"
            shift
            ;;
        -f)
            shift
            loadfile=$1
            shift
            ;;
        -lm)
            shift
            LMODE=$1
            if [[ $LMODE != 'sync' && $LMODE != 'async' ]] then
                usage
                exit 1
            fi
            shift
            ;;
        -el)
            shift
            MLEVEL=$1
            if [[ $MLEVEL != '0' && $MLEVEL != '1' ]] then
                usage
                exit 1
            fi
            shift
            ;;
        -)
            FILES=$(print $FILES "'"$1"'")
            shift
            ;;
        -*)
            usage
            exit 1
            ;;
        *)
            FILES=$(print $FILES "'"$1"'")
            shift
            ;;
        esac
    done
}

if [[ $DOTTYOPTIONS != '' ]] then
    processoptions $DOTTYOPTIONS
fi
processoptions "$@"

if [[ $DOTTYPATH != '' ]] then
    export LEFTYPATH="$DOTTYPATH:$LEFTYPATH"
fi

CMDS=""

CMDS="dotty.protogt.layoutmode = '$LMODE';"

CMDS=$(print $CMDS dotty.mlevel = $MLEVEL";")

if [[ $loadfile != '' ]] then
    CMDS=$(print $CMDS load \("'"$loadfile"'"\)";")
fi

if [[ $FILES = '' ]] then
    FILES=null
fi
FUNC="dotty.createviewandgraph"
for i in $FILES; do
    CMDS=$(print $CMDS $FUNC \($i, "'"file"'", null, null\)";")
done

leftypath=$(whence -p lefty)
if [[ $leftypath == '' ]] then
    print -u2 "lneato: cannot locate the lefty program"
    print -u2 "       make sure that your path includes"
    print -u2 "       the directory containing lneato and lefty"
    exit 1
fi

$leftypath $FLAGS -e "
load ('dotty.lefty');
dotty.protogt.lserver = 'neato';
checkpath = function () {
    if (tablesize (dotty) > 0)
        remove ('checkpath');
    else {
        echo ('lneato: cannot locate the dotty scripts');
        echo ('       make sure that the environment variable LEFTYPATH');
        echo ('       is set to the directory containing dotty.lefty');
        exit ();
    }
};
checkpath ();
dotty.init ();
monitorfile = dotty.monitorfile;
$CMDS
txtview ('off');
"
