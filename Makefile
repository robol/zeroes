# Configure the libraries to be used

# Flags to use GSL
# This seems to work on OSX
LIBS=-lgsl -lblas -lm
# And this works on Linux
# LIBS=-lgsl -L/usr/lib/openblas-base/ -lopenblas

# Flags that shall be used for MPSolve
# CFLAGS=-I../mpsolve/include -I../mpsolve/_build/include -DUSE_MPSOLVE
# LIBS=-L../mpsolve/_build/src/libmps/.libs -lmps -lm

zeroes: zeroes.c
	gcc $(CFLAGS) -std=c99 -O3 -o zeroes zeroes.c $(LIBS)

clean:
	/bin/rm -f zeroes *.pyc
