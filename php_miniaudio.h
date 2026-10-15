#ifndef PHP_MINIAUDIO_H
#define PHP_MINIAUDIO_H

#define PHP_MINIAUDIO_EXTNAME "miniaudio"
#define PHP_MINIAUDIO_VERSION "0.1.0"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

extern zend_module_entry miniaudio_module_entry;
#define phpext_miniaudio_ptr &miniaudio_module_entry

#endif