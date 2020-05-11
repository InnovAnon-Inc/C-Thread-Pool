AC_INIT([libthpool], [0.0], [laurence.a.maddox@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AM_PROG_AR
LT_INIT([])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])

AC_ARG_ENABLE([use-submodules],
    AS_HELP_STRING([--enable-use-submodules], [Enable use submodules]))
AM_CONDITIONAL([USE_SUBMODULES], [test x"$enable_use_submodules" = "xyes"])

AC_CONFIG_MACRO_DIR([m4])
#AC_CONFIG_SUBDIRS([glitter])

AC_CHECK_HEADERS([errno.h signal.h stdio.h stdlib.h time.h unistd.h])
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([glitter.h])

AC_CHECK_LIB([pthread],[pthread_create,pthread_detach,pthread_kill])
AC_CHECK_LIB([pthread],[pthread_mutex_init,pthread_mutex_lock,pthread_mutex_unlock])
AC_CHECK_LIB([pthread],[pthread_cond_init,pthread_cond_wait,pthread_cond_signal,pthread_cond_broadcast])

INNOVANON_WARNINGS([])

AC_OUTPUT
