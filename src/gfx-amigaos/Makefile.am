AM_CPPFLAGS  = @UAE_CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/include -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CFLAGS    = @UAE_CFLAGS@

noinst_LIBRARIES = libgfxdep.a

libgfxdep_a_SOURCES = ami-win.c ami-appw.c comp.c window_icons.c video_convert.c video_palette.c

noinst_HEADERS = gfx.h
