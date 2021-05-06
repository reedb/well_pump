#
# Pila makefile
#

PILAVERSION = 20

CC       = gcc
# CFLAGS   = -O2 -Dunix -D__USE_GNU -pipe -Wall -I ./source/libiberty -I ./source
CFLAGS   = -g3 -Dunix -D__USE_GNU -Wall -I ./source/libiberty -I ./source
# LDFLAGS =
LDFLAGS  = -g3

LIBSRCS1  = source/libiberty/safe-ctype.c
LIBSRCS1 += source/libiberty/xmalloc.c
LIBSRCS1 += source/libiberty/xmemdup.c
LIBSRCS1 += source/libiberty/xstrdup.c
LIBSRCS1 += source/libiberty/xexit.c

LIBSRCS2  = source/libiberty/obstack.c
LIBSRCS2 += source/libiberty/hashtable.c

LIBSRCS3 += source/libiberty/hex.c
LIBSRCS3 += source/libiberty/xstrerror.c
LIBSRCS3 += source/libiberty/lbasename.c

PILASRCS  = source/assemble.c
PILASRCS += source/build.c
PILASRCS += source/codegen.c
PILASRCS += source/directiv.c
PILASRCS += source/error.c
PILASRCS += source/eval.c
PILASRCS += source/instlook.c
PILASRCS += source/insttabl.c
PILASRCS += source/parse.c
PILASRCS += source/listing.c
PILASRCS += source/movem.c
PILASRCS += source/object.c
PILASRCS += source/strcap.c
PILASRCS += source/opparse.c
PILASRCS += source/expand.c
PILASRCS += source/prc.c
PILASRCS += source/symbol.c
PILASRCS += source/guard.c
PILASRCS += source/main.c
PILASRCS += source/options.c
PILASRCS += $(LIBSRCS1)

ENCSRCS   = source/transform-sdk.c
ENCSRCS  += source/crc32.c
ENCSRCS  += $(LIBSRCS1)
ENCSRCS  += $(LIBSRCS2)

all: pila$(PILAVERSION) pila-sdk/transform-sdk
	@echo "done"

# the tool to transform *.inc into *.sdk files or vice versa
# with the help of the according *.h file.

pila-sdk/transform-sdk: $(ENCSRCS:.c=.o)
	$(CC) $(LDFLAGS) -o $(@) $+ $(LOADLIBES)

# the pila executable

pila$(PILAVERSION): $(PILASRCS:.c=.o)
	$(CC) $(LDFLAGS) -o $(@) $+ $(LOADLIBES)
# make sure main.c is recompiled the next time to update timestamp
	@touch source/main.c

# the rules to make *.o files out of *.c files

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $+
