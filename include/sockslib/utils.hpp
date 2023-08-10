#pragma once
#include <string>

#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

namespace sockslib {
#ifdef PLATFORM_WINDOWS
    using SocketHandle = SOCKET;

    constexpr auto handle_valid(SocketHandle handle) noexcept -> bool {
        return handle != INVALID_SOCKET;
    }
#else
    using SocketHandle = int;
    constexpr auto handle_valid(SocketHandle handle) -> bool {
        return handle > 0;
    }
#endif

    [[nodiscard]] auto get_last_error() -> std::string;
}