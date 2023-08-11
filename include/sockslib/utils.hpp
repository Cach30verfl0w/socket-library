#pragma once
#include <string>
#include <kstd/result.hpp>
#include <fmt/format.h>
#include <regex>

#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

namespace sockslib {
    [[nodiscard]] auto get_last_error() noexcept -> std::string;

#ifdef PLATFORM_WINDOWS
    using SocketHandle = SOCKET;
    static kstd::atomic_usize _wsa_user_count = 0;// NOLINT

    constexpr auto handle_valid(SocketHandle handle) noexcept -> bool {
        return handle != INVALID_SOCKET;
    }

    [[nodiscard]] inline auto init_wsa() noexcept -> kstd::Result<void> {
        if(_wsa_user_count == 0) {
            WSADATA wsaData {};
            if(FAILED(WSAStartup(MAKEWORD(2, 2), &wsaData))) {
                return kstd::Error {fmt::format("Unable to initialize Socket => {}", get_last_error())};
            }
        }
        ++_wsa_user_count;
        return {};
    }

    inline auto cleanup_wsa() noexcept -> kstd::Result<void> {
        if(_wsa_user_count == 1) {
            if (FAILED(WSACleanup())) {
                return kstd::Error {fmt::format("Unable to cleanup Socket => {}", get_last_error())};
            }
        }

        if (_wsa_user_count > 0) {
            --_wsa_user_count;
        }

        return {};
    }
#else
    using SocketHandle = int;
    constexpr auto handle_valid(SocketHandle handle) -> bool {
        return handle > 0;
    }
#endif

    inline auto is_domain(const std::string& address) noexcept -> bool {
        return std::regex_match(address, std::regex(R"(\b((?=[a-z0-9-]{1,63}\.)[a-z0-9]+(-[a-z0-9]+)*\.)+[a-z]{2,63}\b)"));
    }

    inline auto is_ipv4_address(const std::string& address) noexcept -> bool {
        return std::regex_match(address, std::regex(R"(([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3}))"));
    }

    inline auto is_ipv6_address(const std::string& address) noexcept -> bool {
        return std::regex_match(address, std::regex("(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))"));
    }
}