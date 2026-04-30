
all: lexer.c parser.c ast.c

.PHONY: all

%.c: ../../%.grace
	sed -ne '/_ast =/q;p' $@ > $@.tmp
	printf '    %s_ast = ' $* >> $@.tmp
	java -cp ../../cpsjava nz.mwh.cpsgrace.Start -P $^ >> $@.tmp
	sed -ne '1,/_ast =/d;p' $@ >> $@.tmp
	mv $@.tmp $@