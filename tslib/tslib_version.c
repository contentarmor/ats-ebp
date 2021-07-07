#include "tslib_version.h"

#ifndef TSLIB_VERSION
#define TSLIB_VERSION "1.0.2"
#endif

const char* get_tslib_version()
{
    return TSLIB_VERSION " (build: " __DATE__ " "__TIME__")";
}
