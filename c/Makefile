CC      = cc
CFLAGS  = -std=gnu11 -Wall -Wextra -g -I.
LDFLAGS = -lm

INTERP_SRCS = grace.c objects.c eval.c prelude.c convert.c gc.c
BAKED_SRCS = baked/ast.c baked/lexer.c baked/parser.c

SRCS = $(INTERP_SRCS) $(BAKED_SRCS) main.c

OBJS = $(SRCS:.c=.o)
INTERP_OBJS = $(INTERP_SRCS:.c=.o)

.PHONY: all clean

all: grace

grace: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

program: $(INTERP_OBJS) program.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

grace.o:        grace.c ast.h grace.h gc.h
objects.o:      objects.c ast.h grace.h gc.h
eval.o:         eval.c ast.h grace.h gc.h
prelude.o:      prelude.c ast.h grace.h gc.h
convert.o:      convert.c ast.h grace.h
main.o:         main.c ast.h grace.h gc.h
gc.o:           gc.c gc.h grace.h

program.o:		program.c ast.h grace.h

baked/ast.o:    baked/ast.c ast.h
baked/lexer.o:  baked/lexer.c ast.h
baked/parser.o: baked/parser.c ast.h

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

baked/%.o: baked/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

onefile.c: $(SRCS)
	cat ast.h grace.h gc.h convert.c eval.c grace.c gc.c objects.c prelude.c baked/*.c main.c > bigfile.c
	sed -i.bak -e '/#include "/d' bigfile.c
	rm bigfile.c.bak

onefile: onefile.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

onefile-template.c: $(INTERP_SRCS) program.c
	cat ast.h grace.h gc.h convert.c eval.c grace.c gc.c objects.c prelude.c > $@
	sed -i.bak -e '/#include "/d' $@
	sed -e 's/program_ast = .*/program_ast = o0C(nil, nil);' program.c >> $@
	rm $@.bak

onefile-program.c: $(INTERP_SRCS) program.c
	cat ast.h grace.h gc.h convert.c eval.c grace.c gc.c objects.c prelude.c program.c > $@
	sed -i.bak -e '/#include "/d' $@
	rm $@.bak

onefile-program: onefile-program.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(OBJS) grace program
