AUTOMAKE_OPTIONS = foreign subdir-objects
CC = @CC@ -Wall -Wno-unused-function
CPP = @CPP@
AM_CFLAGS = -I$(top_srcdir)/include
LIBS = @LIBS@ 
DEFS =
if HAVE_803X_UPDATE
DEFS += -DUPDATE_TCC803X
endif

lib_LTLIBRARIES = libTcUpdateEngine.la
libTcUpdateEngine_la_SOURCES = $(top_srcdir)/src/ab-updater.c \
						$(top_srcdir)/src/boot-control.c \
						$(top_srcdir)/src/firmware-update.c \
						$(top_srcdir)/src/update-engine.c
if HAVE_803X_UPDATE
libTcUpdateEngine_la_SOURCES += $(top_srcdir)/src/803x_snor/snor-update.c
else
libTcUpdateEngine_la_SOURCES += $(top_srcdir)/src/805x_snor/snor-update.c
endif

libTcUpdateEngine_la_LDFLAGS = -version-info 1:1:0

include_HEADERS = $(top_srcdir)/include/update-engine-def.h \
				  $(top_srcdir)/include/update-engine.h

