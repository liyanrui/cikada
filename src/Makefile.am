bin_PROGRAMS = cikada
cikada_CPPFLAGS = $(CIKADA_DEPS_CFLAGS) -DLOCALEDIR=\"$(localedir)\" -std=c99
cikada_LDADD   = $(CIKADA_DEPS_LIBS) -lm
cikada_SOURCES = cikada.c \
		 ckd-meta-slides.c \
		 ckd-slide.c \
		 ckd-view.c \
		 ckd-script.c \
		 ckd-player.c \
		 ckd-magnifier.c
