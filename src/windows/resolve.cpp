#ifdef PLATFORM_WINDOWS
#include "sockslib/resolve.hpp"
#include "sockslib/socket.hpp"
#include <WS2tcpip.h>
#include <fmt/format.h>

namespace sockslib {

    auto resolve_address(const std::string domain, kstd::usize address_type) noexcept -> kstd::Result<std::string> {
        using namespace std::string_literals;

        // Startup WSA and increment socket count
        if(Socket::_socket_count == 0) {
            WSADATA wsaData {};
            if(FAILED(WSAStartup(MAKEWORD(2, 2), &wsaData))) {
                return kstd::Error {fmt::format("Unable to initialize WSA => {}", get_last_error())};
            }
        }
        Socket::_socket_count++;

        // Run query and store everything in record pointer
        PDNS_RECORD record = nullptr;
        if(FAILED(DnsQuery(domain.data(), static_cast<WORD>(address_type), DNS_QUERY_STANDARD | DNS_QUERY_BYPASS_CACHE,
                           nullptr, &record, nullptr))) {
            // Cleanup WSA and decrement socket count
            if(Socket::_socket_count == 1) {
                if(FAILED(WSACleanup())) {
                    return kstd::Error {fmt::format("Unable to cleanup WSA => {}", get_last_error())};
                }
            }
            Socket::_socket_count--;
            return kstd::Error {fmt::format("Unable to resolve address over DNS => {}", get_last_error())};
        }

        std::string address;
        switch(record->wType) {
            case DNS_TYPE_A: {
                IN_ADDR ip_addr;
                ip_addr.S_un.S_addr = record->Data.A.IpAddress;// NOLINT
                DnsRecordListFree(record, DnsFreeRecordListDeep);
                address = inet_ntoa(ip_addr);
                break;
            }
            case DNS_TYPE_AAAA: {
                SOCKADDR_IN6 addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin6_family = AF_INET6;
                addr.sin6_addr = *reinterpret_cast<in_addr6*>(&(record->Data.AAAA.Ip6Address));// NOLINT

                std::array<wchar_t, 46> address_buffer {};
                DWORD size = address_buffer.size();
                if(FAILED(WSAAddressToStringW(reinterpret_cast<LPSOCKADDR>(&addr), sizeof(addr), nullptr,// NOLINT
                                             address_buffer.data(), &size))) {
                    return kstd::Error {fmt::format("Unable to format IPv4 Address into literal => {}", get_last_error())};
                }
                address = std::string {address_buffer.begin(), address_buffer.begin() + size - 1};
                break;
            }
            default: return kstd::Error {"Unable to cleanup WSA => Illegal type of record in response"s};
        }

        // Cleanup WSA and decrement socket count
        if(Socket::_socket_count == 1) {
            if(FAILED(WSACleanup())) {
                return kstd::Error {fmt::format("Unable to cleanup WSA => {}", get_last_error())};
            }
        }
        Socket::_socket_count--;
        return {address};
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
        if(Socket::_socket_count == 0) {
            WSADATA wsaData {};
            if(FAILED(WSAStartup(MAKEWORD(2, 2), &wsaData))) {
                return kstd::Error {fmt::format("Unable to initialize WSA => {}", get_last_error())};
            }
        }
        Socket::_socket_count++;

        SocketHandle handle = socket(static_cast<int>(type), SOCK_DGRAM, 0);
        bool is_valid = handle_valid(handle);
        closesocket(handle);

        // Cleanup WSA and decrement socket count
        if(Socket::_socket_count == 1) {
            if(FAILED(WSACleanup())) {
                return kstd::Error {fmt::format("Unable to cleanup WSA => {}", get_last_error())};
            }
        }
        Socket::_socket_count--;
        return is_valid;
    }
}// namespace sockslib
#endif