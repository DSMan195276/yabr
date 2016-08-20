# Program wide settings
EXE       := yabr
EXEC      := YABR
YABR_VERSION   := 0
YABR_SUBLEVEL  := 1
YABR_PATCH     := 0
YABR_VERSION_N := $(YABR_VERSION).$(YABR_SUBLEVEL).$(YABR_PATCH)

YABR_LIBFLAGS := `pkg-config --libs i3ipc-glib-1.0` \
					-lasound \
					`pkg-config --libs libmpdclient`
YABR_CFLAGS  += -I'./include' `pkg-config --cflags i3ipc-glib-1.0` \
				`pkg-config --cflags libmpdclient` \
				-DYABR_VERSION_N="$(YABR_VERSION_N)"

YABR_OBJS += ./yabr.o

