# Compiler settings
CC      ?= cc
CFLAGS  += -Wall -g -std=gnu99 \
		   -D_GNU_SOURCE -Wno-unused-result
LDFLAGS ?=
LEX     ?= flex
LFLAGS  ?=
YACC    ?= bison
YFLAGS  ?=
LD      ?= ld
PERL    ?= perl -w
MKDIR   ?= mkdir

# Install Paths
PREFIX  := /usr
BINDIR  := $(PREFIX)/bin
MANDIR  := $(PREFIX)/share/man
MAN1DIR := $(MANDIR)/man1
DOCDIR  := $(PREFIX)/share/doc/$(EXE)

# Show all commands executed by the Makefile
V ?= n

CONFIG_DEBUG ?= n

