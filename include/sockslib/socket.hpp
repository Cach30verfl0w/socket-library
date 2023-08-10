#pragma once

#include <kstd/types.hpp>
#include <kstd/result.hpp>
#include <kstd/language.hpp>
#include <kstd/defaults.hpp>
#include <string>
#include "sockslib/utils.hpp"

#ifdef KSTD_CPP_20
#include <span>
#endif

#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include <WinSock2.h>
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

    constexpr SocketHandle invalid_socket_handle = -1;

    enum class ProtocolType : kstd::u8 {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    class Socket {
        protected:
        SocketHandle _socket_handle;// NOLINT
        public:
        static kstd::atomic_usize _socket_count;// NOLINT

        Socket();
        KSTD_DEFAULT_MOVE_COPY(Socket, Socket);
        virtual ~Socket() = default;
    };

    class AcceptedSocket final : Socket {
        public:
        AcceptedSocket(SocketHandle socket_handle);
        AcceptedSocket(const AcceptedSocket& other) = delete;
        AcceptedSocket(AcceptedSocket&& other) noexcept;
        ~AcceptedSocket() noexcept final;

        [[nodiscard]] auto write(void* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize>;
        [[nodiscard]] auto read(kstd::u8* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize>;
#ifdef KSTD_CPP_20
        [[nodiscard]] auto read(std::span<kstd::u8> data) const noexcept -> kstd::Result<kstd::usize>;
#endif

        auto operator=(const AcceptedSocket& other) -> AcceptedSocket& = delete;
        auto operator=(AcceptedSocket&& other) noexcept -> AcceptedSocket&;
    };

    // TODO: Support for UDP
    class ServerSocket final : Socket {
        ProtocolType _protocol_type;
#ifdef PLATFORM_WINDOWS
        PADDRINFOW _addr_info;
#endif
        public:
        ServerSocket(kstd::u16 port, ProtocolType protocol_type);
        ServerSocket(const ServerSocket& other) = delete;
        ServerSocket(ServerSocket&& other) noexcept;
        ~ServerSocket() noexcept final;

        [[nodiscard]] auto accept() const noexcept -> kstd::Result<AcceptedSocket>;

        [[nodiscard]] inline auto protocol_type() const noexcept -> ProtocolType {
            return _protocol_type;
        }

        [[nodiscard]] inline auto socket_handle() const noexcept -> SocketHandle {
            return _socket_handle;
        }

        auto operator=(const ServerSocket& other) -> ServerSocket& = delete;
        auto operator=(ServerSocket&& other) noexcept -> ServerSocket&;
    };

    class ClientSocket final : Socket {
        ProtocolType _protocol_type;

        public:
        ClientSocket(std::string address, kstd::u16 port, ProtocolType protocol_type);
        ClientSocket(const ClientSocket& other) = delete;
        ClientSocket(ClientSocket&& other) noexcept;
        ~ClientSocket() noexcept final;

        [[nodiscard]] inline auto protocol_type() const noexcept -> ProtocolType {
            return _protocol_type;
        }

        [[nodiscard]] inline auto socket_handle() const noexcept -> SocketHandle {
            return _socket_handle;
        }

        [[nodiscard]] auto write(void* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize>;
        [[nodiscard]] auto read(kstd::u8* data, kstd::usize size) const noexcept -> kstd::Result<kstd::usize>;
#ifdef KSTD_CPP_20
        [[nodiscard]] auto read(std::span<kstd::u8> data) const noexcept -> kstd::Result<kstd::usize>;
#endif

        auto operator=(const ClientSocket& other) -> ClientSocket& = delete;
        auto operator=(ClientSocket&& other) noexcept -> ClientSocket&;
    };
}// namespace sockslib