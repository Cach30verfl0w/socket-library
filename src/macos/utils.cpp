#ifdef PLATFORM_APPLE
#include <fmt/format.h>
#include <errno.h>
#include "sockslib/utils.hpp"

namespace sockslib {
    [[nodiscard]] auto get_last_error() -> std::string {
        return fmt::format("ERROR 0x{:X}: {}", errno, strerror(errno));
    }
}
#endif