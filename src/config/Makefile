
objs-y += parser.tab.o

clean-list-y += parser.tab.c
clean-list-y += parser.tab.h

# Add extra dependency information
#
# This forces make to generate the necessary parser headers before attempting
# to create the dependecy files.

$(tree)/parser.tab.c: $(tree)/parser.y
$(tree)/parser.tab.h: $(tree)/parser.y

$(tree)/.parse_file.d: $(tree)/parser.tab.h
$(tree)/.lexer.d: $(tree)/parser.tab.h

objs-y += lexer.o
clean-list-y += lexer.c

objs-y += parse_file.o

# Tell make not to delete the temporary parser files.
.SECONDARY: $(patsubst %,$(tree)/%,$(clean-list-y))

