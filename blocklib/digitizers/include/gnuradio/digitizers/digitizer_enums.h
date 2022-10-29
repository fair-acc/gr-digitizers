#pragma once

#include "api.h"

namespace gr::digitizers {

/*!
 * \brief An enum representing acquisition mode
 * \ingroup digitizers
 */
enum class DIGITIZERS_API acquisition_mode_t {
    RAPID_BLOCK,
    STREAMING
};

/*!
 * \brief An enum representing coupling mode
 * \ingroup digitizers
 */
enum class DIGITIZERS_API coupling_t {
    DC_1M  = 0, /* DC, 1 MOhm */
    AC_1M  = 1, /* AC, 1 MOhm */
    DC_50R = 2, /* DC, 50 Ohm */
};

/*!
 * \brief Specifies a trigger mechanism
 * \ingroup digitizers
 */
enum class DIGITIZERS_API trigger_direction_t {
    RISING,
    FALLING,
    LOW,
    HIGH
};

/*!
 * \brief Downsampling mode
 * \ingroup digitizers
 */
enum class DIGITIZERS_API downsampling_mode_t {
    NONE,
    MIN_MAX_AGG,
    DECIMATE,
    AVERAGE,
};

/*!
 * \brief Error information.
 * \ingroup digitizers
 */
struct DIGITIZERS_API error_info_t {
    uint64_t        timestamp;
    std::error_code code;
};

} // namespace gr::digitizers
