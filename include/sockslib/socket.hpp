#pragma once

#include <kstd/option.hpp>
#include <kstd/result.hpp>
#include <kstd/types.hpp>

#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

namespace sockslib {
#ifdef PLATFORM_WINDOWS
    using SocketHandle = SOCKET;

    constexpr auto handle_invalid(SocketHandle handle) noexcept -> bool {
        return handle == INVALID_SOCKET;
    }

    constexpr auto handle_valid(SocketHandle handle) noexcept -> bool {
        return handle != INVALID_SOCKET;
    }
#else
    using SocketHandle = int;

    constexpr auto handle_invalid(SocketHandle handle) -> bool {
        return handle <= 0;
    }

    constexpr auto handle_valid(SocketHandle handle) -> bool {
        return handle > 0;
    }
#endif

    [[nodiscard]] auto get_last_error() -> std::string;

    enum class SocketType : kstd::u8 {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    class Socket final {
        SocketHandle _socket_handle;
        kstd::u16 _buffer_size;
        kstd::Option<kstd::u16> _port;
        SocketType _type;
#ifdef PLATFORM_WINDOWS
        PADDRINFOW _addr_info;
        static kstd::atomic_usize _socket_count; // NOLINT
#endif

        public:
        Socket(kstd::Option<kstd::u16> port, SocketType type, kstd::u16 buffer_size);
        Socket(const Socket& other) = delete;
        Socket(Socket&& other) noexcept;
        ~Socket() noexcept;

        auto bind() -> kstd::Result<void>;

        auto operator=(const Socket& other) -> Socket& = delete;
        auto operator=(Socket&& other) noexcept -> Socket&;
    };
}