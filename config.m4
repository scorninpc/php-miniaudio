PHP_ARG_ENABLE(miniaudio, whether to enable miniaudio support,
[  --enable-miniaudio           Enable miniaudio support])

if test "$PHP_MINIAUDIO" = "yes"; then
	AC_DEFINE(HAVE_MINIAUDIO, 1, [Have miniaudio support])

  PHP_ADD_INCLUDE($ext_srcdir)

  PHP_CHECK_LIBRARY(pthread, pthread_create, [
		PHP_ADD_LIBRARY(pthread, 1, MINIAUDIO_SHARED_LIBADD)
	],[
		AC_MSG_ERROR([libpthread missing])
	])

  PHP_ADD_LIBRARY(m, 1, MINIAUDIO_SHARED_LIBADD)

  case $host_os in
		*linux*)
			PHP_ADD_LIBRARY(dl, 1, MINIAUDIO_SHARED_LIBADD)
			;;
	esac

  PHP_NEW_EXTENSION(miniaudio, miniaudio.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
	PHP_SUBST(MINIAUDIO_SHARED_LIBADD)
fi