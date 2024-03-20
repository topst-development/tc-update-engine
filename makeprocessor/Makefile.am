AUTOMAKE_OPTIONS = foreign subdir-objects
CC = @CC@ -Wall -Wno-unused-function
CPP = @CPP@
AM_CFLAGS = -I$(top_srcdir)/include
LIBS = @LIBS@ 
DEFS =
if HAVE_803X_UPDATE
DEFS += -DUPDATE_TCC803X
endif

bin_PROGRAMS = TcUpdateEngine

TcUpdateEngine_SOURCES = $(top_srcdir)/src/ab-updater.c\
						 $(top_srcdir)/src/boot-control.c \
						 $(top_srcdir)/src/firmware-update.c \
						 $(top_srcdir)/src/update-engine.c \
						 $(top_srcdir)/src/main.c
if HAVE_803X_UPDATE
TcUpdateEngine_SOURCES += $(top_srcdir)/src/803x_snor/snor-update.c
else
TcUpdateEngine_SOURCES += $(top_srcdir)/src/805x_snor/snor-update.c
endif

