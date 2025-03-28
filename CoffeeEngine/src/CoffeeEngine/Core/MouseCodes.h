/**
 * @defgroup core Core
 * @brief Core components of the CoffeeEngine.
 * @{
 */

#pragma once

#include <cstdint>

namespace Coffee
{
    using MouseCode = uint16_t;

    namespace Mouse
    {
        /**
         * @enum Mouse
         * @brief Enumeration of mouse button codes.
         *
         * These values are from SDL\_mouse.h.
         */
        enum : MouseCode
        {
            Invalid = 0,
            ButtonLeft =    1, ///< Left mouse button
            ButtonMiddle =  2, ///< Middle mouse button
            ButtonRight =   3, ///< Right mouse button
            ButtonX1  =     4, ///< Extra mouse button 1
            ButtonX2  =     5  ///< Extra mouse button 2
        };
    }
}

/** @} */