#pragma once

#include <gnuradio/attributes.h>

#ifdef gnuradio_digitizers_EXPORTS
#define DIGITIZERS_API __GR_ATTR_EXPORT
#else
#define DIGITIZERS_API __GR_ATTR_IMPORT
#endif
