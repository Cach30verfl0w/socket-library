#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <WS2tcpip.h>
#include <Mstcpip.h>
#include <WinDNS.h>
#else
#include <sys/socket.h>
#endif

#include <kstd/result.hpp>
#include <kstd/option.hpp>
#include <kstd/tuple.hpp>
#include <string>
#include <iostream>
#include "sockslib/utils.hpp"

namespace sockslib {

    enum class AddressType {
        IPV4 = PF_INET,
        IPV6 = PF_INET6
    };

    using AddressLiteralTypePair = kstd::Tuple<std::string&, AddressType>;

    [[nodiscard]] auto resolve_address(std::string domain) noexcept -> kstd::Result<std::string>;

    [[nodiscard]] auto address_type_enabled(AddressType type) noexcept -> kstd::Result<bool>;

    [[nodiscard]] inline auto recognize_address_type(std::string& address) noexcept -> kstd::Option<AddressType> {
        if (is_ipv4_address(address)) {
            return {AddressType::IPV4};
        }

        if (is_ipv6_address(address)) {
            return {AddressType::IPV6};
        }
        return {};
    }
}