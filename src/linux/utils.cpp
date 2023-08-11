#ifdef PLATFORM_LINUX
#include <errno.h>
#include "sockslib/utils.hpp"

namespace sockslib {
    auto get_last_error() noexcept -> std::string {
        return fmt::format("ERROR 0x{:X}: {}", errno, strerror(errno));
    }
}
#endif