#ifndef MARCO_H
#define MARCO_H

#include <qglobal.h>

#ifndef LIB_EXPORT
    #if defined MAKE_LIB
     #define LIB_EXPORT Q_DECL_EXPORT
    #else
     #define LIB_EXPORT Q_DECL_IMPORT
    #endif
#endif

#endif // MARCO_H
