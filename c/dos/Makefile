# This makefile is for building on DOS with DJGPP.
# It will require a lot of memory to build the interpreter.
# 256MB should be enough.
CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -g -I.
LDFLAGS = -lm

INTERP_SRCS = grace.c objects.c eval.c prelude.c convert.c gc.c vga.c
BAKED_SRCS = baked\ast.c baked\lexer.c baked\parser.c

SRCS = $(INTERP_SRCS) $(BAKED_SRCS) main.c

OBJS = $(SRCS:.c=.o)
INTERP_OBJS = $(INTERP_SRCS:.c=.o)

.PHONY: all clean

all: grace.exe

grace: grace.exe
	exe2coff grace.exe
program: program.exe
	exe2coff program.exe

graceraw.exe: grace
	copy /B C:\DJGPP\BIN\PMODSTUB.EXE + grace graceraw.exe

prograw.exe: program
	copy /B C:\DJGPP\BIN\PMODSTUB.EXE + program prograw.exe

libgrace.a: $(INTERP_OBJS)
	ar cr libgrace.a $^

coff: $(OBJS)
	$(CC) $(CFLAGS) -o grace $^ $(LDFLAGS)

grace.exe: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

program.exe: $(INTERP_OBJS) program.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

grace.o:        grace.c ast.h grace.h gc.h
objects.o:      objects.c ast.h grace.h gc.h
eval.o:         eval.c ast.h grace.h gc.h
prelude.o:      prelude.c ast.h grace.h gc.h
vga.o:          vga.c grace.h gc.h
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

mindir: ast.h grace.h gc.h libgrace.a minbuild.bat
	mkdir mindir
	mkdir mindir\sys
	copy C:\djgpp\bin\as.exe mindir
	copy C:\djgpp\libexec\gcc\djgpp\9\cc1.exe mindir
	copy C:\djgpp\bin\cwsdpmi.exe mindir
	copy C:\djgpp\bin\exe2coff.exe mindir
	copy C:\djgpp\bin\ld.exe mindir
	copy C:\djgpp\bin\pmodstub.exe mindir
	copy C:\djgpp\bin\stubify.exe mindir
	copy ast.h mindir
	copy grace.h mindir
	copy gc.h mindir
	copy C:\djgpp\include\ast.h mindir
	copy C:\djgpp\include\conio.h mindir
	copy C:\djgpp\include\dos.h mindir
	copy C:\djgpp\include\dpmi.h mindir
	copy C:\djgpp\include\go32.h mindir
	copy C:\djgpp\include\math.h mindir
	copy C:\djgpp\include\stdarg.h mindir
	copy C:\djgpp\include\stddef.h mindir
	copy C:\djgpp\include\stdint.h mindir
	copy C:\djgpp\include\stdio.h mindir
	copy C:\djgpp\include\stdlib.h mindir
	copy C:\djgpp\include\string.h mindir
	copy C:\djgpp\include\sys\djtypes.h mindir\sys
	copy C:\djgpp\include\sys\movedata.h mindir\sys
	copy C:\djgpp\include\sys\version.h mindir\sys
	copy C:\djgpp\lib\libc.a mindir
	copy C:\djgpp\lib\libgcc.a mindir
	copy C:\djgpp\lib\libm.a mindir
	copy C:\djgpp\lib\crt0.o mindir
	copy C:\djgpp\lib\crt0.o mindir
	copy C:\djgpp\lib\gcc\djgpp\9\libgcc.a mindir
	copy libgrace.a mindir
	copy minbuild.bat mindir\build.bat

clean:
	for %f in ($(INTERP_SRCS:.c=.o) main.o) do del %f
	for %f in ($(BAKED_SRCS:.c=.o)) do del %f
	for %f in (grace.exe graceraw.exe grace program.exe prograw.exe) do del %f
	del libgrace.a
