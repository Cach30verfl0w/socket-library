#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <WS2tcpip.h>
#include <Mstcpip.h>
#include <WinDNS.h>
#endif

#include <kstd/result.hpp>
#include <kstd/types.hpp>
#include <string>
#include "sockslib/utils.hpp"

namespace sockslib {
    [[nodiscard]] auto resolve_address(std::string domain, kstd::usize timeout) -> kstd::Result<std::string>;
}