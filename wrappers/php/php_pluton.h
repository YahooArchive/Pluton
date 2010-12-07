#ifndef PHP_PLUTON_H
#define PHP_PLUTON_H

#define PHP_PLUTON_VERSION  "1.1.0"

extern zend_module_entry pluton_module_entry;
#define phpext_pluton_ptr &pluton_module_entry

ZEND_BEGIN_MODULE_GLOBALS(pluton)
ZEND_END_MODULE_GLOBALS(pluton)

#ifdef ZTS
# define PLUTONG(v) TSRMG(pluton_globals_id, zend_pluton_globals *, v)
#else
# define PLUTONG(v) (pluton_globals.v)
#endif

#endif
