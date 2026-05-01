REM This is the build script for a minimal program in a minimal environment,
REM and not for normal usage. It is copied into the minimal environment by
REM `make mindir`.
cc1 -I . program.c
as -o program.o program.s
ld -L . program.o libgrace.a crt0.o -lc -lm -lgcc
stubify -v a.out
exe2coff a.exe
copy /B pmodstub.exe + a program.exe

del a
del a.exe
del a.out
del program.s
del program.o
