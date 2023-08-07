#include "sockslib/socket.hpp"

#include <fmt/format.h>
#include <kstd/utils.hpp>
#include <stdexcept>

#ifdef PLATFORM_WINDOWS
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace sockslib {
#ifdef PLATFORM_WINDOWS
    kstd::atomic_usize Socket::_socket_count = 0;//NOLINT
#endif

    [[nodiscard]] auto get_last_error() -> std::string {
#ifdef PLATFORM_WINDOWS
        const auto error_code = ::GetLastError();

        if(error_code == 0) {
            return "No error found!";
        }

        LPWSTR buffer = nullptr;
        constexpr auto lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        const auto new_length = ::FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                error_code, lang_id, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
        auto message = kstd::utils::to_mbs({buffer, new_length});
        LocalFree(buffer);

        return fmt::format("ERROR 0x{:X}: {}", error_code, message);
#else
        return fmt::format("ERROR 0x{:X}: {}", errno, strerror(errno));
#endif
    }

    Socket::Socket(kstd::u16 port, SocketType type, kstd::u16 buffer_size) :// NOLINT
            _buffer_size {buffer_size},
            _port {port},
            _type {type} {
        initialize(true).throw_if_error();
        bind().throw_if_error();
    }

    Socket::Socket(sockslib::Socket&& other) noexcept :
            _socket_handle {other._socket_handle},
            _buffer_size {other._buffer_size},
            _port {other._port},
#ifdef PLATFORM_WINDOWS
            _addr_info {other._addr_info},
#endif
            _type {other._type} {
        other._socket_handle = 0;
#ifdef PLATFORM_WINDOWS
        other._addr_info = nullptr;
        ++_socket_count;
#endif
    }

    Socket::~Socket() noexcept {
#ifdef PLATFORM_WINDOWS
        // Free the address info if valid
        if(_addr_info != nullptr) {
            FreeAddrInfoW(_addr_info);
        }

        // Free the address info and close the socket if the socket handle is valid
        if(handle_valid(_socket_handle)) {
            closesocket(_socket_handle);
        }

        // Cleanup WSA
        if(_socket_count == 1) {
            WSACleanup();
        }

        // Decrement socket count
        if(_socket_count > 0) {
            --_socket_count;
        }

#else
        // Close the socket if the socket handle is valid
        if(handle_valid(_socket_handle)) {
            if(_port.has_value()) {
                shutdown(_socket_handle, SHUT_RDWR);
            }
            else {
                close(_socket_handle);
            }
        }
#endif
    }

    [[nodiscard]] auto Socket::initialize(bool server) -> kstd::Result<void> {
#if defined(PLATFORM_WINDOWS)
        if(_socket_count == 0) {
            WSADATA wsaData {};
            if(FAILED(WSAStartup(MAKEWORD(2, 2), &wsaData))) {
                return kstd::Error { fmt::format("Unable to initialize WSA => {}", get_last_error()) };
            }
            ++_socket_count;
        }

        ADDRINFOW hints {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = static_cast<int>(_type);
        switch(_type) {
            case SocketType::TCP: hints.ai_protocol = IPPROTO_TCP; break;
            case SocketType::UDP: hints.ai_protocol = IPPROTO_UDP; break;
        }
        if (server) {
            hints.ai_flags = AI_PASSIVE;
        }

        // Resolve address information
        if(FAILED(GetAddrInfoW(nullptr, std::to_wstring(_port).c_str(), &hints, &_addr_info))) {
            if(_socket_count == 1) {
                WSACleanup();
            }
            --_socket_count;
            return kstd::Error { fmt::format("Unable to resolve address information => {}", get_last_error()) };
        }

        // Create the socket
        _socket_handle = socket(_addr_info->ai_family, _addr_info->ai_socktype, _addr_info->ai_protocol);
#elif defined(PLATFORM_LINUX)
        _socket_handle = socket(AF_INET, static_cast<int>(_type), 0);
#else
        _socket_handle = socket(PF_INET, static_cast<int>(_type), 0);
#endif

        if(handle_invalid(_socket_handle)) {
#ifdef PLATFORM_WINDOWS
            FreeAddrInfoW(_addr_info);
            _addr_info = nullptr;
            if(_socket_count == 1) {
                WSACleanup();
            }

            --_socket_count;
#endif
            return kstd::Error { fmt::format("Unable to initialize socket => {}", get_last_error()) };
        }
        return {};
    }

    [[nodiscard]] auto Socket::bind() noexcept -> kstd::Result<void> {
        using namespace std::string_literals;

        if(handle_invalid(_socket_handle)) {
            return kstd::Error {"Unable to bind socket => Socket handle is not valid!"s};
        }

#if defined(PLATFORM_WINDOWS)
        // Bind the socket
        if(FAILED(::bind(_socket_handle, _addr_info->ai_addr, static_cast<int>(_addr_info->ai_addrlen)))) {

            // Free address info and close socket
            FreeAddrInfoW(_addr_info);
            _addr_info = nullptr;
            closesocket(_socket_handle);

            // Cleanup WSA and decrement socket count
            if(_socket_count == 1) {
                WSACleanup();
            }

            if(_socket_count > 0) {
                --_socket_count;
            }

            return kstd::Error {fmt::format("Unable to bind socket => {}", get_last_error())};
        }

        // Listen with socket if protocol is TCP
        if(_type == SocketType::TCP) {
            if(FAILED(listen(_socket_handle, SOMAXCONN))) {
                // Free address info and close socket
                FreeAddrInfoW(_addr_info);
                _addr_info = nullptr;
                closesocket(_socket_handle);

                // Cleanup WSA and decrement socket count
                if(_socket_count == 1) {
                    WSACleanup();
                }

                if(_socket_count > 0) {
                    --_socket_count;
                }

                return kstd::Error {fmt::format("Unable to bind socket => {}", get_last_error())};
            }
        }
#elif defined(PLATFORM_LINUX)
        if(setsockopt(_socket_handle, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &_port, sizeof(&_port))) {
            return kstd::Error {fmt::format("Unable to bind socket => {}", get_last_error())};
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(_port.get());
        if(::bind(_socket_handle, (struct sockaddr*)&address, sizeof(address)) < 0) {
            return kstd::Error {fmt::format("Unable to bind socket => {}", get_last_error())};
        }
#else
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htons(INADDR_ANY);
        address.sin_port = htons(_port.get());
        if(::bind(_socket_handle, (struct sockaddr*)&address, sizeof(address)) < 0) {
            return kstd::Error {fmt::format("Unable to bind socket => {}", get_last_error())};
        }
#endif
        return {};
    }

    auto Socket::operator=(Socket&& other) noexcept -> Socket& {
        _socket_handle = other._socket_handle;
        _buffer_size = other._buffer_size;
        _port = other._port;
        _type = other._type;

        other._socket_handle = 0;
#ifdef PLATFORM_WINDOWS
        _addr_info = other._addr_info;
        other._addr_info = nullptr;
        ++_socket_count;
#endif
        return *this;
    }

}// namespace sockslib