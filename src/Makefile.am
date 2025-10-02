bin_PROGRAMS = ../functy
___functy_SOURCES = function.c function.h function_private.h functy.c functy.h textures.c textures.h utils.c utils.h vis.c vis.h filesave.c filesave.h cartesian.c cartesian.h spherical.c spherical.h shader.c shader.h curve.c curve.h vecsym.c vecsym.h exportply.c exportply.h exportstl.c exportstl.h exportbitmap.c exportbitmap.h exportsvx.c exportsvx.h recall.c recall.h controlvar.c controlvar.h shadow.c shadow.h longpoll.c longpoll.h exportvdb.c exportvdb.h ovdbc.cpp ovdbc.h audio.c audio.h global.c global.h

___functy_LDADD = -lm @FUNCTY_LIBS@ @SYMBOLIC_LIBS@ @OPENVDB_LIBS@

___functy_CPPFLAGS = -DFUNCTYDIR=\"$(datadir)/@PACKAGE@\" -Wall @FUNCTY_CFLAGS@ @SYMBOLIC_CFLAGS@ @OPENVDB_CFLAGS@

#dist_pkgdata_DATA = ../ui/functy.ui

