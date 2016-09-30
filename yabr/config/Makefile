
objs-y += parser.tab.o

clean-list-y += parser.tab.c
clean-list-y += parser.tab.h

$(objtree)/parser.tab.c: $(srctree)/parser.y
$(objtree)/parser.tab.h: $(srctree)/parser.y

$(objtree)/.parse_file.d: $(srctree)/parser.tab.h
$(objtree)/.lexer.d: $(srctree)/parser.tab.h

objs-y += lexer.o
clean-list-y += lexer.c

objs-y += parse_file.o

.SECONDARY: $(patsubst %,$(objtree)/%,$(clean-list-y))

