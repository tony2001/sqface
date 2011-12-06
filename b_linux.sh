CXX=g++
NAME=sqface
OPTS="-O3 -Wall"
LIBS="-lfreeimage -lm"
$CXX $OPTS -o $NAME ${NAME}.cpp $LIBS
