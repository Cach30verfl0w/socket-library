#pragma once
#include <string>
#include <kstd/result.hpp>
#include <fmt/format.h>

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
}