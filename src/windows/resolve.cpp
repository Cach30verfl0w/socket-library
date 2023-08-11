#ifdef PLATFORM_WINDOWS
#include "sockslib/resolve.hpp"
#include "sockslib/socket.hpp"
#include <WS2tcpip.h>
#include <fmt/format.h>

namespace sockslib {

    auto resolve_address(const std::string domain, kstd::usize address_type) noexcept -> kstd::Result<std::string> {
        using namespace std::string_literals;

        // Startup WSA and increment socket count
        auto wsa_init_result = init_wsa();
        if (!wsa_init_result) {
            return kstd::Error {wsa_init_result.get_error()};
        }

        // Run query and store everything in record pointer
        PDNS_RECORD record = nullptr;
        if(FAILED(DnsQuery(domain.data(), static_cast<WORD>(address_type), DNS_QUERY_STANDARD | DNS_QUERY_BYPASS_CACHE,
                           nullptr, &record, nullptr))) {
            // Cleanup WSA and decrement socket count
            cleanup_wsa();
            return kstd::Error {fmt::format("Unable to resolve address over DNS => {}", get_last_error())};
        }

        std::string address;
        std::array<wchar_t, 128> address_buffer {};
        DWORD size = address_buffer.size();
        switch(record->wType) {
            case DNS_TYPE_A: {
                SOCKADDR_IN addr {};
                addr.sin_family = AF_INET;
                addr.sin_addr = *reinterpret_cast<in_addr*>(&record->Data.A.IpAddress);// NOLINT

                if(FAILED(WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&addr), sizeof(addr), nullptr,// NOLINT
                                              address_buffer.data(), &size))) {
                    return kstd::Error {fmt::format("Unable to format IPv4 Address into literal => {}", get_last_error())};
                }
                break;
            }
            case DNS_TYPE_AAAA: {
                SOCKADDR_IN6 addr {};
                memset(&addr, 0, sizeof(addr));
                addr.sin6_family = AF_INET6;
                addr.sin6_addr = *reinterpret_cast<in_addr6*>(&record->Data.AAAA.Ip6Address);// NOLINT

                if(FAILED(WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&addr), sizeof(addr), nullptr,// NOLINT
                                             address_buffer.data(), &size))) {
                    return kstd::Error {fmt::format("Unable to format IPv4 Address into literal => {}", get_last_error())};
                }
                break;
            }
            default: return kstd::Error {"Unable to cleanup WSA => Illegal type of record in response"s};
        }

        // Cleanup WSA and decrement socket count
        auto wsa_cleanup_result = cleanup_wsa();
        if (!wsa_cleanup_result) {
            return kstd::Error {wsa_cleanup_result.get_error()};
        }

        // Free DNS record and return address
        DnsRecordListFree(record, DnsFreeRecordListDeep);
        return {std::string {address_buffer.begin(), address_buffer.begin() + size - 1}};
    }

    auto resolve_address(const std::string domain) noexcept -> kstd::Result<std::string> {
        // Resolve IPv6 address if IPv6 enabled
        const auto ipv6_supported = address_type_enabled(AddressType::IPV6);
        if (!ipv6_supported) {
            return kstd::Error {ipv6_supported.get_error()};
        }

        if (ipv6_supported.get()) {
            auto ipv6_resolve_result = resolve_address(domain, DNS_TYPE_AAAA);
            if(ipv6_resolve_result) {
                return ipv6_resolve_result.get();
            }
        }

        // Resolve IPv4 address if IPv4 enabled
        const auto ipv4_supported = address_type_enabled(AddressType::IPV4);
        if (!ipv4_supported) {
            return kstd::Error {ipv4_supported.get_error()};
        }

        if (ipv4_supported.get()) {
            const auto ipv4_resolve_result = resolve_address(domain, DNS_TYPE_A);
            if(ipv4_resolve_result) {
                return ipv4_resolve_result.get();
            }

            return kstd::Error {ipv4_resolve_result.get_error()};
        }

        using namespace std::string_literals;
        return kstd::Error { "Unable to resolve address => IPv4 and IPv6 are not enabled"s };
    }

    auto address_type_enabled(AddressType type) noexcept -> kstd::Result<bool> {
        // Startup WSA and increment socket count
        auto wsa_init_result = init_wsa();
        if (!wsa_init_result) {
            return kstd::Error {wsa_init_result.get_error()};
        }

        SocketHandle handle = socket(static_cast<int>(type), SOCK_DGRAM, 0);
        bool is_valid = handle_valid(handle);
        closesocket(handle);

        // Cleanup WSA and decrement socket count
        auto wsa_cleanup_result = cleanup_wsa();
        if (!wsa_cleanup_result) {
            return kstd::Error {wsa_cleanup_result.get_error()};
        }
        return is_valid;
    }
}// namespace sockslib
#endif