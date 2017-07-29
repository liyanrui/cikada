AC_INIT(cikada, 0.1)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AM_GNU_GETTEXT_VERSION([0.19.7])
AM_GNU_GETTEXT([external])

AM_PROG_CC_C_O

PKG_CHECK_MODULES(CIKADA_DEPS, [clutter-1.0 poppler-glib])


AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile po/Makefile.in])

AC_OUTPUT
