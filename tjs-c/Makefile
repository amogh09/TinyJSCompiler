CC = gcc
INCLUDE_PATH = /usr/local/include
# CFLAGS = -O2 -I${INCLUDE_PATH} -DXP_UNIX
CFLAGS = -Wall -O2 -I${INCLUDE_PATH} -DXP_UNIX
# LIBJS_PATH = /usr/local/lib64
LIBJS_PATH = /usr/local/lib
LDFLAGS = -L${LIBJS_PATH} ${LFLAGS} -lm -ljs

tjscompiler: tjscompiler.o
	gcc -o $@ tjscompiler.o ${LDFLAGS}

clean:
	-rm -f tjscompiler.o *~ tjscompiler
