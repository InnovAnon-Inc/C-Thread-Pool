AC_INIT([libthpool], [0.0], [laurence.a.maddox@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AM_PROG_AR
LT_INIT([])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_CONFIG_MACRO_DIR([m4])
AC_OUTPUT

AC_CHECK_LIB([pthread],[pthread_create,pthread_detach,pthread_kill])
AC_CHECK_LIB([pthread],[pthread_mutex_init,pthread_mutex_lock,pthread_mutex_unlock])
AC_CHECK_LIB([pthread],[pthread_cond_init,pthread_cond_wait,pthread_cond_signal,pthread_cond_broadcast])