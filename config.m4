dnl $Id$
dnl config.m4 for extension doctrine

PHP_ARG_ENABLE(doctrine, whether to enable doctrine support,
[  --enable-doctrine            Enable doctrine support])

if test "$PHP_DOCTRINE" != "no"; then
  PHP_NEW_EXTENSION(doctrine, doctrine.c, $ext_shared)
fi
