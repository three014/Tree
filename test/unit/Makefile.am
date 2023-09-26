TESTS = check_bamboo check_hashmap
check_PROGRAMS = check_bamboo check_hashmap

check_hashmap_SOURCES = check_hashmap.c $(top_builddir)/include/hashmap.h
check_hashmap_CFLAGS = @CHECK_CFLAGS@
check_hashmap_LDADD = $(top_builddir)/src/libbamboo.la @CHECK_LIBS@

check_bamboo_SOURCES = check_bamboo.c $(top_builddir)/include/hashmap.h $(top_builddir)/include/arena.h
check_bamboo_CFLAGS = @CHECK_CFLAGS@
check_bamboo_LDADD = $(top_builddir)/src/libbamboo.la @CHECK_LIBS@

