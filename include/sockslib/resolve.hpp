#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <WS2tcpip.h>
#include <Mstcpip.h>
#include <WinDNS.h>
#else
#include <sys/socket.h>
#endif

#include <kstd/result.hpp>
#include <string>
#include "sockslib/utils.hpp"

namespace sockslib {

    enum class AddressType {
        IPV4 = PF_INET,
        IPV6 = PF_INET6
    };

    [[nodiscard]] auto resolve_address(std::string domain) noexcept -> kstd::Result<std::string>;

    [[nodiscard]] auto address_type_enabled(AddressType type) noexcept -> kstd::Result<bool>;
}