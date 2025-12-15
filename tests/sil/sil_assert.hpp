#pragma once

#include <cstdlib>
#include <iostream>

#define SIL_FAIL(msg)                                                     \
    do {                                                                  \
        std::cerr << "[SIL][FAIL] " << msg << " (" << __FILE__ << ":"     \
                  << __LINE__ << ")\n";                                   \
        std::exit(1);                                                     \
    } while (0)

#define SIL_ASSERT_TRUE(cond, msg)                                        \
    do {                                                                  \
        if (!(cond)) {                                                    \
            SIL_FAIL(msg);                                                \
        }                                                                 \
    } while (0)

#define SIL_ASSERT_FALSE(cond, msg)                                       \
    do {                                                                  \
        if ((cond)) {                                                     \
            SIL_FAIL(msg);                                                \
        }                                                                 \
    } while (0)
