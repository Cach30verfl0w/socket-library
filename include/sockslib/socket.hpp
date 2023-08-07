#pragma once

#include <kstd/types.hpp>
#include <kstd/result.hpp>
#include <string>

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

    enum class ProtocolType : kstd::u8 {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    /*enum class CallbackType {
        SOCKET_CONNECTED,
        SOCKET_DISCONNECTED,
        DATA_RECEIVED,
        DATA_SENT
    };*/

    class Socket {
        protected:
        SocketHandle _socket_handle;// NOLINT
        ProtocolType _protocol_type;// NOLINT
        kstd::u16 _buffer_size;// NOLINT
        static kstd::atomic_usize _socket_count;// NOLINT

        Socket(ProtocolType protocol_type, kstd::u16 buffer_size);
        Socket() noexcept = default;
    };

    class ServerSocket final : Socket {// NOLINT
#ifdef PLATFORM_WINDOWS
        PADDRINFOW _addr_info;
#endif
        public:
        ServerSocket(kstd::u16 port, ProtocolType protocol_type, kstd::u16 buffer_size);
        ServerSocket(const ServerSocket& other) = delete;
        ServerSocket(ServerSocket&& other) noexcept;
        ~ServerSocket() noexcept;

        /*template<CallbackType TYPE, typename F>
        auto register_callback(F function) noexcept {
            // TODO: Write function lol
            if constexpr(TYPE == CallbackType::SOCKET_CONNECTED) {
                static_assert(std::is_invocable_v<F, Socket>, "Illegal function signature => Please use void()");
            } else if constexpr(TYPE == CallbackType::SOCKET_DISCONNECTED) {
                static_assert(std::is_invocable_v<F, Socket>, "Illegal function signature => Please use void()");
            }
        }*/

        auto operator=(const ServerSocket& other) -> ServerSocket& = delete;
        auto operator=(ServerSocket&& other) noexcept -> ServerSocket&;
    };

    class ClientSocket final : Socket {// NOLINT
        public:
        ClientSocket(std::string address, kstd::u16 port, ProtocolType protocol_type, kstd::u16 buffer_size);
        ClientSocket(const ClientSocket& other) = delete;
        ClientSocket(ClientSocket&& other) noexcept;
        ~ClientSocket() noexcept;

        /*template<CallbackType TYPE, typename F>
        auto register_callback(F function) noexcept {
            // TODO: Write function lol
            if constexpr(TYPE == CallbackType::SOCKET_CONNECTED) {
                static_assert(std::is_invocable_v<F>, "Illegal function signature => Please use void()");
            } else if constexpr(TYPE == CallbackType::SOCKET_DISCONNECTED) {
                static_assert(std::is_invocable_v<F>, "Illegal function signature => Please use void()");
            }
        }*/

        auto operator=(const ClientSocket& other) -> ClientSocket& = delete;
        auto operator=(ClientSocket&& other) noexcept -> ClientSocket&;
    };
}// namespace sockslib